cmake_minimum_required(VERSION 3.10)



# project(sub_project VERSION 1.0 LANGUAGES CXX)
project(sub_project VERSION 1.0)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)



configure_file(config/TutorialConfig.h.in TutorialConfig.h)


add_library(my_json11 src/my_json11.cpp)

message("CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")

target_include_directories(my_json11 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
message("sub_project?my_json11 include dir: ${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(my_json11 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
message("sub_project?my_json11 include dir: ${CMAKE_CURRENT_SOURCE_DIR}/src")



add_executable(my_test src/my_test.cpp)

target_include_directories(my_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/config)
target_include_directories(my_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
message("PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")

target_include_directories(my_test PUBLIC ${PROJECT_BINARY_DIR})
target_include_directories(my_test PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

