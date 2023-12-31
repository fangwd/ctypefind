#include "indexer.h"
#include "config.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Tooling/Tooling.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "membuf.h"

using namespace clang;

static const char *to_string(const AccessSpecifier access);
static std::string get_template_arg_kind_name(const clang::TemplateArgument::ArgKind &kind);
static std::string get_ns(const Decl *val);

class IndexerVisitor : public RecursiveASTVisitor<IndexerVisitor> {
  public:
    explicit IndexerVisitor(ASTContext &context, SourceManager *source_manager, Indexer &builder)
        : context_(context), source_manager_(source_manager), indexer_(builder) {}

    bool accept(const clang::Decl *d) {
        const clang::FileEntry *fe = getFileEntryForDecl(d, source_manager_);
        if (!fe) {
            if (auto decl = llvm::dyn_cast<clang::NamedDecl>(d)) {
                std::string name = decl->getQualifiedNameAsString();
                if (name.substr(0, 5) == "std::" || name.substr(0, 2) == "__") {
                    return false;
                }
            }
            return true;
        }
        return indexer_.accept(fe->getName().str().c_str());
    }

    db::Location location_of(const Decl *d) {
        const clang::FileEntry *fe = getFileEntryForDecl(d, source_manager_);
        db::Location location;
        location.file = fe ? fe->getName().str().c_str() : "";
        set_range(location, d->getSourceRange());
        return location;
    }

    void set_range(db::Location& location, const clang::SourceRange& range) {
        clang::SourceManager &sm = *source_manager_;
        clang::SourceLocation begin = range.getBegin();
        clang::SourceLocation end = range.getEnd();
        location.start_line = (int)sm.getSpellingLineNumber(begin);
        location.end_line = (int)sm.getSpellingLineNumber(end);
        location.start_column = (int)sm.getSpellingColumnNumber(begin);
        location.end_column = (int)sm.getSpellingColumnNumber(end);
    }

    bool VisitTagDecl(const TagDecl *d) {
        const clang::FileEntry *fe = getFileEntryForDecl(d, source_manager_);
        auto &db = indexer_.db();

        if (d->isThisDeclarationADefinition() && fe && accept(d)) {
            db::Decl row;
            row.type = d->getKindName();
            row.name = signature_of(d->getTypeForDecl()->getCanonicalTypeUnqualified());
            row.location = location_of(d);
            row.comment = comment_of(d);

            if (auto *record_decl = dyn_cast<CXXRecordDecl>(d)) {
                row.is_abstract = record_decl->isAbstract();
                row.is_template = record_decl->getDescribedClassTemplate() != nullptr;
                if (row.is_template) {
                    row.name = record_decl->getQualifiedNameAsString();
                }
                row.is_struct = record_decl->isStruct();
                bool inserted = false;
                db.insert(row, &inserted);

                for (const auto field : record_decl->fields()) {
                    db::DeclField decl_field;
                    decl_field.decl_id = row.id;
                    decl_field.type_id = insert_type(field->getType());
                    decl_field.name = field->getName();
                    decl_field.comment = comment_of(field);
                    decl_field.access = to_string(field->getAccess());
                    decl_field.location = location_of(field);
                    db.insert(decl_field);
                }

                size_t base_order = 0;
                for (const auto &base : record_decl->bases()) {
                    const RecordType *record_type = base.getType().getTypePtr()->getAs<RecordType>();
                    if (record_type) {
                        const RecordDecl *record_decl = record_type->getDecl();
                        const CXXRecordDecl *decl = dyn_cast<CXXRecordDecl>(record_decl);
                        if (decl) {
                            db::DeclBase r;
                            r.decl_id = row.id;
                            auto base_name = signature_of(decl->getTypeForDecl()->getCanonicalTypeUnqualified());
                            r.base_id = db.get_decl_id(base_name);
                            r.access = to_string(base.getAccessSpecifier());
                            r.position = base_order;
                            db.insert(r);
                        }
                    }
                    base_order++;
                }
                if (inserted) {
                    if (auto templ_decl = record_decl->getDescribedClassTemplate()) {
                        TemplateParameterList *params = templ_decl->getTemplateParameters();
                        int index = 0;
                        for (auto &param : *params) {
                            db::TemplateParam param_row;
                            param_row.template_id = row.id;
                            param_row.template_type = "class";
                            param_row.name = param->getNameAsString();
                            param_row.is_variadic = param->isParameterPack();
                            param_row.index = index++;
                            get_param_kind_and_value(param, param_row);
                            db.insert(param_row);
                        }
                    }
                }
            } else if (auto *enum_decl = dyn_cast<EnumDecl>(d)) {
                row.is_scoped = enum_decl->isScoped();
                db.insert(row);
            }
        }
        return true;
    }

