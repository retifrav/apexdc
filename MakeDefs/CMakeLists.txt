cmake_minimum_required(VERSION 3.22)

project("MakeDefs")

add_executable(${PROJECT_NAME})

set(sources
    StdAfx.cpp
    MakeDefs.cpp
    # isn't great, but okay
    ${CMAKE_SOURCE_DIR}/client/client/File.cpp
    ${CMAKE_SOURCE_DIR}/client/client/SimpleXML.cpp
    ${CMAKE_SOURCE_DIR}/client/client/SimpleXMLReader.cpp
    ${CMAKE_SOURCE_DIR}/client/client/StringTokenizer.cpp
    ${CMAKE_SOURCE_DIR}/client/client/Text.cpp
    ${CMAKE_SOURCE_DIR}/client/client/Util.cpp
)

target_sources(${PROJECT_NAME}
    PRIVATE
        ${sources}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/client
)

set_target_properties(${PROJECT_NAME}
    PROPERTIES
        DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
        # LINK_FLAGS
        #     "/NODEFAULTLIB:libcmt.lib"
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        "_UNICODE"
        "UNICODE"
        "_CONSOLE"
        #"NDEBUG"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        boost
        WS2_32
)
