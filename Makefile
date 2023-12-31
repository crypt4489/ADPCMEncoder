
OBJS_DIR := ./objs
INC_DIR := ./

# Define source files and object files
SRC_CPP := $(shell find . -type f -name '*.cpp')
objs := $(patsubst ./%, $(OBJS_DIR)/%, $(SRC_CPP:%.cpp=%.o))

# Define target
target := ADPCMEncoder

incflags := -I$(INC_DIR)

# Compiler and flags
CXX := g++
cppflags := -std=c++17 -Wall

# Build rule for object files
$(OBJS_DIR)/%.o: %.cpp
	$(CXX) $(cppflags) $(incflags) -c -o $@ $^


.PHONY : all clean compile directories

# Build rule for the target
all: directories compile

compile:
	make $(target)

directories:
	if [ ! -d $(OBJS_DIR) ]; then mkdir -p $(OBJS_DIR) ; fi

$(target): $(objs)
	$(CXX) -o $@ $^

# Clean rule
clean:
	rm -rf $(objs) $(target) $(OBJS_DIR)/*.o