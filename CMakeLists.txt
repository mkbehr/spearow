cmake_minimum_required(VERSION 3.1)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "DEBUG")
  #set(CMAKE_BUILD_TYPE "RELEASE")
  #set(CMAKE_BUILD_TYPE "RELWITHDEBINFO")
  #set(CMAKE_BUILD_TYPE "MINSIZEREL")
endif()

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 14)

add_executable(cpu-test cpu-test.cpp)