    template <class Row>
    void get_param_kind_and_value(clang::NamedDecl *&param, Row &param_row) {
        if (auto p = dyn_cast<TemplateTypeParmDecl>(param)) {
            param_row.kind = "type";
            if (p->hasDefaultArgument()) {
                QualType default_type = p->getDefaultArgument();
                param_row.value = signature_of(default_type);
            }
        } else if (auto p = dyn_cast<NonTypeTemplateParmDecl>(param)) {
            param_row.kind = "non-type";
            param_row.type = signature_of(p->getType());
            if (p->hasDefaultArgument()) {
                Expr *expr = p->getDefaultArgument();
                if (expr) {
                    llvm::SmallString<1024> str;
                    llvm::raw_svector_ostream out(str);
                    PrintingPolicy policy(context_.getLangOpts());
                    expr->printJson(out, nullptr, policy, false);
                    param_row.value = str.c_str();
                }
            }
        } else if (auto p = dyn_cast<TemplateTemplateParmDecl>(param)) {
            param_row.kind = "template";
            if (p->hasDefaultArgument()) {
                const auto &arg = p->getDefaultArgument().getArgument();
                if (arg.getKind() == clang::TemplateArgument::Template) {
                    clang::TemplateDecl *decl = arg.getAsTemplate().getAsTemplateDecl();
                    param_row.value = decl->getQualifiedNameAsString();
                }
            }
        }
    }

    db::Function to_row(FunctionDecl *decl) {
        db::Function row;

        row.id = 0;
        row.name = decl->getNameAsString();
        row.qual_name = decl->getQualifiedNameAsString();
        row.signature = signature_of(decl);
        row.location = location_of(decl);
        row.comment = comment_of(decl);
        row.is_static = decl->isStatic();
        row.is_inline = decl->isInlineSpecified();
        row.type_id = insert_type(decl->getReturnType());
        if (const auto *method = dyn_cast<CXXMethodDecl>(decl)) {
            auto &db = indexer_.db();
            row.decl_id =
                db.get_decl_id(signature_of(method->getParent()->getTypeForDecl()->getCanonicalTypeInternal()));
            row.is_virtual = method->isVirtual();
            row.is_pure = method->isPure();
            row.is_ctor = isa<CXXConstructorDecl>(decl);
            row.is_overriding = method->size_overridden_methods() > 0;
            row.access = to_string(decl->getAccess());
        }
        return row;
    }

    bool VisitTypedefDecl(TypedefDecl *d) {
        db::Decl row;
        if (accept(d)) {
            row.type = "typedef";
            row.name = d->getQualifiedNameAsString();
            row.underlying_type = signature_of(d->getUnderlyingType());
            indexer_.db().insert(row);
        }
        return true;
    }

    bool VisitTypeAliasDecl(TypeAliasDecl *d) {
        db::Decl row;
        if (accept(d)) {
            row.type = "using";
            row.name = d->getQualifiedNameAsString();
            row.underlying_type = d->getUnderlyingType().getAsString();
            row.location = location_of(d);
            indexer_.db().insert(row);
        }
        return true;
    }

