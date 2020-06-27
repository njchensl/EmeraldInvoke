cmake_minimum_required(VERSION 3.16)
project(EmeraldInvoke)

set(CMAKE_CXX_STANDARD 20)

include_directories(${CMAKE_SOURCE_DIR}/src/vendor)

add_executable(EmeraldInvoke src/Emerald/main.cpp src/Emerald/stdlibext.cpp)
target_link_libraries(EmeraldInvoke ${CMAKE_SOURCE_DIR}/lib/cinvoke.lib)
target_compile_options(EmeraldInvoke PUBLIC /VERBOSE /MD)
target_link_options(EmeraldInvoke PUBLIC /NODEFAULTLIB:LIBC.lib)
