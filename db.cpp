#include "db.h"
#include "sql.h"
#include "util.h"
#include <unistd.h>
#include <cstdio>

#define log_error(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

namespace db {

Database::Database(const char *dbname) : db_(nullptr) {
    auto error = sqlite3_open(dbname, &db_);
    if (error != 0) {
        log_error("Failed to open %s: %s", dbname, sqlite3_errstr(error));
    } else if (table_count() == 0) {
        create_tables();
    }
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

int Database::table_count() {
    const char *query = "SELECT COUNT(*) FROM sqlite_master WHERE type='table'";
    sqlite3_stmt *stmt;

    int result = 0;

    if (sqlite3_prepare_v2(db_, query, -1, &stmt, NULL) != SQLITE_OK) {
        log_error("Error preparing statement: %s", sqlite3_errmsg(db_));
        return -1;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int(stmt, 0);
    } else {
        log_error("Error preparing statement: %s", sqlite3_errmsg(db_));
        result = -1;
    }

    sqlite3_finalize(stmt);

    return result;
}

int Database::create_tables() {
    const char *sql = R"sql(
create table `file`(
  id integer primary key,
  path varchar(1024),
  constraint uk_file unique(path)
);

create table decl(
  id integer primary key,
  type varchar(60),
  name varchar(1024),
  file_id int,
  start_line int,
  end_line int,
  start_column int,
  end_column int,
  brief_comment text,
  comment text,

  -- typedef and using
  underlying_type varchar(100),

  is_struct bool,
  is_abstract bool,
  is_template bool,
  is_scoped bool,

  constraint uk_decl unique(name),
  constraint fk_decl_file foreign key (file_id) references `file`(id) on delete cascade
);

create table template_parameter(
  id integer primary key,
  template_id int,
  template_type varchar(20) not null,
  kind varchar(30),
  type varchar(100),
  name varchar(100),
  value varchar(200),
  is_variadic bool,
  `index` int, 
  constraint uk_template_parameter unique(template_type, template_id, name),
  constraint fk_template_parameter_template foreign key (template_id) references decl(id) on delete cascade
);

create table decl_base(
  id integer primary key,
  decl_id int,
  base_id int,
  position int,
  access varchar(30),
  constraint uk_decl_base unique(decl_id, base_id),
  constraint fk_decl_base_decl foreign key (decl_id) references decl(id) on delete cascade,
  constraint fk_decl_base_base foreign key (base_id) references decl(id) on delete cascade
);

create table decl_tree(
  id integer primary key,
  decl_id int,
  base_id int,
  level int,
  constraint uk_decl_tree unique(decl_id, base_id, level),
  constraint fk_decl_tree_decl foreign key (decl_id) references decl(id) on delete cascade,
  constraint fk_decl_tree_base foreign key (base_id) references decl(id) on delete cascade
);

create table `type`(
  id integer primary key,
  name varchar(200) not null,
  decl_name varchar(200) not null,
  decl_kind varchar(30),
  indirection varchar(20),
  template_parameter_index int,
  constraint uk_type unique(name, template_parameter_index)
);

create table `type_argument`(
  id integer primary key,
  type_id int,
  kind varchar(32),
  `value` varchar(200),
  `index` int,
  referenced_type_id int,
  constraint fk_type_argument_template foreign key (type_id) references `type`(id) on delete cascade,
  constraint fk_type_argument_template foreign key (referenced_type_id) references `type`(id) on delete cascade,
  constraint uk_type_argument_template_index unique(type_id, `index`)
);

create table decl_field(
  id integer primary key,
  decl_id int,
  type_id int,
  name varchar(100),
  access varchar(30),
  brief_comment text,
  comment text,
  file_id int,
  start_line int,
  end_line int,
  start_column int,
  end_column int,
  constraint uk_decl_field unique(decl_id, name),
  constraint fk_decl_field_decl foreign key (decl_id) references decl(id) on delete cascade,
  constraint fk_decl_field_type foreign key (type_id) references `type`(id) on delete cascade
);

create table enum_field(
  id integer primary key,
  enum_id int,
  name varchar(100),
  value int,
  brief_comment text,
  comment text,
  file_id int,
  start_line int,
  end_line int,
  start_column int,
  end_column int,
  constraint uk_enum_field unique(enum_id, name)
);

create table func(
  id integer primary key,
  name varchar(200),
  qual_name varchar(200),
  signature varchar(512),
  file_id int,
  start_line int,
  end_line int,
  start_column int,
  end_column int,
  brief_comment text,
  comment text,
  decl_id int,
  type_id int,
  access varchar(32),
  is_static bool,
  is_inline bool,
  is_virtual bool,
  is_pure bool,
  is_ctor bool,
  is_overriding bool,
  is_const bool,
  constraint uk_func unique(signature),
  constraint fk_func_file foreign key (file_id) references file(id) on delete cascade,
  constraint fk_func_type foreign key (type_id) references `type`(id) on delete cascade,
  constraint fk_func_class foreign key (decl_id) references decl(id) on delete cascade
);

create table func_param(
  id integer primary key,
  func_id int,
  position int,
  type_id int,
  name varchar(200),
  default_value varchar(200),
  constraint uk_func_param unique(func_id, position),
  constraint fk_func_param_type foreign key (type_id) references `type`(id) on delete cascade,
  constraint fk_func_param_func foreign key (func_id) references func(id) on delete cascade
);

create table method_override(
  id integer primary key,
  method_id int,
  overridden_method_id int,
  constraint uk_method_override unique(method_id, overridden_method_id),
  constraint fk_method_override_method foreign key (method_id) references func(id) on delete cascade,
  constraint fk_method_override_overridden_method foreign key (overridden_method_id) references func(id) on delete cascade
);

create table var_decl(
  id integer primary key,
  class_id int,
  type_id int,
  name varchar(200),
  file_id int,
  start_line int,
  end_line int,
  start_column int,
  end_column int,
  constraint uk_var_decl unique(file_id, end_line, end_column),
  constraint fk_var_decl_file foreign key (file_id) references file(id) on delete cascade,
  constraint fk_var_decl_class foreign key (class_id) references `decl`(id) on delete cascade,
  constraint fk_var_decl_type foreign key (type_id) references `type`(id) on delete cascade
);

create table var_ref(
  id integer primary key,
  var_id int,
  file_id int,
  start_line int,
  end_line int,
  start_column int,
  end_column int,
  constraint uk_var_ref unique(file_id, end_line, end_column),
  constraint fk_var_ref_var foreign key (var_id) references var_decl(id) on delete cascade
);

create table fcall(
  id integer primary key,
  func_id int,
  file_id int,
  start_line int,
  end_line int,
  start_column int,
  end_column int,
  constraint uk_fcall unique(file_id, end_line, end_column),
  constraint fk_fcall_func foreign key (func_id) references func(id) on delete cascade
);
)sql";

    char *errmsg;
    int result = sqlite3_exec(db_, sql, nullptr, nullptr, &errmsg);

    if (result != SQLITE_OK) {
        log_error("Error executing query: %s", errmsg);
        sqlite3_free(errmsg);
    }

    return result;
}

int Database::clear() {
    const char *sql = R"sql(
delete from var_ref;
delete from var_decl;
delete from decl_base;
delete from decl_tree;
delete from template_parameter;
delete from type_argument;
delete from type;
delete from enum_field;
delete from decl_field;
delete from decl;
delete from file;
delete from fcall;
delete from func;
delete from func_param;
delete from method_override;
    )sql";