    bool VisitFunctionDecl(FunctionDecl *decl) {
        if (!accept(decl)) {
            return true;
        }

        auto row = to_row(decl);
        auto &db = indexer_.db();

        if (db.get_func_id(row.signature) > 0) {
            return true;
        }

        db.insert(row);

        if (decl->isTemplated() && decl->getDescribedFunctionTemplate()) {
            TemplateParameterList *params = decl->getDescribedFunctionTemplate()->getTemplateParameters();
            if (params) {
                int index = 0;
                for (auto &param : *params) {
                    db::TemplateParam param_row;
                    param_row.template_id = row.id;
                    param_row.template_type = "function";
                    param_row.name = param->getNameAsString();
                    param_row.is_variadic = param->isParameterPack();
                    param_row.index = index++;
                    get_param_kind_and_value(param, param_row);
                    indexer_.db().insert(param_row);
                }
            }
        }

        int position = 0;
        for (const auto &param : decl->parameters()) {
            db::FunctionParam r;
            r.function_id = row.id;
            r.position = position++;
            r.name = param->getNameAsString();
            r.type_id = insert_type(param->getType());
            if (auto *expr = param->getInit()) {
                r.default_value = to_json(expr);
            }
            indexer_.db().insert(r);
        }

        if (row.is_overriding) {
            auto *method = dyn_cast<CXXMethodDecl>(decl);
            for (const auto &m : method->overridden_methods()) {
                if (m->getParent() != method->getParent()) {
                    db::MethodOverride entry{.method_id = row.id,
                                             .overridden_method_id = indexer_.db().get_func_id(signature_of(m))};
                    db.insert(entry);
                }
            }
        }

        return true;
    }

    int insert_type(const QualType &type, int decl_id = 0) {
        auto &db = indexer_.db();
        db::Type row;
        std::vector<db::TypeArgument> type_arguments;
        QualType decl_type = type;
        for (const Type *p = type.getUnqualifiedType().getTypePtr(); p;) {
            if (const auto ptr = dyn_cast<PointerType>(p)) {
                decl_type = ptr->getPointeeType();
                p = ptr->getPointeeType().getTypePtr();
                row.indirection = '*' + row.indirection;
            } else if (const auto ref = dyn_cast<ReferenceType>(p)) {
                decl_type = ref->getPointeeType();
                p = ref->getPointeeType().getTypePtr();
                row.indirection = '&' + row.indirection;
            } else {
                if (const TagType *tag = p->getAs<TagType>()) {
                    const TagDecl *decl = tag->getDecl();
                    row.decl_name = signature_of(decl->getTypeForDecl()->getCanonicalTypeUnqualified());
                    row.decl_kind = decl->getKindName();
                } else if (const auto *typedefType = p->getAs<TypedefType>()) {
                    row.decl_name = typedefType->getDecl()->getQualifiedNameAsString();
                    if (dyn_cast<TypeAliasDecl>(typedefType->getDecl())) {
                        row.decl_kind = "using";
                    } else {
                        row.decl_kind = "typedef";
                    }
                } else if (p->isTemplateTypeParmType()) {
                    row.template_parameter_index = p->getAs<TemplateTypeParmType>()->getIndex();
                }
                if (auto specialisation = p->getAs<TemplateSpecializationType>()) {
                    // Note above: row.name = record_decl->getQualifiedNameAsString();
                    row.decl_name = specialisation->getTemplateName().getAsTemplateDecl()->getQualifiedNameAsString();

                    std::string args;
                    for (auto &arg : specialisation->template_arguments()) {
                        if (args.length() > 0) {
                            args += ",";
                        }
                        args += signature_of(arg);
                    }
                    int index = 0;
                    for (auto &arg : specialisation->template_arguments()) {
                        db::TypeArgument row;
                        row.kind = get_template_arg_kind_name(arg.getKind());
                        row.value = signature_of(arg);
                        row.index = index++;
                        if (arg.getKind() == clang::TemplateArgument::ArgKind::Type) {
                            auto type = arg.getAsType();
                            row.referenced_type_id = insert_type(type);
                        }
                        type_arguments.push_back(row);
                    }
                }

                break;
            }
        }

        if (row.decl_name.empty()) {
            row.decl_name = as_string(decl_type.getUnqualifiedType());
        }

        row.name = as_string(type);

        bool inserted = false;
        auto type_id = db.insert(row, &inserted);

        if (inserted) {
            for (auto &row : type_arguments) {
                row.type_id = type_id;
                db.insert(row);
            }
        }

        return type_id;
    }

