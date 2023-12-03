ctypefind finds types, functions, and variable declarations/references in C++ code and writes them to a SQL database. 

ctypefind is able to find many important metadata in a C++ code base. Such as:
- Struct, class, enum, union and function declarations
- Class hierarchies
- Class/Function templates and template parameters
- Class members and their references
- Variable declarations and their references

## Building

To build on MacOS:
```
$ brew install llvm
$ make
```

## Usage
```
$ ctypefind [Options] -- [Compiler Options] <filename>
```
For example:
```
./ctypefind --db example.db -- -std=c++17 -c example.cpp -I/usr/local/include
```