    char *errmsg;
    int result = sqlite3_exec(db_, sql, nullptr, nullptr, &errmsg);

    if (result != SQLITE_OK) {
        log_error("Error executing query: %s", errmsg);
        sqlite3_free(errmsg);
    }

    return result;
}

int Database::get_file_id(const std::string &path) {
    MemBuf mb;
    mb.printf("select id from `%s` where `%s` = '%s'", "file", "path", path.c_str());

    int id = get_int(mb);
    if (id == 0) {
        mb.clear();
        mb.printf("insert into file(path) values('%s')", path.c_str());
        return exec(mb);
    }

    return id;
}

int Database::update_location(const char *table, int id, Location &location) {
    MemBuf mb;

    mb.printf("update `%s` set file_id=%d, start_line=%d, end_line=%d, start_column=%d, end_column=%d where id=%d",
              table, get_file_id(location.file), location.start_line, location.end_line,location.start_column,
              location.end_column, id);

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_, mb.content(), -1, &stmt, nullptr);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return 0;
}

int Database::update_comment(const char *table, int id, Comment &comment) {
    MemBuf mb;
    mb.printf("update `%s` set brief_comment=?, comment=? where id=%d", table, id);

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db_, mb.content(), -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, comment.brief.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, comment.raw.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return 0;
}