    std::string as_string(QualType type) {
        // if (auto builtin = type.getTypePtr()->getAs<BuiltinType>()) {
        //     if (builtin->getKind() == BuiltinType::Bool) {
        //         // clang/lib/AST/Type.cpp: return Policy.Bool ? "bool" : "_Bool";
        //         return "bool";
        //     }
        // }
        return type.getAsString();
    }

    std::string pointee_of(const QualType &type) {
        const Type *p = type.getUnqualifiedType().getTypePtr();
        while (p) {
            if (const auto *ptr = dyn_cast<PointerType>(p)) {
                p = ptr->getPointeeType().getTypePtr();
            } else if (const auto *ref = dyn_cast<ReferenceType>(p)) {
                p = ref->getPointeeType().getTypePtr();
            } else {
                const TagType *tag = p->getAs<TagType>();
                if (tag) {
                    const TagDecl *decl = tag->getDecl();
                    return decl->getQualifiedNameAsString();
                }
            }
        }
        return signature_of(type);
    }

    std::string signature_of(const QualType &type) {
        PrintingPolicy pp(context_.getLangOpts());
        pp.SuppressTagKeyword = true;
        return type.getCanonicalType().getAsString(pp);
    }

    std::string signature_of(const FunctionDecl *d) {
        MemBuf mb;
        mb << d->getReturnType().getCanonicalType().getAsString() << ' ' << d->getQualifiedNameAsString() << '(';
        for (const auto &param : d->parameters()) {
            if (mb.last_char() != '(') {
                mb << ", ";
            }
            mb << param->getType().getCanonicalType().getAsString();
        }
        mb << ')';
        if (auto *method = dyn_cast<CXXMethodDecl>(d)) {
            if (method->isConst()) {
                mb << " const";
            }
        }
        return mb.content();
    }

    std::string signature_of(const clang::TemplateArgument &arg) {
        auto kind = arg.getKind();
        switch (kind) {
        case clang::TemplateArgument::ArgKind::Null:
            return "null";
        case clang::TemplateArgument::ArgKind::Type:
            return signature_of(arg.getAsType());
        case clang::TemplateArgument::ArgKind::Declaration:
            assert(0);
        case clang::TemplateArgument::ArgKind::NullPtr:
            return "nullptr";
        case clang::TemplateArgument::ArgKind::Integral:
            assert(0);
        case clang::TemplateArgument::ArgKind::Template:
            assert(0);
        case clang::TemplateArgument::ArgKind::TemplateExpansion:
            assert(0);
        case clang::TemplateArgument::ArgKind::Expression:
            return to_json(arg.getAsExpr());
            return "Expression";
        case clang::TemplateArgument::ArgKind::Pack:
            assert(0);
        }
        return "?";
    }

    std::string to_json(const Expr *expr) {
        llvm::SmallString<1024> str;
        llvm::raw_svector_ostream out(str);
        PrintingPolicy policy(context_.getLangOpts());

        expr->printJson(out, nullptr, policy, false);

        std::string ns;
        if (const auto *ref = dyn_cast<DeclRefExpr>(expr)) {
            const ValueDecl *val = ref->getDecl();
            if (const_cast<ValueDecl *>(val)->getKind() == Decl::EnumConstant) {
                ns = get_ns(val);
            }
        }

        if (!ns.empty()) {
            ns += "::";
        }

        return ns + std::string(str.c_str());
    }

