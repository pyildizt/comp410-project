# Compiler
CXX := g++

# Directories
SRCDIR := .
UTILDIR := ../util
ZAXDIR := ../extern/zax-parser/src
INCDIR := ../include
IMGUI_DIR := ../extern/imgui-1.90.5

# Compiler flags
CXXFLAGS := -std=c++11 -g $(shell pkg-config --cflags-only-other glfw3 glew libpng)

# Include directories
INCLUDES = -I$(INCDIR) -I$(UTILDIR) -I$(ZAXDIR) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends $(shell pkg-config --cflags-only-I glfw3 glew libpng)

# Include MACOS frameworks
FRAMEWORKS = -framework OpenGL -framework Cocoa -framework IOKit

# Libraries
LIBS = $(shell pkg-config --libs glfw3 glew libpng)

# Source files
SRCS := $(wildcard $(SRCDIR)/*.cpp) $(wildcard $(UTILDIR)/*.cpp) $(wildcard $(ZAXDIR)/*.cpp)
SRCS += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SRCS += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

# Object files
OBJS := $(SRCS:.cpp=.o)

# Executable name
EXEC := scene_builder

.PHONY: all clean

# Default target
all: $(EXEC)

# Linking
$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS) $(FRAMEWORKS)

# Compiling
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean
clean:
	rm -f $(OBJS) $(EXEC)
