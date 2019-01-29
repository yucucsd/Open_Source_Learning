#If no project is specified in make, the program will construct the first project with no wildcard character in its target

#font color: reset = 0, black = 30, red = 31, green = 32, yellow = 33, blue = 34, purple = 35, sky-blue = 36, white = 37
#background color: reset = 0, black = 40, red = 41, green = 42, yellow = 43, blue = 44, purple = 45, sky-blue = 46, white = 47
#\033 or \e is escape character, [ stands for the start of color, and m stands for the end, 
#highlight: 1, underline: 4, blink: 5
CCCOLOR = "\033[34m"
ENDCOLOR = "\033[0m"
SRCCOLOR = "\033[33m"
BINCOLOR = "\033[37;1m"
LINKCOLOR = "\033[34;1m"

#$(CC) = gcc cannot link c++ std library, $(CXX) = g++, both gcc and g++ can compile cpp file
#in fact g++ also use gcc to compile cpp files
#0: stdin, 1: stdout, 2: stderr
#1>&2(>&2) redirect stdout to stderr
#(May be) ; seperate the later g++ from this line
QUIET_CC = @printf '    %b %b\n' $(CCCOLOR)CC$(ENDCOLOR) $(SRCCOLOR)$@$(ENDCOLOR) 1>&2;
QUIET_LINK = @printf '    %b %b\n' $(LINKCOLOR)LINK$(ENDCOLOR) $(BINCOLOR)$@$(ENDCOLOR) 1>&2;
REDIS_CC =  $(QUIET_CC)$(CXX)
REDIS_LD = $(QUIET_LINK)$(CXX)
REDIS_SERVER_NAME = redis-server
REDIS_SERVER_OBJ = server.o memtest.o util.o redisserver.o

#redis-server
$(REDIS_SERVER_NAME): $(REDIS_SERVER_OBJ)
	$(REDIS_LD) -o $@ $^  # $@ is the target, $^ is all dependency

#make will generate .o file based on .cpp automatically, if this dependency is not stated
#However, in this way, make cannot track the modification in .h files 
%.o : %.cpp *.h
	$(REDIS_CC) -c $< # $< is the first dependency -c means does not link
clean :
	rm -rf $(REDIS_SERVER_NAME) *.o
