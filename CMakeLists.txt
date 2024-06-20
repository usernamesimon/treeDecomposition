# My current version, but probably backwards compatible quite a bit
cmake_minimum_required(VERSION 3.25.1)

project(treedecomp)

add_executable(${PROJECT_NAME} src/treedecomp.h src/treedecomp.c src/graph.h src/graph.c)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)