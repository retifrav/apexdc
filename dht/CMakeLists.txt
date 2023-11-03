cmake_minimum_required(VERSION 3.22)

project("dht")

add_library(${PROJECT_NAME})

set(sources
    dht/BootstrapManager.cpp
    dht/ConnectionManager.cpp
    dht/DHT.cpp
    dht/IndexManager.cpp
    dht/KBucket.cpp
    dht/SearchManager.cpp
    dht/stdafx.cpp
    dht/TaskManager.cpp
    dht/UDPSocket.cpp
    dht/Utils.cpp
)
target_sources(${PROJECT_NAME}
    PRIVATE
        ${sources}
)

# <AdditionalIncludeDirectories>$(SolutionDir);$(SolutionDir)\boost;$(SolutionDir)\openssl\include;$(SolutionDir)\bzip2;$(SolutionDir)\zlib;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/dht
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

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        boost
        bzip2
        client # dht needs client, and client needs dht, isn't it great
)
