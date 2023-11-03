cmake_minimum_required(VERSION 3.22)

project(
    "apexdc"
    DESCRIPTION "Innovative DC++ program using the Direct Connect and ADC networks"
    VERSION 1.6.5
)

#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED YES)

set(CMAKE_EXPORT_COMPILE_COMMANDS YES)

# global debug postfix for libraries (executables still need to set it)
set(CMAKE_DEBUG_POSTFIX "d" CACHE STRING "Filename postfix for libraries under DEBUG configuration")

# here it's a top-level project, so CMAKE_PROJECT_NAME is fine
add_executable(${CMAKE_PROJECT_NAME})

set_target_properties(${CMAKE_PROJECT_NAME}
    PROPERTIES
        DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
)

target_compile_definitions(${CMAKE_PROJECT_NAME}
    PRIVATE
        "_UNICODE"
        "UNICODE"
)

file(GLOB sources ${CMAKE_CURRENT_SOURCE_DIR}/client/*)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${sources}
)

find_package(minizip CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(wtl CONFIG REQUIRED)
find_package(zlib CONFIG REQUIRED)

add_subdirectory(boost)

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        zlib
        minizip
        OpenSSL::SSL
        OpenSSL::Crypto
        boost
        wtl
)

# for CMAKE_INSTALL_LIBDIR, CMAKE_INSTALL_BINDIR, CMAKE_INSTALL_INCLUDEDIR and others
include(GNUInstallDirs)

# install the target
install(TARGETS ${CMAKE_PROJECT_NAME})
