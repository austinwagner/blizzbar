cmake_minimum_required(VERSION 3.4)

add_library(blizzbar-common INTERFACE)
target_include_directories(blizzbar-common INTERFACE include)
target_compile_features(blizzbar-common INTERFACE cxx_std_17)