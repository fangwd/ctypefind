import sqlite3

# python3 faiss.py
# dot -Tsvg faiss.dot > faiss.svg

def strip_ns(name):
  return name.lstrip('faiss::')

def print_edges(conn, fp, name):
  cursor = conn.cursor()
  cursor.execute('''
    select d.name as decl, b.name as base
    from decl_base db
        join decl d on db.decl_id = d.id
      join decl b on db.base_id=b.id
    where b.name = ?
  ''', (name, ))
  rows = cursor.fetchall()
  for row in rows:
    fp.write(f"  {strip_ns(row[1])} -> {strip_ns(row[0])}\n")
    print_edges(conn, fp, row[0])

if __name__ == '__main__':
  conn = sqlite3.connect('faiss.db')
  with open('faiss.dot', 'w') as fp:
    fp.write('digraph D {\n')
    print_edges(conn, fp, 'faiss::Index')
    fp.write('}\n')
  conn.close()
