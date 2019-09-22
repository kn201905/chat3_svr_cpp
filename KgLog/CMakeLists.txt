cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall -pthread")

set(MY_BOOST_DIR /home/boost/1_71_0)
set(MY_BOOST_LIB_DIR ${MY_BOOST_DIR}/lib)
include_directories(${MY_BOOST_DIR}/include)

# add_executable(glog main.cpp src/KgLog.cpp)
add_library(_KgLog STATIC src/KgLog.cpp) 
