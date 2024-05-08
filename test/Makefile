# Compiler
CXX := g++

# Directories
SRCDIR := .
UTILDIR := ../util
ZAXDIR := ../extern/zax-parser/src
INCDIR := ../include

# Compiler flags
CXXFLAGS := -std=c++11 -I$(INCDIR) -I$(UTILDIR) -I$(ZAXDIR) -ggdb

# Source files
SRCS := $(wildcard $(SRCDIR)/*.cpp) $(wildcard $(UTILDIR)/*.cpp) $(wildcard $(ZAXDIR)/*.cpp)

# Object files
OBJS := $(SRCS:.cpp=.o)

# Executable name
EXEC := my_program

.PHONY: all clean

# Default target
all: $(EXEC)

# Linking
$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Compiling
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(EXEC)
