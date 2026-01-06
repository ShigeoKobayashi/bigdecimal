LIB_SOURCES=bigdecimal.cpp
LIB_HEADERS=bigdecimal.h
VPC_SOURCES=vpc.c reader.c parser.c setting.c calculator.c
VPC_HEADERS=vpc.h
lib:
	g++ -o libbigdecimal.so -shared -fPIC -fvisibility=hidden $(LIB_SOURCES)
vpc:
	gcc -L./ -o vpc $(VPC_SOURCES) -lbigdecimal
run:
	LD_LIBRARY_PATH=. ./vpc