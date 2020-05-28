cmake_minimum_required(VERSION 3.0 FATAL_ERROR)
project(dubins_test)
SET(GCC_COVERAGE_COMPILE_FLAGS "-std=c++14 -O3")
SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
INCLUDE_DIRECTORIES(
  include
  ${CMAKE_SOURCE_DIR}/include
  )



find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(OpenMP REQUIRED)
include_directories( ${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS} "/usr/local/include/eigen3/" ..)
add_executable(test_dubins source/test_dubins.cpp source/fsglxkeymap.cpp source/fsglxwrapper.cpp source/dubins.cpp)
target_link_libraries(test_dubins ${OPENGL_INCLUDE_DIRS}  ${GLUT_INCLUDE_DIRS} /usr/lib/libX11.so /usr/lib/libXext.so.6 m GL OpenMP::OpenMP_CXX)


