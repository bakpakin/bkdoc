# Bakdown
# Copyright Calvin Rose

cmake_minimum_required(VERSION 2.8)

project(bakdown)

set(CMAKE_C_STANDARD 99)

set(SOURCES
src/main.c
src/bkd_errors.c
src/bkd_parse.c
src/bkd_html.c
src/bkd_util.c
src/bkd_utf8.c
)

include_directories("src")
include_directories("include")

add_executable(bakdown ${SOURCES})