    bool VisitEnumConstantDecl(const EnumConstantDecl *decl) {
        if (accept(decl)) {
            auto &db = indexer_.db();
            const clang::EnumDecl *enum_decl = dyn_cast<clang::EnumDecl>(decl->getDeclContext());
            db::EnumField field;
            field.enum_id =
                db.get_decl_id(signature_of(enum_decl->getTypeForDecl()->getCanonicalTypeUnqualified()));
            field.name = decl->getNameAsString();
            field.value = decl->getInitVal().getSExtValue();
            field.location = location_of(decl);
            field.comment = comment_of(decl);
            db.insert(field);
        }
        return true;
    }

    bool VisitVarDecl(const VarDecl *decl) {
        db::VarDecl row;
        row.location = location_of(decl);
        row.type_id = insert_type(decl->getType());
        row.name = decl->getNameAsString();
        indexer_.db().insert(row);
        return true;
    }

    bool VisitDeclRefExpr(DeclRefExpr *ref) {
        auto decl = ref->getDecl();
        if (auto *var_decl = dyn_cast<VarDecl>(decl)) {
            auto fe = source_manager_->getFileEntryForID(source_manager_->getFileID(ref->getLocation()));
            if (fe) {
                auto& db = indexer_.db();
                db::VarRef row;
                db::Location var_loc = location_of(var_decl);
                row.var_id = db.get_var_id(var_loc.file, var_loc.end_line, var_loc.end_column);
                row.location.file = fe->getName();
                set_range(row.location, ref->getSourceRange());
                row.location.end_column = row.location.start_column + var_decl->getNameAsString().length() - 1;
                db.insert(row);
            }
        }
        return true;
    }

    bool VisitCallExpr(CallExpr *expr) {
        if (FunctionDecl *decl = expr->getDirectCallee()) {
            auto fe = source_manager_->getFileEntryForID(source_manager_->getFileID(expr->getExprLoc()));
            if (fe) {
                auto &db = indexer_.db();
                db::FCall row;
                row.func_id = db.get_func_id(signature_of(decl));
                row.location.file = fe->getName();
                set_range(row.location, expr->getSourceRange());
                db.insert(row);
            }
        }
        return true;
    }

    bool VisitFieldDecl(FieldDecl *field) {
        if (RecordDecl *decl = field->getParent()) {
            auto &db = indexer_.db();
            db::VarDecl row;
            auto class_name = signature_of(decl->getTypeForDecl()->getCanonicalTypeUnqualified());
            row.class_id = db.get_decl_id(class_name);
            row.location = location_of(field);
            row.type_id = insert_type(field->getType());
            row.name = field->getNameAsString();
            indexer_.db().insert(row);
        }
        return true;
    }

    bool VisitMemberExpr(MemberExpr *expr) {
        if (FieldDecl *field = dyn_cast<FieldDecl>(expr->getMemberDecl())) {
            auto fe = source_manager_->getFileEntryForID(source_manager_->getFileID(expr->getExprLoc()));
            if (fe) {
                auto &db = indexer_.db();
                db::VarRef row;
                db::Location var_loc = location_of(field);
                row.var_id = db.get_var_id(var_loc.file, var_loc.end_line, var_loc.end_column);
                row.location.file = fe->getName();
                set_range(row.location, expr->getSourceRange());
                row.location.end_column = row.location.start_column + field->getNameAsString().length() - 1;
                db.insert(row);
            }
        }
        return true;
    }

    db::Comment comment_of(const Decl *d) {
        db::Comment comment;
        const auto *rc = d->getASTContext().getRawCommentForDeclNoCache(d);
        if (rc) {
            comment.raw = rc->getRawText(*source_manager_).str();
            comment.brief = rc->getBriefText(context_);
        }
        return comment;
    }

