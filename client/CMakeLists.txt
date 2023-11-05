cmake_minimum_required(VERSION 3.22)

project("client")

add_library(${PROJECT_NAME})

set(sources
    client/AdcCommand.cpp
    client/AdcHub.cpp
    client/ADLSearch.cpp
    client/Archive.cpp
    client/BufferedSocket.cpp
    client/BZUtils.cpp
    client/ChatMessage.cpp
    client/CID.cpp
    client/Client.cpp
    client/ClientManager.cpp
    client/ConnectionManager.cpp
    client/ConnectivityManager.cpp
    client/CryptoManager.cpp
    client/DCPlusPlus.cpp
    client/DetectionManager.cpp
    client/DirectoryListing.cpp
    client/Download.cpp
    client/DownloadManager.cpp
    client/Encoder.cpp
    client/FavoriteManager.cpp
    client/File.cpp
    client/FileReader.cpp
    client/FinishedManager.cpp
    client/HashBloom.cpp
    client/HashManager.cpp
    client/HttpConnection.cpp
    client/HttpManager.cpp
    client/LogManager.cpp
    client/Magnet.cpp
    client/Mapper.cpp
    client/MappingManager.cpp
    client/NmdcHub.cpp
    client/PluginApiImpl.cpp
    client/PluginManager.cpp
    client/QueueItem.cpp
    client/QueueManager.cpp
    client/RawManager.cpp
    client/ResourceManager.cpp
    client/SearchManager.cpp
    client/SearchQueue.cpp
    client/SearchResult.cpp
    client/SettingsManager.cpp
    client/SharedFileStream.cpp
    client/ShareManager.cpp
    client/SimpleXML.cpp
    client/SimpleXMLReader.cpp
    client/Socket.cpp
    client/SSL.cpp
    client/SSLSocket.cpp
    client/stdinc.cpp
    client/StringDefs.cpp
    client/Tagger.cpp
    client/Text.cpp
    client/Thread.cpp
    client/ThrottleManager.cpp
    client/TigerHash.cpp
    client/TimerManager.cpp
    client/Transfer.cpp
    client/UpdateManager.cpp
    client/Upload.cpp
    client/UploadManager.cpp
    client/User.cpp
    client/UserCommand.cpp
    client/UserConnection.cpp
    client/Util.cpp
    client/WebServerManager.cpp
    client/ZipFile.cpp
    client/ZUtils.cpp
)
target_sources(${PROJECT_NAME}
    PRIVATE
        ${sources}
)

# \MakeDefs\MakeDefs.exe" .\client\StringDefs.h .\client\StringDefs.cpp .\EN_Example.xml

# $(SolutionDir);$(SolutionDir)\boost;$(SolutionDir)\openssl\include;$(SolutionDir)\bzip2;$(SolutionDir)\zlib;
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/client
    PUBLIC
        # where top-level project will look for the library's public headers
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        # where external projects will look for the library's public headers
        #$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_precompile_headers(${CMAKE_PROJECT_NAME}
    PRIVATE
        client/stdinc.h
)

target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        "_WIN32_WINNT=0x600"
        "WINVER=0x600"
        #"NDEBUG"
        "_UNICODE"
        "UNICODE"
        #"HAS_PCH"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        bzip2
        zlib
        minizip
        OpenSSL::SSL
        OpenSSL::Crypto
        boost
        dht # client needs dht, and dht needs client, isn't it great
)
