import subprocess
import unittest
import pprint
import json
from util import DB_NAME, all, load_json

pp = pprint.PrettyPrinter(indent=4)  # pp.pprint(dict(row))

def parse(filename: str):
    return subprocess.call([
        "./ctypefind", "--db", DB_NAME, "--truncate", "--", "-std=c++11",
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


if __name__ == '__main__':
    unittest.main()
