CXX		= g++
GCCPLUGINS_DIR	= $(shell $(CXX) -print-file-name=plugin)
CXXFLAGS	= -I$(GCCPLUGINS_DIR)/include -shared -fPIC -O2 -fno-rtti -Wall
ENV_CXXFLAGS	= -Wl,--no-as-needed -shared -fPIC -O2 -Wall -ldl -lpthread 

# do not make changes below this line
SRC		= PolymorphicCanaries.cpp
SHARED_OBJ	= $(SRC:.cpp=.so)

# phony targets
.PHONY: all clean

all: $(SHARED_OBJ) 

# build the plugin
$(SHARED_OBJ): $(SRC) $(SRC:.cpp=.h) offsets.h
	$(CXX) $(CXXFLAGS) $(SRC) -o $@

# clean
clean:
	rm -rf $(SHARED_OBJ) 
