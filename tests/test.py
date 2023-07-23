import os
import subprocess
import unittest
import pprint
from util import DB_NAME, all, load_json, lines

pp = pprint.PrettyPrinter(indent=4)  # pp.pprint(dict(row))


def parse(filename: str):
    return subprocess.call([
        "./typefind", "--db", DB_NAME, "--remove", "--", "-std=c++11",
        "-I/Library/Developer/CommandLineTools/usr/lib/llvm-gcc/4.2.1/include",
        "-fparse-all-comments", "-c", filename
    ])


class TestDecls(unittest.TestCase):

    def setUp(self):
        self.filename = 'tests/files/decls.cpp'
        self.assertEqual(parse(self.filename), 0)

    def test_insert_decls(self):
        inserted = all("from decl order by name")
        expected = load_json('decls')['decl']
        self.assertEqual(inserted, expected)

    def test_insert_files(self):
        inserted = all("from file order by path")
        expected = load_json('decls')['file']
        self.assertEqual(inserted, expected)


class TestVirtual(unittest.TestCase):

    def setUp(self):
        self.filename = 'tests/files/virtual.cpp'
        self.assertEqual(parse(self.filename), 0)

    def test_insert_decls(self):
        inserted = all("from decl order by name")
        expected = load_json('decls')['decl']
        self.assertEqual(inserted, expected)

    def test_insert_files(self):
        inserted = all("from file order by path")
        expected = load_json('decls')['file']
        self.assertEqual(inserted, expected)


# class TestTemplates(unittest.TestCase):

#     def setUp(self):
#         self.filename = 'tests/files/templates.cpp'
#         self.assertEqual(parse(self.filename), 0)

#     def test_insert_decls(self):
#         inserted = all("from decl order by id")
#         expected = load_json('templates')['decl']
#         self.assertEqual(inserted, expected)

#     def test_insert_template_parameters(self):
#         inserted = all("from template_parameter order by id")
#         expected = load_json('templates')['template_parameter']
#         self.assertEqual(inserted, expected)

# class TestStructs(unittest.TestCase):
#     def setUp(self):
#         self.filename = 'tests/files/structs.cpp'
#         self.assertEqual(parse(self.filename), 0)

#     def test_insert(self):
#         row = first("from decl where name='S1'")
#         self.assertEqual(row['type'], 'struct')
#         self.assertGreater(row['end_line'], row['start_line'])

#         row = first("from decl where name='S2'")
#         self.assertEqual(row['brief_comment'], 'some comment')

#     def test_location(self):
#         file = first("from file where path like '%structs.cpp'")

#         decl = first("from decl where name='S1'")
#         self.assertEqual(decl['file_id'], file['id'])

#         line = lines(self.filename)[decl['start_line']-1]
#         self.assertEqual(line, 'struct S1 {')

#         line = lines(self.filename)[decl['end_line']-1]
#         self.assertEqual(line.strip(), '}; // S1')

# class TestEnums(unittest.TestCase):
#     def setUp(self):
#         self.filename = 'tests/files/enums.cpp'
#         self.assertEqual(parse(self.filename), 0)

#     def test_enum(self):
#         row = first("from decl where name='Enum1'")
#         self.assertEqual(row['type'], 'enum')
#         self.assertEqual(row['is_scoped'], 0)

#     def test_enum_class(self):
#         row = first("from decl where name='Enum2'")
#         self.assertEqual(row['type'], 'enum')
#         self.assertEqual(row['is_scoped'], 1)

#     def test_namespace(self):
#         row = first("from decl where name='ns1::Fruit'")
#         self.assertEqual(row['type'], 'enum')
#         self.assertEqual(row['is_scoped'], 1)

#         rows = all(f'from enum_field where enum_id={row["id"]} order by value')
#         self.assertEqual(rows[0]['name'], 'Apple')
#         self.assertEqual(rows[0]['value'], 2)
#         self.assertEqual(rows[0]['brief_comment'], 'Not a phone')

#         self.assertEqual(rows[1]['name'], 'Orange')
#         self.assertEqual(rows[1]['value'], 3)

# class TestFuncs(unittest.TestCase):
#     def setUp(self):
#         self.filename = 'tests/files/funcs.cpp'
#         self.assertEqual(parse(self.filename), 0)

#     def test_insert(self):
#         row = first("from func where name='func1'")
#         self.assertEqual(row['brief_comment'], 'func1')
#         self.assertEqual(row['start_line'], 2)

#     def test_multi_insert(self):
#         pass

if __name__ == '__main__':
    unittest.main()
