cmake_minimum_required(VERSION 3.4)
project(urlparser)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp UrlParser.cpp UrlParser.h)
add_executable(urlparser ${SOURCE_FILES})