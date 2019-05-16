CXX	= g++
CXXFLAGS= -std=c++11 
AOCL_COMPILE_CONFIG := $(shell aocl compile-config )
AOCL_LINK_CONFIG := $(shell aocl link-config )
TARGET_DIR=bin
.PHONY : all

## GENERIC

host_%: examples/host_%.cpp $(TARGET_DIR)
	$(CXX) $(CXXFLAGS) -o $(TARGET_DIR)/host_$* $< $(AOCL_COMPILE_CONFIG) $(AOCL_LINK_CONFIG) $(LDFLAGS)


$(TARGET_DIR):
		mkdir $(TARGET_DIR)
