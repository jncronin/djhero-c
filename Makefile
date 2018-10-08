SOURCES := $(wildcard *.cpp)
OBJFILES := $(patsubst %.cpp, %.o, $(SOURCES))

all: djhero


%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) -g `pkg-config --cflags gstreamer-1.0` -I ../lv -o $@ $<

djhero: $(OBJFILES)
	$(CXX) -o $@ $(LDFLAGS) $(OBJFILES) -lm -llvgl -lboost_filesystem -lboost_system -L../lv -lgd -levdev `pkg-config --libs gstreamer-1.0`

