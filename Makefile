SOURCES := $(wildcard *.cpp)
OBJFILES := $(patsubst %.cpp, %.o, $(SOURCES))

all: djhero


%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -g -I ../pc_simulator -o $@ $<

djhero: $(OBJFILES)
	$(CXX) -o $@ $(LDFLAGS) $(OBJFILES) -lm -llvgl -lSDL2 -lboost_filesystem -lboost_system -L../pc_simulator -lgd

