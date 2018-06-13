project(MyFirstScheduler)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
find_package(SimGrid REQUIRED)
include_directories(${SimGrid_INCLUDE_DIR})

set(SIMULATOR_SOURCES main.c other.c util.c)
add_executable(my_simulator ${SIMULATOR_SOURCES})
target_link_libraries(my_simulator ${SimGrid_LIBRARY})

set(OTHER_SOURCES blah.c bar.c foo.h)
add_executable(other_xp ${OTHER_SOURCES})
target_link_libraries(other_xp ${SimGrid_LIBRARY})
