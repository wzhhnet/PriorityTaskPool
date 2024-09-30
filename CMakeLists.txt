cmake_minimum_required(VERSION 3.10)
#Priority task pool library
project(ptpl LANGUAGES C CXX)

option (ENABLE_TEST "option for test" OFF)

add_library(${PROJECT_NAME} STATIC priority_task_pool.cc)

if (${ENABLE_TEST})
 add_executable(test test_main.cc)
 target_link_libraries(test ${PROJECT_NAME})
endif ()
