cmake_minimum_required(VERSION 2.6)
project(gridfloat)

find_package(PNG)

set(SOURCES
  src/linear.c
  src/quadratic.c
  src/gridfloat.c
  src/gfpng.c
  src/gfstl.c
)

add_library(gf STATIC ${SOURCES})

add_executable(gridfloat src/main.c)
target_link_libraries(gridfloat gf png z)
