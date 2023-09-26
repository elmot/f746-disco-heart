cmake_minimum_required(VERSION 3.26)
project(MAX30100)
add_library(MAX30100 INTERFACE)
target_include_directories(MAX30100 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(MAX30100 PUBLIC MAX30100.cpp MAX30100.h)

