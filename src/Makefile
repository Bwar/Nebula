CC = gcc
CXX = g++
CFLAGS = -g -O3 -Wall -Wno-unused-function -m64 -Wl,--export-dynamic -D__GUNC__ -fPIC 
cplusplus_version=$(shell g++ -dumpversion | awk '{if ($$NF > 5.0) print "c++14"; else print "c++11";}')
CXXFLAG = -std=$(cplusplus_version) -g -O3 -Wall -Wno-unused-function -m64 -Wl,--export-dynamic -D_GNU_SOURCE=1 -D_REENTRANT -D__GUNC__ -fPIC  -DNODE_BEAT=10.0 

ifeq ($(unit_test),y)
CXXFLAG += -DUNIT_TEST
endif

ARCH:=$(shell uname -m)

ARCH32:=i686
ARCH64:=x86_64

ifeq ($(ARCH),$(ARCH64))
	SYSTEM_LIB_PATH:=/usr/lib64:/usr/local/lib64
else
	SYSTEM_LIB_PATH:=/usr/lib:/usr/local/lib
endif
LIB3RD_PATH = ../../NebulaDepend

NEBULA_PATH = ..

VPATH = $(NEBULA_PATH)/src
SUB_DIRS := $(foreach dir, $(VPATH), $(shell find $(dir) -maxdepth 5 -type d))
DIRS := $(SUB_DIRS)


INC := $(INC) \
       -I $(LIB3RD_PATH)/include \
       -I $(NEBULA_PATH)/src 


LDFLAGS := $(LDFLAGS) -D_LINUX_OS_ \
           -L$(LIB3RD_PATH)/lib -lhiredis \
           -L$(LIB3RD_PATH)/lib -lcryptopp \
           -L$(LIB3RD_PATH)/lib -lev \
           -L$(LIB3RD_PATH)/lib -lprotobuf \
           -L$(SYSTEM_LIB_PATH) -lc -lrt -ldl

SUB_INCLUDE = channel ios labor codec pb mydis logger
DEEP_SUB_INCLUDE = actor util
CPP_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cpp))
CC_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.cc))
C_SRCS = $(foreach dir, $(DIRS), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.cpp,%.o,$(CPP_SRCS)) $(patsubst %.c,%.o,$(C_SRCS)) $(patsubst %.cc,%.o,$(CC_SRCS))

TARGET = libnebula.so

all: $(TARGET)
	
libnebula.so: $(OBJS)
	$(CXX) -fPIE -rdynamic -shared -lstdc++ -g -o $@ $^ $(LDFLAGS)
	mkdir -p $(NEBULA_PATH)/include $(NEBULA_PATH)/lib
	@for dir in $(SUB_INCLUDE); \
	do \
		mkdir -p $(NEBULA_PATH)/include/$$dir; \
		for f in `ls $(NEBULA_PATH)/src/$$dir/*.*`; \
		do \
			if [[ $$f =~ ".hpp" || $$f =~ ".h" || $$f =~ ".inl" ]]; then cp $$f $(NEBULA_PATH)/include/$$dir/; fi; \
		done; \
	done
	@for d in `find $(DEEP_SUB_INCLUDE) -type d`; \
	do \
		mkdir -p $(NEBULA_PATH)/include/$$d; \
		for f in `ls $(NEBULA_PATH)/src/$$d/*.*`; \
		do \
			if [[ $$f =~ ".hpp" || $$f =~ ".h" || $$f =~ ".inl" ]]; then cp $$f $(NEBULA_PATH)/include/$$d/; fi; \
		done; \
	done
	cp -f $(NEBULA_PATH)/src/*.hpp $(NEBULA_PATH)/include/
	cp -f $@ $(NEBULA_PATH)/lib/

%.o:%.cpp
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.cc
	$(CXX) $(INC) $(CXXFLAG) -c -o $@ $< $(LDFLAGS)
%.o:%.c
	$(CC) $(INC) $(CFLAGS) -c -o $@ $< $(LDFLAGS)
clean:
	rm -f $(OBJS)
	rm -f $(TARGET)
	rm -rf $(NEBULA_PATH)/include
	rm -f $(NEBULA_PATH)/lib/libnebula.*
        
        
