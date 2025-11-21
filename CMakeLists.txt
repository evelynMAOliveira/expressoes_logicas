cmake_minimum_required(VERSION 3.10)

project(expressoes_logicas)

add_executable(main
  src/main.c
  src/dnf_converter.c
)

target_include_directories(main PUBLIC include)