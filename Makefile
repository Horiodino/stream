CXX=g++
CXXFLAGS=-Wall -Wextra -std=c++11
LDFLAGS= -lX11 -lXinerama -lXext -lstdc++ -lXcomposite 

TARGET=obs-capture
SRCS=capture.cpp

$(TARGET): $(SRCS)
INCLUDES=-I./vcpkg/installed/x64-linux/include

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
