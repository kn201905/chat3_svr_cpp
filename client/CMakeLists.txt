# set(CMAKE_VERBOSE_MAKEFILE 1)
cmake_minimum_required(VERSION 3.0)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
# project(chattest)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")

set(MY_BOOST_DIR /home/boost/1_71_0)
set(MY_BOOST_LIB_DIR ${MY_BOOST_DIR}/lib)
include_directories(${MY_BOOST_DIR}/include)
# link_directories(dir)

# add_subdirectory(dir_sub)
# dir_sub 内では  add_library(foo STATIC sub1.cpp sub2.cpp) のようにする

set(MY_SOURCES client.cpp)
add_executable(client ${MY_SOURCES})

target_link_libraries(
	client
	${MY_BOOST_LIB_DIR}/libboost_date_time.a	# asio
	${MY_BOOST_LIB_DIR}/libboost_regex.a		# asio
	${MY_BOOST_LIB_DIR}/libboost_system.a		# asio
#	${MY_BOOST_LIB_DIR}/libboost_chrono.a
#	${MY_BOOST_LIB_DIR}/libboost_thread.a
)
