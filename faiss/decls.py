from typing import List
import re

class Argument:
    name: str
    type_name: str
    type_file: str
    default_value: str

    def __repr__(self) -> str:
        default_value = f"={self.default_value}" if self.default_value else ""
        return f"{self.type_name} {self.name}{default_value} (in {self.type_file})"


class Function:
    id: int
    name: str
    args: List[Argument]

    def __repr__(self) -> str:
        return f"{self.name}({self.args})"


class Decl:
    id: int
    kind: str
    name: str
    comment: str
    is_abstract: bool

    ctors: List[Function]
    file: str

    def is_index(self):
        for ctor in self.ctors:
            for arg in ctor.args:
                if re.search(r"(Quantizer|Index|Binary)$", arg.type_name):
                    return False
        return not re.search(r"(Quantizer|Scan)$", self.name)

    def __repr__(self) -> str:
        return f"{self.kind} {self.name}[{self.ctors}] (in {self.file})"

class StructField:
    data_type: str
    name: str

    def __init__(self, data_type, name) -> None:
        self.data_type =data_type
        self.name = name

class Struct:
    name: str
    index_type: str
    fields: List[StructField]

    def __init__(self, name, index_type) -> None:
        self.name = name
        self.index_type = index_type
        self.fields = []
