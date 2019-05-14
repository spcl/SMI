#SPCL Makefile
#compiling host and version 1
CXX	= g++
AOCXX	= aoc
CXXFLAGS= -std=c++11 -g
LDFLAGS = 
BOARD=p520_max_sg280l
AOCXXFLAGS  = -march=emulator -v -board=$(BOARD) -emulator-channel-depth-model=strict
# OpenCL compile and link flags.
AOCL_COMPILE_CONFIG := $(shell aocl compile-config )
AOCL_LINK_CONFIG := $(shell aocl link-config )
TARGET_DIR=bin


host: nw.cpp 
	$(CXX) $(CXXFLAGS) -o host_nw $< $(AOCL_COMPILE_CONFIG) $(AOCL_LINK_CONFIG) $(LDFLAGS)

v1_emulator: nw_kernel_v1.cl 
	$(AOCXX) $(AOCXXFLAGS) -o $(TARGET_DIR)/v1.aocx $<

v1_report: nw_kernel_v1
	$(AOCXX) -board=$(BOARD) -rtl $< -report -no-interleaving=default -fp-relaxed -fpc
