LLVMCONFIG 	= llvm-config
CXXFLAGS = -g -Wall -std=c++14 $(shell $(LLVMCONFIG) --cxxflags) -fvisibility-inlines-hidden -I/Users/weidongfang/Downloads/lua-5.4.6/src
LDFLAGS = $(shell $(LLVMCONFIG) --libs --ldflags)
CLANGLIBS = -lclang\
				-lclangTooling\
				-lclangFrontendTool\
				-lclangFrontend\
				-lclangDriver\
				-lclangSerialization\
				-lclangCodeGen\
				-lclangParse\
				-lclangSema\
				-lclangStaticAnalyzerFrontend\
				-lclangStaticAnalyzerCheckers\
				-lclangStaticAnalyzerCore\
				-lclangAnalysis\
				-lclangARCMigrate\
				-lclangRewriteFrontend\
				-lclangRewrite\
				-lclangEdit\
				-lclangAST\
				-lclangASTMatchers\
				-lclangLex\
				-lclangBasic\
				-lclangSupport
LDLIBS = $(CLANGLIBS)

SOURCES = main.cpp indexer.cpp db.cpp util.cpp config.cpp

OBJECTS = $(SOURCES:.cpp=.o) sqlite3.o /Users/weidongfang/Downloads/lua-5.4.6/src/liblua.a

all: $(OBJECTS)
	$(CXX) -otypefind $^ $(LDFLAGS) $(LDLIBS)

%: %.o
	$(CXX) -o $@ $< $(CXXFLAGS)

sqlite3.o: deps/sqlite/sqlite3.c
	cc -c -O2 -o $@ $<

clean:
	rm -f *.o typefind