int Database::insert(Decl &decl, bool *inserted) {
    const auto &location = decl.location;
    const auto &comment = decl.comment;
    decl.id = get_decl_id(decl.name, inserted);

    MemBuf mb;

    mb << "update decl set " << sql::field("type", decl.type, true) << sql::field("file_id", get_file_id(location.file))
       << sql::field("start_line", location.start_line) << sql::field("end_line", location.end_line)
       << sql::field("start_column", location.start_column) << sql::field("end_column", location.end_column)
       << sql::field("is_struct", decl.is_struct) << sql::field("is_abstract", decl.is_abstract)
       << sql::field("is_template", decl.is_template) << sql::field("is_scoped", decl.is_scoped)
       << sql::field("brief_comment", comment.brief) << sql::field("comment", comment.raw)
       << sql::field("underlying_type", decl.underlying_type) << " where "
       << sql::field("id", decl.id, true);

    exec(mb);

    return decl.id;
}

int Database::insert(TemplateParam &row) {
    MemBuf mb;
    mb << "insert into template_parameter(template_id, template_type, kind, type, name, value, is_variadic, `index`) "
       "values ("
       << row.template_id << ", " << sql::str(row.template_type) << "," << sql::str(row.kind) << ", "
       << sql::str(row.type) << ", " << sql::str(row.name) << ", " << sql::str(row.value) << ", " << row.is_variadic
       << ", " << row.index << ")";
    return (row.id = exec(mb));
}

int Database::insert(DeclBase &row) {
    MemBuf mb;
    mb.printf(
        "insert into decl_base(decl_id, base_id, position, access) "
        "values(%d, %d, %d, '%s')",
        row.decl_id, row.base_id, row.position, row.access.c_str());
    row.id = exec(mb);

    mb.clear();

    mb.printf(
        "insert into decl_tree(decl_id, base_id, level) "
        "values(%d, %d, 1)", row.decl_id, row.base_id);
    exec(mb);

    mb.clear();

    mb.printf(
        "insert into decl_tree(decl_id, base_id, level) "
        "select %d, base_id, level + 1 from decl_tree where decl_id=%d",
        row.decl_id, row.base_id);
    exec(mb);

    return row.id;
}

int Database::insert(DeclField &row) {
    MemBuf mb;
    mb.printf(
        "insert into decl_field(decl_id, type_id, name, access) "
        "values(%d, %d, '%s', '%s')",
        row.decl_id, row.type_id, row.name.c_str(), row.access.c_str());

    row.id = exec(mb);

    update_location("decl_field", row.id, row.location);
    update_comment("decl_field", row.id, row.comment);

    return row.id;
}

int Database::insert(EnumField &row) {
    MemBuf mb;
    mb.printf(
        "insert into enum_field(enum_id, name, value) "
        "values(%d, '%s', %d)",
        row.enum_id, row.name.c_str(), row.value);

    row.id = exec(mb);

    update_location("enum_field", row.id, row.location);
    update_comment("enum_field", row.id, row.comment);

    return row.id;
}

int Database::exec(const MemBuf &mb) {
    sqlite3_stmt *stmt;

    int error = sqlite3_prepare_v2(db_, mb.content(), -1, &stmt, nullptr);
    if (error != SQLITE_OK) {
        log_error("Error: %s\nQuery was: %s", sqlite3_errmsg(db_), mb.content());
        return -1;
    }

    if ((error = sqlite3_step(stmt)) != SQLITE_DONE) {
        log_error("Error: %s (%s)", sqlite3_errstr(error), mb.content());
        return -1;
    }

    long long last_id = sqlite3_last_insert_rowid(db_);

    sqlite3_finalize(stmt);

    return (int)last_id;
}

int Database::get_int(const MemBuf &mb) {
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db_, mb.content(), -1, &stmt, nullptr);

    int id = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        id = (int)sqlite3_column_int64(stmt, 0);
    }

    sqlite3_finalize(stmt);

    return id;
}

int Database::get_decl_id(const std::string &name, bool *inserted) {
    MemBuf mb;
    mb.printf("select id from decl where name = '%s'", name.c_str());
    int id = get_int(mb);
    if (id == 0) {
        mb.clear();
        mb.printf("insert into decl(type, name, is_scoped) values ('', '%s', false)", name.c_str());
        if (inserted) {
            *inserted = true;
        }
        return exec(mb);
    }
    return id;
}

int Database::get_func_id(const std::string &signature) {
    MemBuf mb;
    mb.printf("select id from func where signature = '%s'", signature.c_str());
    return get_int(mb);
}

