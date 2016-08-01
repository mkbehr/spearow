cmake_minimum_required(VERSION 3.1)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "DEBUG")
  #set(CMAKE_BUILD_TYPE "RELEASE")
  #set(CMAKE_BUILD_TYPE "RELWITHDEBINFO")
  #set(CMAKE_BUILD_TYPE "MINSIZEREL")
endif()

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 14)

add_compile_options("-O3")

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
  add_compile_options("-g")
endif()

add_executable(cpu-test cpu-test.cpp)

add_library(mem mem.cpp)

add_library(cpu cpu.cpp)

add_library(opcodes opcodes.cpp)

add_executable(spearow spearow.cpp cpu opcodes mem)
