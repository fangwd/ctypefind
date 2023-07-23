import re
import json
import sqlite3

DB_NAME = "typetests.db"
def connect():
    conn = sqlite3.connect(DB_NAME)
    conn.row_factory = sqlite3.Row
    return conn, conn.cursor()


def select(query):
    if re.match(r'^\s*from\b', query, re.IGNORECASE):
        return f'select * {query}'
    return f'select {query}'


def lines(filename):
    with open(filename, 'r') as file:
      result = file.readlines()
      result = [line.rstrip() for line in result]
    return result


def first(query):
    _, cursor = connect()
    cursor.execute(select(query))
    return cursor.fetchone()


def all(query):
    _, cursor = connect()
    cursor.execute(select(query))
    columns = [column[0] for column in cursor.description]
    rows= []
    for row in cursor.fetchall():
        rows.append(dict(zip(columns, row)))

    return rows

def load_json(file):
    with open(f'tests/files/{file}.json') as fp:
        return json.load(fp)
