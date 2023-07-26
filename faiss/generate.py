import sqlite3
import re
from jinja2 import Environment, FileSystemLoader

from util import find_decl, find_descendent_decls, remove_empty_lines
from decls import Struct, StructField


def format_include_file(filename):
    filename = re.sub(r"^.+?/faiss", "faiss", filename)
    return f"<{filename}>"

def to_index_type_name(orginal_name):
    return re.sub(r"^.+?::Index", "", orginal_name)

class Generator:
    db: sqlite3.Connection

    def __init__(self, db) -> None:
        self.db = db

    def generate(self, class_names, header_file, impl_file):
        decl_ids = [
            find_decl(self.db, name=class_name).id for class_name in class_names]
        decls = find_descendent_decls(self.db, decl_ids)
        header_includes = set()
        impl_includes = set()
        index_types = set()
        structs = []
        decls = sorted(decls, key=lambda x: to_index_type_name(x.name))
        for decl in decls:
            if not decl.is_index():
                continue
            impl_includes.add(format_include_file(decl.file))
            decl_name = to_index_type_name(decl.name)
            index_type = decl_name  # .upper()
            index_types.add(index_type)
            struct = Struct(decl_name, index_type)
            for ctor in decl.ctors:
                if (len(ctor.args) > 0):
                    for arg in ctor.args:
                        if arg.type_file:
                            header_includes.add(
                                format_include_file(arg.type_file))
                        field = StructField(arg.type_name, arg.name)
                        struct.fields.append(field)
                    break
            structs.append(struct)

        environment = Environment(loader=FileSystemLoader("faiss/"))
        template = environment.get_template("index_config.h.txt")
        context = {
            "includes": header_includes,
            "index_types": index_types,
            "structs": structs,
        }

        with open(header_file, mode="w", encoding="utf-8") as results:
            results.write(template.render(context))
        remove_empty_lines(header_file)


if __name__ == '__main__':
    connection = sqlite3.connect('faisstypes.db')
    generator = Generator(connection)
    generator.generate(["faiss::Index", "faiss::IndexBinary"],
                       "index_config.h", "index_config.cpp")
    connection.close()
