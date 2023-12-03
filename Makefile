CXXFLAGS = -g -Wall -std=c++14 $(shell llvm-config --cxxflags) -fvisibility-inlines-hidden
LDFLAGS = $(shell llvm-config --ldflags)
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
LDLIBS = $(CLANGLIBS) $(shell llvm-config --libs) $(shell llvm-config --system-libs)

SOURCES = main.cpp indexer.cpp db.cpp util.cpp config.cpp

OBJECTS = $(SOURCES:.cpp=.o) sqlite3.o

all: $(OBJECTS)
	$(CXX) -octypefind $^ $(LDFLAGS) $(LDLIBS)

%: %.o
	$(CXX) -o $@ $< $(CXXFLAGS)

sqlite3.o: deps/sqlite/sqlite3.c
	cc -c -O2 -o $@ $<

clean:
	rm -f *.o ctypefind
