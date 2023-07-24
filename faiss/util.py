import sqlite3
from decls import Decl, Function, Argument
from typing import List


def init(clazz, row, cols):
    obj = clazz()
    for i, col in enumerate(cols):
        setattr(obj, col, row[i])
    return obj


def set_func_params(db: sqlite3.Connection, func: Function) -> Function:
    query = """select p.name, t.name as type_name, p.default_value, f.path as type_file
               from func_param p join `type` t on t.id = p.type_id
                  left join decl d on t.name = d.name
                  left join file f on f.id = d.file_id
               where p.func_id = ?
               order by p.position
            """
    
    cur = db.cursor()
    cur.execute(query, (func.id,))
    rows = cur.fetchall()
    cols = [column[0] for column in cur.description]
    func.args = [init(Argument, row, cols) for row in rows]
    return func


def set_decl_ctors(db: sqlite3.Connection, decl: Decl) -> Decl:
    query = "select f.id, f.name, f.comment from func f where f.is_ctor and f.class_id = ?"
    cur = db.cursor()
    cur.execute(query, (decl.id,))
    rows = cur.fetchall()
    cols = [column[0] for column in cur.description]
    decl.ctors = [set_func_params(db, init(Function, row, cols)) for row in rows]
    return decl


def find_decl(db: sqlite3.Connection, id: int = None, name: str = None) -> Decl:
    query = """select d.id, d.type as kind, d.name, d.comment, d.is_abstract, f.path as file
               from decl d left join file f on f.id = d.file_id
            """
    if name is not None:
        query += " where name = ?"
        args = (name,)
    else:
        query += " where d.id = ?"
        args = (id,)

    cur = db.cursor()
    cur.execute(query, args)
    row = cur.fetchone()

    if row is None:
        return None

    cols = [column[0] for column in cur.description]

    return set_decl_ctors(db, init(Decl, row, cols))


def find_descendent_decls(db, base_id):
    cur = db.cursor()
    cur.execute("""select d.id, d.type as kind, d.name, d.comment, d.is_abstract, f.path as file
     from decl_base db join decl d on db.decl_id = d.id left join file f on f.id = d.file_id
     where db.base_id = ?""", (base_id,))
    rows = cur.fetchall()
    cols = [column[0] for column in cur.description]
    result = []
    for row in rows:
        decl = set_decl_ctors(db, init(Decl, row, cols))
        result.append(decl)
        result = result + find_descendent_decls(db, decl.id)
    return result
