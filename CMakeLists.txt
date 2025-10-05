cmake_minimum_required(VERSION 3.15)
project(rawprocess)

find_package(libraw CONFIG REQUIRED)
find_package(stb CONFIG REQUIRED)
find_package(opencv CONFIG REQUIRED)

add_executable(rawproc src/main.cpp)
target_link_libraries(rawproc PRIVATE
  libraw::libraw
  stb::stb
  opencv::opencv
)
