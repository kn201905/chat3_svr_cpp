
cmake_minimum_required(VERSION 3.0)

project(glog_toTxt CXX)

set(MY_CMN_DIR /home/user-k/chatsvr/__common)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")
add_executable(glog_toTxt main.cpp ${MY_CMN_DIR}/_KFile_W.cpp )
