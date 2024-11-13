cmake_minimum_required(VERSION 2.8)
if (CMAKE_VERSION VERSION_LESS "3")
  project(json11 CXX)
else()
  cmake_policy(SET CMP0048 NEW) 
  project(json11 VERSION 1.0.0 LANGUAGES CXX)
endif()

add_subdirectory(sub_project)

enable_testing()  # 启用测试

option(JSON11_BUILD_TESTS "Build unit tests" ON)
option(JSON11_ENABLE_DR1467_CANARY "Enable canary test for DR 1467" ON)

if(CMAKE_VERSION VERSION_LESS "3")
  add_definitions(-std=c++11)
else()
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/install")
endif()

add_library(json11 json11.cpp)
target_include_directories(json11 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(json11
  PRIVATE -fPIC -fno-rtti -fno-exceptions -Wall)

include(CheckCXXCompilerFlag)
set(_possible_warnings_flags /W4 /WX -Wextra -Werror)
foreach(_warning_flag ${_possible_warnings_flags})
  unset(_flag_supported)
  CHECK_CXX_COMPILER_FLAG(${_warning_flag} _flag_supported)
  if(${_flag_supported})
    target_compile_options(json11 PRIVATE ${_warning_flag})
  endif()
endforeach()

configure_file("json11.pc.in" "json11.pc" @ONLY)

if (JSON11_BUILD_TESTS)
  if(JSON11_ENABLE_DR1467_CANARY)
    add_definitions(-D JSON11_ENABLE_DR1467_CANARY=1)
  else()
    add_definitions(-D JSON11_ENABLE_DR1467_CANARY=0)
  endif()

  add_executable(json11_test test.cpp)
  target_link_libraries(json11_test json11)
  add_test(NAME json11_test COMMAND json11_test)  # 添加测试
endif()

install(TARGETS json11 DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/lib)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/json11.hpp" DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/include)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/json11.pc" DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install/lib/pkgconfig)