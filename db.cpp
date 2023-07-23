#include "db.h"

#include <unistd.h>

#include <cstdio>

#include "sql.h"

#define log_error(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

namespace db {

Database::Database(const char *dbname) {
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
    static const char *schema_file = "schema/schema.sql";

    MemBuf mb;
    if (!mb.load(schema_file)) {
        log_error("Failed to open schema file: %s", schema_file);
        return -1;
    }

    char *errmsg;
    int result = sqlite3_exec(db_, mb.content(), nullptr, nullptr, &errmsg);

    if (result != SQLITE_OK) {
        log_error("Error executing query: %s", errmsg);
        sqlite3_free(errmsg);
    }

    return result;
}

int Database::clear() {
    MemBuf mb;
    mb.load("schema/clear.sql");

    char *errmsg;
    int result = sqlite3_exec(db_, mb.content(), nullptr, nullptr, &errmsg);

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
    //     mb_.clear();
    //     mb_.printf("update `%s` set file_id=%d, start_line=%d, end_line=%d where id=%d", table, location.file_id,
    //                location.start_line, location.end_line, id);

    //     sqlite3_stmt *stmt;
    //     sqlite3_prepare_v2(db_, mb_.content(), -1, &stmt, nullptr);
    //     sqlite3_step(stmt);
    //     sqlite3_finalize(stmt);

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
    return (row.id = exec(mb));
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
        mb.printf("insert into decl(name) values ('%s')", name.c_str());
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

int Database::insert(Type &row, bool *inserted) {
    MemBuf mb;
    mb.printf("select id from `type` where `qual_name` = '%s'", row.qual_name.c_str());

    int id = get_int(mb);

    if (id == 0) {
        mb.clear();
        mb << "insert into type(name, qual_name, decl_kind, "
           << "template_parameter_index) values (" << sql::str(row.name) << ", " << sql::str(row.qual_name) << ", "
           << sql::str(row.decl_kind) << "," << row.template_parameter_index << ")";
        id = exec(mb);
        if (inserted) {
            *inserted = true;
        }
    }

    return (row.id = id);
}

int Database::insert(TemplateArgument &row) {
    MemBuf mb;
    mb << "insert into template_argument(template_id, kind, value, `index`) "
       << "values (" << row.template_id << ", " << sql::str(row.kind) << ", " << sql::str(row.value) << ", "
       << row.index << ")";
    row.id = exec(mb);
    if (row.id <= 0) {
        log_error("Failed to insert template argument");
    }
    return row.id;
}

int Database::insert(Function &row) {
    MemBuf mb;
    mb << "insert into func(name, signature, class_id, access, is_static, is_inline, "
       << "is_virtual, is_pure, is_ctor, is_overriding, is_const) values (" << sql::str(row.name) << ", "
       << sql::str(row.signature) << ", " << sql::pk(row.class_id) << ", " << sql::str(row.access) << ", "
       << row.is_static << ", " << row.is_inline << ", " << row.is_virtual << ", " << row.is_pure << ", " << row.is_ctor
       << ", " << row.is_overriding << ", " << row.is_const << ")";
    row.id = exec(mb);
    update_location("func", row.id, row.location);
    update_comment("func", row.id, row.comment);
    return row.id;
}

int Database::insert(FunctionParam &row) {
    MemBuf mb;
    mb.printf(R"SQL(
        insert into func_param(func_id, position, type_id, name)
        values (%d, %d, %d, '%s')
    )SQL",
              row.function_id, row.position, row.type_id, row.name.c_str());

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

}  // namespace db
