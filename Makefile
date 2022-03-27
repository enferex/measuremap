CXXFLAGS = -O0 -pedantic --std=c++20 -Wall
SRCS = main.cc
OBJS = $(SRCS:.cc=.o)

all: timer

timer: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -lLLVM -o $@

clean:
	$(RM) -v $(OBJS) timer
