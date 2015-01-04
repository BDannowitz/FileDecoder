PNAME = decode
OBJS = Uploader.o
CC = g++ -std=c++11
DEBUG = -g
#Include $(DEBUG) in CFLAGS and LFLAGS to debug
CFLAGS = -Wall -c -l mysqlcppconn 
LFLAGS = -Wall -l mysqlcppconn 

all: main.cpp $(OBJS)
	$(CC) $(LFLAGS) main.cpp $(OBJS) -o $(PNAME) 

Uploader.o: Uploader.h Uploader.cpp
	$(CC) $(CFLAGS) Uploader.cpp 

clean:
	rm *.o $(PNAME)
