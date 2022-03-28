CXXFLAGS = -O0 -pedantic --std=c++20 -Wall
SRCS = main.cc
OBJS = $(SRCS:.cc=.o)
APP = measuremap

all: $(APP)

$(APP): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -lLLVM -o $@

clean:
	$(RM) -v $(OBJS) $(APP)
