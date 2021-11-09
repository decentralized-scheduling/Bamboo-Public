CC=g++
CFLAGS=-Wall -g -std=c++0x -fno-omit-frame-pointer

.SUFFIXES: .o .cpp .h

SRC_DIRS = ./ ./benchmarks/ ./concurrency_control/ ./storage/ ./system/
INCLUDE = -I. -I./benchmarks -I./concurrency_control -I./storage -I./system -I./qcc

CFLAGS += $(INCLUDE) -D NOGRAPHITE=1 -O3 -Wno-unused-variable -Wno-class-memaccess -march=native #-Werror
LDFLAGS = -Wall -L. -pthread -g -lrt -std=c++0x -O3 -ljemalloc
LDFLAGS += $(CFLAGS)

ifneq ($(QCC_WORKERS),)
CFLAGS += -DQCC_MAX_WORKERS_OVERRIDE=$(QCC_WORKERS)
endif

ifneq ($(QCC_MAX_ACCESSES),)
CFLAGS += -DQCC_MAX_ACCESSES_OVERRIDE=$(QCC_MAX_ACCESSES)
endif

CPPS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)*.cpp))
OBJS = $(CPPS:.cpp=.o)
DEPS = $(CPPS:.cpp=.d)

CS = ./qcc/qcc_lib_cpp.c ./qcc/qcc.c
OBJS += $(CS:.c=.o)
DEPS += $(CS:.c=.d)

all:rundb

rundb : $(OBJS)
	$(CC) -no-pie -o $@ $^ $(LDFLAGS)
	#$(CC) -o $@ $^ $(LDFLAGS)

-include $(OBJS:%.o=%.d)

%.d: %.cpp
	$(CC) -MM -MT $*.o -MF $@ $(CFLAGS) $<

%.d: %.c
	$(CC) -MM -MT $*.o -MF $@ $(CFLAGS) $<

%.o: %.cpp
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

.SILENT: clean
.PHONY: clean
clean:
	rm -f rundb $(OBJS) $(DEPS)