    static const clang::FileEntry *getFileEntryForDecl(const clang::Decl *decl, clang::SourceManager *sm) {
        if (!decl || !sm) {
            return 0;
        }
        return sm->getFileEntryForID(sm->getFileID(decl->getLocation()));
    }

  private:
    ASTContext &context_;
    SourceManager *source_manager_;
    Indexer &indexer_;
};

class IndexerASTConsumer : public clang::ASTConsumer {
  public:
    IndexerASTConsumer(ASTContext &context, SourceManager *source_manager, Indexer &builder)
        : visitor(context, source_manager, builder) {}

  private:
    virtual void HandleTranslationUnit(clang::ASTContext &context) {
        visitor.TraverseDecl(context.getTranslationUnitDecl());
    }

    IndexerVisitor visitor;
};

class IndexerAction : public clang::ASTFrontendAction {
  private:
    virtual std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &compiler,
            llvm::StringRef inFile) {
        std::unique_ptr<clang::ASTConsumer> consumer(
            new IndexerASTConsumer(compiler.getASTContext(), &compiler.getSourceManager(), indexer_));
        return consumer;
    }

  public:
    IndexerAction(Indexer &builder) : indexer_(builder) {}

  private:
    Indexer &indexer_;
};

static const char *to_string(const AccessSpecifier access) {
    switch (access) {
    case clang::AccessSpecifier::AS_public:
        return "public";
    case clang::AccessSpecifier::AS_protected:
        return "protected";
    case clang::AccessSpecifier::AS_private:
        return "private";
    case clang::AccessSpecifier::AS_none:
        return "none";
    }
    return "?";
}

bool Indexer::run(std::vector<std::string> &options) {
    clang::FileManager *file_manager = new clang::FileManager(clang::FileSystemOptions());
    clang::tooling::ToolInvocation tool_invocation(options, std::make_unique<IndexerAction>(*this), file_manager);
    return tool_invocation.run();
}

bool Indexer::accept(const char *filename) {
    if (!filename || !filename[0]) {
        return false;
    }

    if (config.accept_paths.size() == 0) {
        return filename[0] != '/';
    }

    for (const auto &path : config.accept_paths) {
        if (strlen(filename) >= path.size()) {
            if (strncmp(filename, path.c_str(), path.size()) == 0) {
                if (config.verbose) {
                    printf("Accepted %s\n", filename);
                }
                return true;
            }
        }
    }
    if (config.verbose) {
        printf("Rejected %s\n", filename);
    }
    return false;
}

static std::string get_template_arg_kind_name(const clang::TemplateArgument::ArgKind &kind) {
    switch (kind) {
    case clang::TemplateArgument::ArgKind::Null:
        return "Null";
    case clang::TemplateArgument::ArgKind::Type:
        return "Type";
    case clang::TemplateArgument::ArgKind::Declaration:
        return "Declaration";
    case clang::TemplateArgument::ArgKind::NullPtr:
        return "NullPtr";
    case clang::TemplateArgument::ArgKind::Integral:
        return "Integral";
    case clang::TemplateArgument::ArgKind::Template:
        return "Template";
    case clang::TemplateArgument::ArgKind::TemplateExpansion:
        return "TemplateExpansion";
    case clang::TemplateArgument::ArgKind::Expression:
        return "Expression";
    case clang::TemplateArgument::ArgKind::Pack:
        return "Pack";
    }
    return "?";
}

static std::string get_ns(const Decl *val) {
    std::string ns;
    const DeclContext *ctx = val->getDeclContext();
    while (ctx) {
        if (const NamespaceDecl *decl = dyn_cast<NamespaceDecl>(ctx)) {
            if (!ns.empty()) ns = "::" + ns;
            ns = decl->getNameAsString() + ns;
        }
        ctx = ctx->getParent();
    }
    return ns;
}
