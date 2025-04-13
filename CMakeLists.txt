cmake_minimum_required(VERSION 3.10)

project(Assign1_HY342 VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(Assign1_HY342
    src/main.c
)

target_compile_options(Assign1_HY342 PRIVATE 
    -g 
    -ansi 
    -pedantic 
    -Werror
    -D_POSIX_C_SOURCE=200809L 
)

target_link_libraries(Assign1_HY342 PRIVATE pthread)