int Database::get_type_id(const std::string &qual_name) {
    MemBuf mb;
    mb.printf("select id from `type` where qual_name = '%s'", qual_name.c_str());
    return get_int(mb);
}

int Database::get_var_id(const std::string &file, int end_line, int end_column) {
    MemBuf mb;
    mb.printf("select id from var_decl where file_id=%d and end_line=%d and end_column=%d", get_file_id(file), end_line,
              end_column);
    return get_int(mb);
}

int Database::insert(Type &row, bool *inserted) {
    MemBuf mb;
    mb.printf("select id from `type` where `name` = '%s' and template_parameter_index = %d", row.name.c_str(),
              row.template_parameter_index);

    int id = get_int(mb);

    if (id == 0) {
        mb.clear();
        mb << "insert into type(name, decl_name, decl_kind, indirection, "
           << "template_parameter_index) values (" << sql::str(row.name) << ", " << sql::str(row.decl_name) << ", "
           << sql::str(row.decl_kind) << "," << sql::str(row.indirection, false) << "," << row.template_parameter_index
           << ")";
        id = exec(mb);
        if (inserted) {
            *inserted = true;
        }
    }

    return (row.id = id);
}

int Database::insert(TypeArgument &row) {
    MemBuf mb;
    mb << "insert into type_argument(type_id, kind, value, `index`, referenced_type_id) "
       << "values (" << row.type_id << ", " << sql::str(row.kind) << ", " << sql::str(row.value) << ", "
       << row.index << "," << sql::pk(row.referenced_type_id) << ")";
    row.id = exec(mb);
    if (row.id <= 0) {
        log_error("Failed to insert template argument");
    }
    return row.id;
}

int Database::insert(Function &row) {
    MemBuf mb;
    mb << "insert into func(name, qual_name, signature, decl_id, type_id, access, is_static, is_inline, "
       << "is_virtual, is_pure, is_ctor, is_overriding, is_const) values (" << sql::str(row.name) << ", "
       << sql::str(row.qual_name) << ", " << sql::str(row.signature) << ", " << sql::pk(row.decl_id) << ", "
       << sql::pk(row.type_id) << "," << sql::str(row.access) << ", " << row.is_static << ", " << row.is_inline << ", "
       << row.is_virtual << ", " << row.is_pure << ", " << row.is_ctor << ", " << row.is_overriding << ", "
       << row.is_const << ")";
    row.id = exec(mb);
    update_location("func", row.id, row.location);
    update_comment("func", row.id, row.comment);
    return row.id;
}

int Database::insert(FunctionParam &row) {
    MemBuf mb;
    mb << "insert into func_param(func_id, position, type_id, name, default_value) values (" << row.function_id << ","
       << row.position << "," << row.type_id << "," << sql::str(row.name) << ", " << sql::str(row.default_value) << ")";
    return (row.id = exec(mb));
}

int Database::insert(MethodOverride &row) {
    MemBuf mb;

    mb.printf(R"SQL(
              insert into method_override(method_id, overridden_method_id)
              values (%d, %d))SQL",
    row.method_id, row.overridden_method_id);

    return (row.id = exec(mb));
}

int Database::insert(VarDecl &row) {
    MemBuf mb;
    mb << "insert or ignore into var_decl(class_id, type_id, name, file_id, start_line, end_line, start_column, end_column) "
       << "values (" << sql::pk(row.class_id) << "," << sql::pk(row.type_id) << "," << sql::str(row.name) << ","
       << get_file_id(row.location.file) << "," << row.location.start_line << "," << row.location.end_line << ","
       << row.location.start_column << "," << row.location.end_column << ")";
    row.id = exec(mb);
    return row.id;
}

int Database::insert(VarRef &row) {
    MemBuf mb;
    mb << "insert or ignore into var_ref(var_id, file_id, start_line, end_line, start_column, end_column) "
       << "values (" << sql::pk(row.var_id) << "," << get_file_id(row.location.file)
       << "," << row.location.start_line << "," << row.location.end_line << "," << row.location.start_column
       << "," << row.location.end_column << ")";
    row.id = exec(mb);
    return row.id;
}

int Database::insert(FCall& row) {
    MemBuf mb;
    mb << "insert or ignore into fcall(func_id, file_id, start_line, end_line, start_column, end_column) "
       << "values (" << sql::pk(row.func_id) << "," << get_file_id(row.location.file)
       << "," << row.location.start_line << "," << row.location.end_line << "," << row.location.start_column
       << "," << row.location.end_column << ")";
    row.id = exec(mb);
    return row.id;
}

}  // namespace db
