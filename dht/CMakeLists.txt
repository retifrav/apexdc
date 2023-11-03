cmake_minimum_required(VERSION 3.22)

project("dht")

add_library(${PROJECT_NAME})

set(sources
    BootstrapManager.cpp
    ConnectionManager.cpp
    DHT.cpp
    IndexManager.cpp
    KBucket.cpp
    SearchManager.cpp
    stdafx.cpp
    TaskManager.cpp
    UDPSocket.cpp
    Utils.cpp
)
target_sources(${PROJECT_NAME}
    PRIVATE
        ${sources}
)

# <AdditionalIncludeDirectories>$(SolutionDir);$(SolutionDir)\boost;$(SolutionDir)\openssl\include;$(SolutionDir)\bzip2;$(SolutionDir)\zlib;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
target_include_directories(${PROJECT_NAME}
    PUBLIC
        # where top-level project will look for the library's public headers
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        # where external projects will look for the library's public headers
        #$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        #"NDEBUG"
        #"_UNICODE"
        #"UNICODE"
        "HAS_PCH"
)

# target_link_libraries(${PROJECT_NAME}
#     PRIVATE
#         zlib
# )
