cmake_minimum_required(VERSION 3.11)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -g")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-sanitize-recover=all -fstack-protector")

project(hw01)
add_executable(hw01 solution.cpp common.h progtest_solver.h sample_tester.cpp)
target_link_directories(hw01 PUBLIC "/home/galrene/school/22_23/ls/osy/hw01/x86_64-linux-gnu/")
target_link_libraries(hw01 pthread progtest_solver)
