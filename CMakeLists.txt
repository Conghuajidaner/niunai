cmake_minimum_required(VERSION 3.12)

project(niunai)

set(CMAKE_CXX_STANDARD 17)


include_directories(include)

add_library(niunai SHARED ./src/niunai.cpp)

link_libraries(niunai rt)

# install()

# add_subdirectory(test)
