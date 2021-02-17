# Disable in-source builds to prevent source tree corruption.
if(" ${CMAKE_SOURCE_DIR}" STREQUAL " ${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "FATAL: In-source builds are not allowed.
       You should create a separate directory for build files.")
endif()

# Set minimum cmake version + cmake settings
cmake_minimum_required (VERSION 3.9)
cmake_policy(SET CMP0048 OLD)

# Specify the project, and set the use of c++ and c
project(cancelthread LANGUAGES CXX C)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -fPIC -rdynamic -Wall -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -rdynamic -Wall -O2 -std=c++11 ")

include_directories(${PROJECT_SOURCE_DIR})

set(pt-cancel pt-cancel.c)
set(pt-docancel pt-docancel.c)
set(pt-init pt-init.c)
set(pt-setcancelstate pt-setcancelstate.c)
set(pt-setcanceltype pt-setcanceltype.c)
set(pt-test pt-test.c)
set(pt-testcancel pt-testcancel.c)


# Build a static lib
add_library(cancelthread STATIC ${pt-cancel} ${pt-docancel} ${pt-init} ${pt-setcancelstate} ${pt-setcanceltype} ${pt-test} ${pt-testcancel})


target_link_libraries(cancelthread -lz -ldl -lm -lpthread)
