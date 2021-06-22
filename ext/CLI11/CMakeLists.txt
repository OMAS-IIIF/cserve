cmake_minimum_required(VERSION 3.19)
project(cli11_builder C CXX)
include(ExternalProject)

ExternalProject_Add(project_cli11
        INSTALL_DIR ${COMMON_LOCAL}
        URL https://github.com/CLIUtils/CLI11/archive/refs/heads/master.zip
        SOURCE_DIR ${COMMON_SRCS}/CLI11-master)