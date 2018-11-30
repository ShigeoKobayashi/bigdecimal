DLL_SOURCES=bigdecimal.cpp
TEST_SOURCE=vpc.c
HEADERS=stdafx.h bigdecimal.h
OBJS=test.o bigdecimal.o

lib:
	g++ -o libbigdecimal.so -shared -fPIC -fvisibility=hidden $(DLL_SOURCES)
link:
	gcc -L./ -o vpc vpc.c -lbigdecimal
run:
	LD_LIBRARY_PATH=. ./vpc
