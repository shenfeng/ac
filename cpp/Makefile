GEN_SRC := server.cpp gen-cpp/AutoComplete.cpp gen-cpp/api_types.cpp logger.cpp
GEN_OBJ := $(patsubst %.cpp,%.o, $(GEN_SRC))

INCLUDE =-I gen-cpp -I /usr/include/thrift -I /usr/local/include/thrift/
CPPFLAGS   = -Wall -pedantic -O3 -c -std=c++0x -DNDEBUG


gen: api.thrift
	rm -rf gen-cpp
	thrift -gen cpp api.thrift
  # rm gen-cpp/AutoComplete_server.skeleton.cpp

share:
	g++ -c -fPIC lib_ac.cpp -o libac.o -std=c++0x
	g++ -shared -Wl -o libac.so libac.o
	rm libac.o

# INC := -I$(THRIFT_DIR) -I$(BOOST_DIR)

.PHONY: all clean

all: ac_server

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c $< -o $@ ${INCLUDE}

ac_server: $(GEN_OBJ)
	$(CXX) $^ -o $@ -L/usr/local/lib -lthrift -lboost_program_options -lpthread

clean:
	$(RM) *.o ac_server
