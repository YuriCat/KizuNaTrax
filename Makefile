CXX = g++
CXXFLAGS =  -std=c++1y -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-unused-function -Wno-unused-value -Wno-sign-compare -Wno-unused-variable -Wno-unused-but-set-variable

ifeq ($(OS),Windows_NT)
  # for Windows
  #LDFLAGS += -lwsock32 -lws2_32
else
  UNAME = \${shell uname}

  ifeq ($(UNAME),Linux)
    # for Linux
    LDFLAGS += -pthread
  endif

  ifeq ($(UNAME),Darwin)
    # for MacOSX
  endif
endif

ifeq ($(TARGET),release)
	CXXFLAGS += -Ofast -flto -DNDEBUG
endif
ifeq ($(TARGET),default)
	CXXFLAGS += -Ofast -g
endif
ifeq ($(TARGET),debug)
	CXXFLAGS += -O0 -g -DDEBUG -D_GLIBCXX_DEBUG
endif

sources  := lib/trax.cc lib/move.cc lib/trace.cc lib/validation.cc
output_dir := out/$(TARGET)/
objects    ?= $(sources:%.cc=$(output_dir)/%.o)
directories  ?= $(output_dir)

default release debug:
	$(MAKE) TARGET=$@ preparation trax_test kizuna_client kizuna_engine

preparation $(directories):
	mkdir -p $(directories)

kizuna_client: $(KIZUNA_OBJS)
	$(CXX) $(CXXFLAGS) -o $(output_dir)kizuna_client src/trax/client.cc $(LDFLAGS)

kizuna_engine: $(KIZUNA_OBJS)
	$(CXX) $(CXXFLAGS) -o $(output_dir)kizuna_engine src/trax/client.cc $(LDFLAGS) -DENGINE

trax_test: $(KIZUNA_OBJS)
	$(CXX) $(CXXFLAGS) -o $(output_dir)trax_test src/trax/trax_test.cc $(LDFLAGS)

clean:
	-rm -rf $(output_dir)*