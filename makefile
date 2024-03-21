DLL_SOURCES=bigdecimal.cpp
TEST_SOURCES=vpc.c reader.c parser.c setting.c calculator.c
HEADERS=vpc.h bigdecimal.h
lib:
	g++ -o libbigdecimal.so -shared -fPIC -fvisibility=hidden $(DLL_SOURCES)
link:
	gcc -L./ -o vpc $(TEST_SOURCES) -lbigdecimal
run:
	LD_LIBRARY_PATH=. ./vpc
