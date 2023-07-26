import sqlite3
import re
from jinja2 import Environment, FileSystemLoader

from util import find_decl, find_descendent_decls, remove_empty_lines
from decls import Struct, StructField, DefaultValue


def format_include_file(filename):
    filename = re.sub(r"^.+?/faiss", "faiss", filename)
    return f"<{filename}>"


def to_index_type_name(orginal_name):
    return re.sub(r"^.+?::Index", "", orginal_name)

def fix_cpp_type_name(s):
    return 'bool' if s == '_Bool' else s

class Generator:
    db: sqlite3.Connection

    def __init__(self, db) -> None:
        self.db = db

    def generate(self, class_names, header_file, manager_file):
        decl_ids = [
            find_decl(self.db, name=class_name).id for class_name in class_names]
        decls = find_descendent_decls(self.db, decl_ids)
        header_includes = set()
        impl_includes = set()
        index_types = set()
        structs = []
        decls = sorted(decls, key=lambda x: to_index_type_name(x.name))
        for decl in decls:
            if not decl.is_index() or decl.is_abstract:
                continue
            impl_includes.add(format_include_file(decl.file))
            name = to_index_type_name(decl.name)
            index_type = name  # .upper()
            index_types.add(index_type)
            struct = Struct(name, decl.name, index_type)
            for ctor in decl.ctors:
                if (len(ctor.args) > 0):
                    for arg in ctor.args:
                        if arg.type_file:
                            header_includes.add(
                                format_include_file(arg.type_file))
                        field = StructField(arg.type_name, arg.name)
                        struct.fields.append(field)
                        if arg.default_value:
                            struct.default_values.append(
                                DefaultValue(arg.name, arg.default_value))
                    break
            structs.append(struct)

        environment = Environment(loader=FileSystemLoader("faiss/"))
        environment.globals['fix_cpp_type_name'] = fix_cpp_type_name

        template = environment.get_template("index_config.h.txt")
        context = {
            "includes": header_includes,
            "index_types": index_types,
            "structs": structs,
        }

        with open(header_file, mode="w", encoding="utf-8") as results:
            results.write(template.render(context))
        remove_empty_lines(header_file)

        template = environment.get_template("index_manager.cpp.txt")
        context = {
            "includes": impl_includes,
            "structs": structs,
        }

        with open(manager_file, mode="w", encoding="utf-8") as results:
            results.write(template.render(context))

        remove_empty_lines(header_file)


if __name__ == '__main__':
    connection = sqlite3.connect('faisstypes.db')
    generator = Generator(connection)
    generator.generate(["faiss::Index"],  # "faiss::IndexBinary"
                       "faiss/index_config.h", "faiss/index_manager.cpp")
    connection.close()
