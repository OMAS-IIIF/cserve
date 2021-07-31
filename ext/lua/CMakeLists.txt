cmake_minimum_required(VERSION 3.19)
project(liblua_builder C CXX)
include(ExternalProject)

#
# get lua
#
ExternalProject_Add(project_lua
    INSTALL_DIR ${COMMON_LOCAL}
    URL http://www.lua.org/ftp/lua-5.4.3.tar.gz
    SOURCE_DIR ${COMMON_SRCS}/lua-5.4.3
    CONFIGURE_COMMAND ls
    BUILD_COMMAND make INSTALL_LIB=${CONFIGURE_LIBDIR} ${LUA_ARCH}
    INSTALL_COMMAND make INSTALL_LIB=${CONFIGURE_LIBDIR} install INSTALL_TOP=${COMMON_LOCAL}
    BUILD_IN_SOURCE 1)

ExternalProject_Get_Property(project_lua install_dir)

if(MAKE_SHARED_CSERVE)
    add_library(lua SHARED IMPORTED GLOBAL)
    set_property(TARGET lua PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/liblua${CMAKE_SHARED_LIBRARY_SUFFIX})
else()
    add_library(lua STATIC IMPORTED GLOBAL)
    set_property(TARGET lua PROPERTY IMPORTED_LOCATION ${CONFIGURE_LIBDIR}/liblua${CMAKE_STATIC_LIBRARY_SUFFIX})
endif()

add_dependencies(lua project_lua)
