#----------------------------------------------------------------------------
# Setup the project
cmake_minimum_required(VERSION 2.6 FATAL_ERROR)
project(digi)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions("-std=c++11 -march=native -mtune=native")
include(FindROOT.cmake)

#----------------------------------------------------------------------------
# Locate sources and headers for this project
# NB: headers are included so they will show up in IDEs
#
include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${ROOT_INCLUDE_DIR})
link_directories(${ROOT_LIBRARY_DIR})
file(GLOB sources ${PROJECT_SOURCE_DIR}/*.cpp)
file(GLOB headers ${PROJECT_SOURCE_DIR}/*.hpp)

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(digi main.cpp ${sources} ${headers})
target_link_libraries(digi ${ROOT_LIBRARIES} CAENDigitizer)
