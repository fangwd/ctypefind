typefind finds types, functions and methods in C++ files.

typefind was initially written to generate C++ code to build/destroy [faiss](https://github.com/facebookresearch/faiss)
index objects.

TODO:

- Complete tests (not all files in the tests directory are used).

## Building on Mac

```
brew install llvm
cd typefind
make
brew install libomp
./typefind --db faisstypes.db -- -std=c++14 -c faiss/faiss.cpp -I. -I/Library/Developer/CommandLineTools/usr/lib/llvm-gcc/4.2.1/include -I/opt/homebrew/opt/libomp/include -I/usr/local/include
```

Note after brew install:
```
export PATH="$(brew --prefix llvm)/bin:$(brew --prefix bison)/bin:$PATH"
export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"
```

## Building in Docker

```
docker build -t llvm:15 .
docker run -dit llvm:15
docker exec -it 260282d9900e bash

# clean up
docker rm -f 260282d9900e
docker image rm llvm:15
```

## Build using a locally-built LLVM

# Getting Types

## Prepare LLVM

### By dowloading a release

https://github.com/llvm/llvm-project/releases/tag/llvmorg-15.0.2

(`mv clang+llvm-15.0.2-x86_64-apple-darwin/ clang-llvm` if needed)

### By building from source
https://clang.llvm.org/get_started.html
```
git clone --depth=1 https://github.com/llvm/llvm-project.git
cd llvm-project
mkdir build
cd build
cmake -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles" ../llvm
make
make install
```

Include all header files in a single c++ file:

```
import glob
with open('faiss.cpp', 'w') as f:
  f.write('\n'.join(list(map(lambda s: f'#include "{s}"', glob.glob('faiss/*.h')))) + '\nint main(){}')
```

### Development

vscode
```
"C_Cpp.clang_format_fallbackStyle": "{ BasedOnStyle: Google, IndentWidth: 4, ColumnLimit: 100 }",
````

To make the API getRawCommentForDeclNoCache return normal comments like // or /* you need to provide option "-fparse-all-comments" while invoking clang.
 