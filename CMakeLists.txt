cmake_minimum_required(VERSION 2.6)
project(gridfloat)

find_package(PNG)

set(SOURCES
  src/linear.c
  src/quadratic.c
  src/gridfloat.c
  src/gfpng.c
  src/gfstl.c
  src/sort.c
  src/rtree.c
  src/db.c
)

add_library(gf STATIC ${SOURCES})
set_target_properties(gf PROPERTIES COMPILE_FLAGS "-g")

add_executable(gridfloat src/main.c)
target_link_libraries(gridfloat gf png z)

add_executable(gridfloat-test test/main.c)
target_link_libraries(gridfloat-test gf m)
set_target_properties(gridfloat-test PROPERTIES COMPILE_FLAGS "-g")

add_test(gridfloat-test "${EXECUTABLE_OUTPUT_PATH}/gridfloat-test")
