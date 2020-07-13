G = g++
FLAGS = -lpthread -lsqlite3
OBJS = douyu_danmu.o test_douyu.o

test_douyu:$(OBJS)
	$(G) $(FLAGS) $(OBJS) -o test_douyu

douyu_danmu.o:douyu_danmu.hpp  douyu_danmu.cpp
	$(G) -c douyu_danmu.cpp -o douyu_danmu.o

test_douyu.o:test_douyu.cpp
	$(G) test_douyu.cpp -c -o test_douyu.o

clean:
	rm -rf *.o test_douyu
