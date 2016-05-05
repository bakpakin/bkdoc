# Bakdown
# Copyright Calvin Rose

cmake_minimum_required(VERSION 2.8)

project(bakdown)

set(SOURCES
src/main.c
src/bkd_parse.c
src/bkd_debug.c
)

include_directories(src)

add_executable(bakdown ${SOURCES})
