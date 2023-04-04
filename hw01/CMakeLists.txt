cmake_minimum_required(VERSION 3.11)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -g")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-sanitize-recover=all -fstack-protector")

project(.)
add_executable(. solution.cpp common.h progtest_solver.h sample_tester.cpp)
target_link_directories(. PUBLIC "[Path to solver folder, not the executable]/x86_64-linux-gnu")
target_link_libraries(. pthread progtest_solver)
