cmake_minimum_required(VERSION 3.24)
project(osy)

set(CMAKE_CXX_STANDARD 14)

include_directories(hw01)

add_executable(osy
        hw01/x86_64-linux-gnu/libprogtest_solver.a
        hw01/common.h
        hw01/progtest_solver.h
        hw01/sample_tester.cpp
        hw01/sample_tester.h
        hw01/solution.cpp)
