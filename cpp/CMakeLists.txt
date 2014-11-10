cmake_minimum_required(VERSION 2.8.4)
project(ac_server)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -pedantic")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O2")

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

include_directories(
  gen-cpp
  /usr/include/thrift
  /usr/local/include/thrift/
)

set(SOURCE_FILES server.cpp gen-cpp/AutoComplete.cpp gen-cpp/api_types.cpp logger.cpp acindex.cpp)
add_executable(ac_server ${SOURCE_FILES})

set(LIBS thrift boost_program_options pthread)

target_link_libraries(ac_server ${LIBS})
