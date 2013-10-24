TARGET = mcpat
SHELL = /bin/sh
.PHONY: all depend clean
.SUFFIXES: .cpp .o

ifndef NTHREADS
  NTHREADS = 4
endif


LIBS = 
INCS = -lm

ifeq ($(TAG),dbg)
  DBG = -Wall 
  OPT = -ggdb -g -O0 -DNTHREADS=1
else
  DBG = 
  OPT = -O3 -msse2 -mfpmath=sse -DNTHREADS=$(NTHREADS)
  #OPT = -O0 -DNTHREADS=$(NTHREADS)
endif

#CXXFLAGS = -Wall -Wno-unknown-pragmas -Winline $(DBG) $(OPT) 
CXXFLAGS = -Wno-unknown-pragmas $(DBG) $(OPT) -DCONFIG_STANDALONE 
CXX = g++ -m32
CC  = gcc -m32

SRCS  = \
  Ucache.cpp \
  XML_Parse.cpp \
  arbiter.cpp \
  area.cpp \
  array.cpp \
  bank.cpp \
  basic_circuit.cpp \
  cacti_interface.cpp \
  component.cpp \
  core.cpp \
  crossbar.cpp \
  decoder.cpp \
  htree2.cpp \
  interconnect.cpp \
  io.cpp \
  logic.cpp \
  main.cpp \
  mat.cpp \
  memoryctrl.cpp \
  noc.cpp \
  nuca.cpp \
  parameter.cpp \
  processor.cpp \
  router.cpp \
  sharedcache.cpp \
  subarray.cpp \
  technology.cpp \
  uca.cpp \
  wire.cpp \
  xmlParser.cpp 

OBJS = $(patsubst %.cpp,obj_$(TAG)/%.o,$(SRCS))

all: obj_$(TAG)/$(TARGET)
	cp -f obj_$(TAG)/$(TARGET) $(TARGET)

obj_$(TAG)/$(TARGET) : $(OBJS)
	$(CXX) $(OBJS) -o $@ $(INCS) $(CXXFLAGS) $(LIBS) -pthread

#obj_$(TAG)/%.o : %.cpp
#	$(CXX) -c $(CXXFLAGS) $(INCS) -o $@ $<

obj_$(TAG)/%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-rm -f *.o $(TARGET)


