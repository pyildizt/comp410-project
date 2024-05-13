# Compiler
CXX = g++

# Directories
DIR_INCLUDE = ../include

# Compiler flags
CXXFLAGS = -g -Wall -Wextra -std=c++11 $(shell pkg-config --cflags-only-other glfw3 glew)

# Include directories
INCLUDES = -I$(DIR_INCLUDE) $(shell pkg-config --cflags-only-I glfw3 glew)
# Include MACOS frameworks
FRAMEWORKS = -framework OpenGL -framework Cocoa -framework IOKit
# Libraries
LIBS = $(shell pkg-config --libs glfw3 glew)

# Source files
SRCS = main.cpp InitShader.cpp

# Object files
OBJS := $(SRCS:.cpp=.o)

# Executable name
EXEC := scene_builder

.PHONY: all clean

# Default rule
all: $(EXEC)

# Linking
$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(EXEC) $(LIBS) $(FRAMEWORKS)
	rm $(OBJS)

# Compiling
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(EXEC)
