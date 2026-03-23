cmake_minimum_required(VERSION 4.1)
project(snakecurses)

set(CMAKE_CXX_STANDARD 20)

find_package(Curses REQUIRED)
set(CURSES_NEED_WIDE TRUE)

add_executable(snakecurses main.cpp)

target_link_libraries(snakecurses ${CURSES_LIBRARIES})