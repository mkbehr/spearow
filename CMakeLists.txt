cmake_minimum_required(VERSION 3.1)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "DEBUG")
  #set(CMAKE_BUILD_TYPE "RELEASE")
  #set(CMAKE_BUILD_TYPE "RELWITHDEBINFO")
  #set(CMAKE_BUILD_TYPE "MINSIZEREL")
endif()

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 14)

add_compile_options("-O3" "-Wall")

if (CMAKE_BUILD_TYPE MATCHES DEBUG)
  add_compile_options("-ggdb")
endif()

# TODO make this less platform-specific, limit gperftools linking to
# debug build
set(GPERFTOOLS_LIBRARY_DIRS "/usr/local/lib")
set(GPERFTOOLS_PROFILER "profiler")

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

find_library(Cocoa_FRAMEWORK Cocoa)
find_library(OpenGL_FRAMEWORK OpenGL)
find_library(IOKit_FRAMEWORK IOKit)
find_library(CoreFoundation_FRAMEWORK CoreFoundation)
find_library(CoreVideo_FRAMEWORK CoreVideo)
find_library(CoreAudio_FRAMEWORK CoreAudio)
find_library(CoreMIDI_FRAMEWORK CoreMIDI)

set(Portaudio_FIND_REQUIRED TRUE)
include(cmake_modules/FindPortAudio.cmake)

include_directories(SYSTEM ${GLFW_INCLUDE_DIRS} ${PORTAUDIO_INCLUDE_DIRS})
link_directories("${GLFW_LIBRARY_DIRS}" "${GPERFTOOLS_LIBRARY_DIRS}")

add_library(mem mem.cpp)

add_library(cpu cpu.cpp)

add_library(opcodes opcodes.cpp)

add_library(debugger debugger.cpp)

add_library(screen screen.cpp)
target_link_libraries(screen
  ${GLFW_LIBRARIES}
  ${Cocoa_FRAMEWORK} ${OpenGL_FRAMEWORK}
  ${IOKit_FRAMEWORK} ${CoreFoundation_FRAMEWORK}
  ${CoreVideo_FRAMEWORK}
  )

add_library(pulseunit PulseUnit.cpp)

add_library(customwaveunit CustomWaveUnit.cpp)

add_library(audio audio.cpp pulseunit customwaveunit)
target_link_libraries(audio ${PORTAUDIO_LIBRARIES})

add_executable(cpu-test cpu-test.cpp cpu opcodes mem screen debugger audio pulseunit customwaveunit)
target_link_libraries(cpu-test
  ${GLFW_LIBRARIES}
  ${Cocoa_FRAMEWORK} ${OpenGL_FRAMEWORK}
  ${IOKit_FRAMEWORK} ${CoreFoundation_FRAMEWORK}
  ${CoreVideo_FRAMEWORK}
  ${PORTAUDIO_LIBRARIES}
  )

add_executable(spearow spearow.cpp cpu opcodes mem debugger screen audio pulseunit customwaveunit)
target_link_libraries(spearow
  ${GLFW_LIBRARIES}
  ${Cocoa_FRAMEWORK} ${OpenGL_FRAMEWORK}
  ${IOKit_FRAMEWORK} ${CoreFoundation_FRAMEWORK}
  ${CoreVideo_FRAMEWORK} ${GPERFTOOLS_PROFILER}
  ${PORTAUDIO_LIBRARIES}
  )
