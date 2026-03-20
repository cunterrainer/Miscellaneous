cmake_minimum_required(VERSION 3.16)
project(Tetris CXX)
set(CMAKE_CXX_STANDARD 17)
find_package(Curses REQUIRED)
add_executable(tetris
    src/main.cpp src/tetromino.cpp
    src/board.cpp src/renderer.cpp src/game.cpp)
target_include_directories(tetris PRIVATE ${CURSES_INCLUDE_DIR} src)
target_link_libraries(tetris PRIVATE ${CURSES_LIBRARIES})
