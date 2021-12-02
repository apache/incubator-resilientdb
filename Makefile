CC=g++
CFLAGS=-Wall -g -gdwarf-3 -std=c++11 -rdynamic #
JEMALLOC=deps/jemalloc-5.1.0
NNMSG=deps/nng-1.3.2
BOOST=deps/boost_1_67_0
CRYPTOPP=deps/crypto
SQLITE=deps/sqlite-autoconf-3290000/build
RABBITMQ_C=deps/rabbitmq-c
AMQP_CLIENT=deps/SimpleAmqpClient
OPENSSL=deps/openssl-3.0.0

.SUFFIXES: .o .cpp .h

SRC_DIRS = ./ ./benchmarks/ ./client/ ./transport/ ./system/ ./statistics/ ./blockchain/ ./db/ ./smart_contracts/ ./data_structures
DEPS = -I. -I./benchmarks -I./client/ -I./transport -I./system -I./statistics -I./blockchain -I./smart_contracts -I./data_structures  -I$(JEMALLOC)/include -I$(NNMSG)/include -I$(BOOST) -I$(CRYPTOPP) -I./db -I$(SQLITE)/include -I$(RABBITMQ_C)/include -I$(AMQP_CLIENT)/src

CFLAGS += $(DEPS) -D NOGRAPHITE=1 -Werror -Wno-sizeof-pointer-memaccess
LDFLAGS = -Wall -L. -L$(NNMSG)/lib -L$(JEMALLOC)/lib -Wl,-rpath,$(JEMALLOC)/lib -pthread -gdwarf-3 -lrt -std=c++0x -L$(CRYPTOPP) -L$(SQLITE)/lib -L$(OPENSSL) -L$(RABBITMQ_C)/build/librabbitmq -L$(AMQP_CLIENT)/lib
LDFLAGS += $(CFLAGS)
LIBS = -lnng -lanl -ljemalloc -lcryptopp -lsqlite3 -ldl -lrabbitmq -lSimpleAmqpClient

DB_MAINS = ./client/client_main.cpp  ./unit_tests/unit_main.cpp
CL_MAINS = ./system/main.cpp ./unit_tests/unit_main.cpp
UNIT_MAINS = ./system/main.cpp ./client/client_main.cpp 

CPPS_DB = $(foreach dir,$(SRC_DIRS),$(filter-out $(DB_MAINS), $(wildcard $(dir)*.cpp))) 
CPPS_CL = $(foreach dir,$(SRC_DIRS),$(filter-out $(CL_MAINS), $(wildcard $(dir)*.cpp))) 
CPPS_UNIT = $(foreach dir,$(SRC_DIRS),$(filter-out $(UNIT_MAINS), $(wildcard $(dir)*.cpp))) 

#CPPS = $(wildcard *.cpp)
OBJS_DB = $(addprefix obj/, $(notdir $(CPPS_DB:.cpp=.o)))
OBJS_CL = $(addprefix obj/, $(notdir $(CPPS_CL:.cpp=.o)))
OBJS_UNIT = $(addprefix obj/, $(notdir $(CPPS_UNIT:.cpp=.o)))

#NOGRAPHITE=1

all:rundb runcl 


.PHONY: deps_db
deps:$(CPPS_DB)
	$(CC) $(CFLAGS) -MM $^ > obj/deps
	sed '/^[^ ]/s/^/obj\//g' obj/deps > obj/deps.tmp
	mv obj/deps.tmp obj/deps
-include obj/deps

rundb : $(OBJS_DB)
	$(CC) -static -o $@ $^ $(LDFLAGS) $(LIBS)
./obj/%.o: transport/%.cpp
	$(CC) -static -c $(CFLAGS) $(INCLUDE) $(LIBS) -o $@ $<
./obj/%.o: benchmarks/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: blockchain/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: system/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: statistics/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: client/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: %.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: db/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: smart_contracts/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<

runcl : $(OBJS_CL)
	$(CC) -static -o $@ $^ $(LDFLAGS) $(LIBS)

./obj/%.o: transport/%.cpp
	$(CC) -static -c $(CFLAGS) $(INCLUDE) $(LIBS) -o $@ $<
./obj/%.o: benchmarks/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: blockchain/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: system/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: statistics/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: client/%.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<
./obj/%.o: %.cpp
	$(CC) -c $(CFLAGS) $(INCLUDE) -o $@ $<

.PHONY: clean
clean:
	rm -f obj/*.o obj/.depend rundb runcl runsq unit_test
