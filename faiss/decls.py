from typing import List


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

    def __repr__(self) -> str:
        return f"{self.kind} {self.name}[{self.ctors}] (in {self.file})"
