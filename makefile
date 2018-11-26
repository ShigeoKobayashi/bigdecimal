DLL_SOURCES=bigdecimal.cpp
TEST_SOURCE=test.c
HEADERS=stdafx.h bigdecimal.h
OBJS=test.o bigdecimal.o

lib:
	g++ -o libbigdecimal.so -shared -fPIC -fvisibility=hidden $(DLL_SOURCES)
link:
	gcc -L./ -o test test.c -lbigdecimal
run:
	LD_LIBRARY_PATH=. ./test
