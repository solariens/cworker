.SUFFIXES:.o .cpp
CC=g++
SRCS=webserver.cpp
OBJS=$(SRCS:.cpp=.o)
EXEC=main
start: $(OBJS)
	$(CC) -o $(EXEC) $(OBJS)
	@echo '---OK----'
.cpp.o:
	$(CC) -o $@ -c $<
clean:
	rm -rf core.*
	rm -rf $(OBJS)
	rm -rf $(EXEC)
