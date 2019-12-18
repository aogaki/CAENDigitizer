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
                    ${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${ROOT_INCLUDE_DIR})
link_directories(${ROOT_LIBRARY_DIR})
file(GLOB headers ${PROJECT_SOURCE_DIR}/include/*.hpp)
file(GLOB waveRecSources
  ${PROJECT_SOURCE_DIR}/src/TDigitizer.cpp
  ${PROJECT_SOURCE_DIR}/src/TWaveRecord.cpp)

file(GLOB psdSources
  ${PROJECT_SOURCE_DIR}/src/TDigitizer.cpp
  ${PROJECT_SOURCE_DIR}/src/TDPP.cpp
  ${PROJECT_SOURCE_DIR}/src/TPSD.cpp)

file(GLOB phaSources
  ${PROJECT_SOURCE_DIR}/src/TDigitizer.cpp
  ${PROJECT_SOURCE_DIR}/src/TDPP.cpp
${PROJECT_SOURCE_DIR}/src/TPHA.cpp)

#----------------------------------------------------------------------------
# Add the executable, and link it to the Geant4 libraries
#
add_executable(digi main.cpp ${waveRecSources} ${headers})
target_link_libraries(digi ${ROOT_LIBRARIES} CAENDigitizer)

add_executable(psd dpp.cpp ${psdSources} ${headers})
target_link_libraries(psd ${ROOT_LIBRARIES} CAENDigitizer)

add_executable(pha PHA.cpp ${phaSources} ${headers})
target_link_libraries(pha ${ROOT_LIBRARIES} CAENDigitizer)
