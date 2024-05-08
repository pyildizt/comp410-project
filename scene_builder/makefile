## Makefile for MacOS shared by instructor

# Compiler
CPP = g++

# Directories
DIR_INCLUDE = ../include
DIR_SRC = ../src

# Compiler flags
CPP_FLAGS = -g -Wall -Wextra -std=c++11 $(shell pkg-config --cflags-only-other glfw3 glew)

# Include directories
INCLUDES = -I$(DIR_INCLUDE) $(shell pkg-config --cflags-only-I glfw3 glew)

# Include MACOS frameworks
FRAMEWORKS = -framework OpenGL -framework Cocoa -framework IOKit

# Libraries
LIBS = $(shell pkg-config --libs glfw3 glew)

# Source files
SRCS = main.cpp InitShader.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Name of the program
TARGET = main

# Default rule
all: $(TARGET)

# Rule to compile .cpp files to object files
%.o: %.cpp
	$(CPP) $(CPP_FLAGS) $(INCLUDES) -c $< -o $@

# Rule to link object files into the executable
$(TARGET): $(OBJS)
	$(CPP) $(CPP_FLAGS) $(OBJS) -o $(TARGET) $(LIBS) $(FRAMEWORKS)
	rm $(OBJS)

# Run rule
run:
	./$(TARGET)

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)
