# BKDoc
# Copyright Calvin Rose

cmake_minimum_required(VERSION 2.8)

project(bkd)

set(CMAKE_C_STANDARD 99)

set(SOURCES
cli/main.c
src/bkd_parse.c
src/bkd_html.c
src/bkd_util.c
src/bkd_utf8.c
src/bkd_string.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g -rdynamic -Wall -Wextra")

include_directories("src")
include_directories("include")
include_directories("cli")

add_executable(bkd ${SOURCES})
