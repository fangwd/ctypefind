#pragma once

#include <sqlite3.h>

#include "membuf.h"

namespace db {

struct Location {
    std::string file;
    int start_line = 0;
    int end_line = 0;
};

struct Comment {
    std::string raw;
    std::string brief;
};

// A NamedDecl (class/union/enum).
struct Decl {
    int id = 0;
    std::string type;
    std::string name;
    Location location;
    Comment comment;

    std::string underlying_type;

    // classes
    bool is_struct = false;
    bool is_abstract = false;
    bool is_template = false;

    // union
    bool is_scoped = false;  // enum class
};

struct TemplateParam {
    int id;
    int template_id;
    std::string template_type;   // function or class
    std::string name;   // e.g. T
    std::string kind;   // type/non-type/template
    std::string type;   // when kind is non-type, this is the param type name
    std::string value;  // type name/value/template name
    bool is_variadic;   // isParameterPack
    int index;
};

struct DeclBase {
    int id = 0;
    int decl_id = 0;
    int base_id = 0;
    int position = 0;
    std::string access;
};

struct DeclField {
    int id = 0;
    int decl_id = 0;
    int type_id = 0;
    std::string name;
    std::string access;
    Location location;
    Comment comment;
};

struct EnumField {
    int id = 0;
    int enum_id = 0;
    std::string name;
    int value;
    Location location;
    Comment comment;
};

struct Type {
    int id = 0;
    std::string name;
    std::string qual_name; // todo: delete this
    std::string decl_name;
    std::string decl_kind;
    int template_parameter_index = -1;
};

// TemplateArgument for TemplateSpecializationType
struct TemplateArgument {
    int id;
    int template_id;
    std::string kind;   // TemplateArgument::ArgKind
    std::string value;  // type name, expr, etc
    int index;
};

struct Function {
    int id = 0;
    std::string name;
    std::string signature;
    Location location;
    Comment comment;
    int class_id = 0;
    std::string access;
    bool is_static = false;
    bool is_inline = false;
    bool is_virtual = false;
    bool is_pure = false;
    bool is_ctor = false;
    bool is_overriding = false;
    bool is_const = false;
};

struct FunctionParam {
    int id = 0;
    int function_id = 0;
    int position = 0;
    int type_id = 0;
    std::string name;
};

struct MethodOverride {
    int id = 0;
    int method_id = 0;
    int overridden_method_id = 0;
};

class Database {
   private:
    sqlite3 *db_;

    int create_tables();
    int table_count();

    int get_int(const MemBuf &);
    int exec(const MemBuf &);

    int get_file_id(const std::string &path);

    int update_location(const char *table, int id, Location &);
    int update_comment(const char *table, int id, Comment &);

   public:
    Database(const char *dbname);
    Database(std::string &dbname) : Database(dbname.c_str()) {}
    ~Database();

    int clear();

    int get_decl_id(const std::string &name, bool *inserted = nullptr);
    int get_type_id(const std::string &name);
    int get_func_id(const std::string &signature);

    int insert(Decl &decl, bool *inserted = nullptr);
    int insert(TemplateParam &param);

    int insert(DeclBase &base);
    int insert(DeclField &field);
    int insert(EnumField &field);
    int insert(Type &type, bool *inserted);
    int insert(TemplateArgument &arg);
    int insert(Function &func);
    int insert(FunctionParam &param);
    int insert(MethodOverride &mo);
};

}  // namespace db
