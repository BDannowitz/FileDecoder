PNAME = decode
OBJS = Uploader.o
CC = g++ -std=c++11
DEBUG = -g
#Include $(DEBUG) in CFLAGS and LFLAGS to debug
CFLAGS = -Wall -c -lmysqlcppconn
LFLAGS = -Wall -lmysqlcppconn
INCLUDE = -I/usr/include/cppconn
LIB = -L/usr/lib 

all: main.cpp $(OBJS)
	$(CC) $(INCLUDE) $(LIB) main.cpp $(OBJS) -o $(PNAME) $(LFLAGS) 

Uploader.o: Uploader.h Uploader.cpp
	$(CC)  $(INCLUDE) $(LIB) Uploader.cpp $(CFLAGS)


clean:
	rm *.o $(PNAME)
