cmake_minimum_required(VERSION 3.0)

project(libyaml-cpp)

add_library(libyaml-cpp
	src/MapObject.cpp
)

target_link_libraries(libyaml-cpp yaml)

set_property(TARGET libyaml-cpp PROPERTY CXX_STANDARD 17)
