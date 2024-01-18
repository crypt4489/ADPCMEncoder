objs_dir := ./objs
inc_dir := ./

# Define source files and object files
src_cpp := $(shell find . -maxdepth 1 -type f -name '*.cpp')
objs := $(patsubst ./%, $(objs_dir)/%, $(src_cpp:%.cpp=%.o))

# Define target
target := ADPCMEncoder

build_type ?= DEBUG

incflags := -I$(inc_dir)

# Compiler and flags
cxx := g++
cppflags := -std=c++17 -Wall

ifeq ($(build_type), RELEASE)
    cppflags += -O3
endif

# Build rule for object files
$(objs_dir)/%.o: %.cpp
    $(cxx) $(cppflags) $(incflags) -c -o $@ $^

.PHONY : all clean compile directories

# Build rule for the target
all: directories compile

compile:
    make $(target)

directories:
    if [ ! -d $(objs_dir) ]; then mkdir -p $(objs_dir) ; fi

$(target): $(objs)
    $(cxx) -o $@ $^

# Clean rule
clean:
    rm -rf $(objs) $(target) $(objs_dir)/*.o