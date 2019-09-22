
cmake_minimum_required(VERSION 3.0)

project(glog_toTxt CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")
add_executable(glog_toTxt main.cpp "../__common/_KFile_W.cpp")
