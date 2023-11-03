cmake_minimum_required(VERSION 3.22)

project("client")

add_library(${PROJECT_NAME})

set(sources
    AdcCommand.cpp
    AdcHub.cpp
    ADLSearch.cpp
    Archive.cpp
    BufferedSocket.cpp
    BZUtils.cpp
    ChatMessage.cpp
    CID.cpp
    Client.cpp
    ClientManager.cpp
    ConnectionManager.cpp
    ConnectivityManager.cpp
    CryptoManager.cpp
    DCPlusPlus.cpp
    DetectionManager.cpp
    DirectoryListing.cpp
    Download.cpp
    DownloadManager.cpp
    Encoder.cpp
    FavoriteManager.cpp
    File.cpp
    FileReader.cpp
    FinishedManager.cpp
    HashBloom.cpp
    HashManager.cpp
    HttpConnection.cpp
    HttpManager.cpp
    LogManager.cpp
    Magnet.cpp
    Mapper.cpp
    MappingManager.cpp
    NmdcHub.cpp
    PluginApiImpl.cpp
    PluginManager.cpp
    QueueItem.cpp
    QueueManager.cpp
    RawManager.cpp
    ResourceManager.cpp
    SearchManager.cpp
    SearchQueue.cpp
    SearchResult.cpp
    SettingsManager.cpp
    SharedFileStream.cpp
    ShareManager.cpp
    SimpleXML.cpp
    SimpleXMLReader.cpp
    Socket.cpp
    SSL.cpp
    SSLSocket.cpp
    stdinc.cpp
    StringDefs.cpp
    Tagger.cpp
    Text.cpp
    Thread.cpp
    ThrottleManager.cpp
    TigerHash.cpp
    TimerManager.cpp
    Transfer.cpp
    UpdateManager.cpp
    Upload.cpp
    UploadManager.cpp
    User.cpp
    UserCommand.cpp
    UserConnection.cpp
    Util.cpp
    WebServerManager.cpp
    ZipFile.cpp
    ZUtils.cpp
)
target_sources(${PROJECT_NAME}
    PRIVATE
        ${sources}
)

# \MakeDefs\MakeDefs.exe" .\client\StringDefs.h .\client\StringDefs.cpp .\EN_Example.xml

# $(SolutionDir);$(SolutionDir)\boost;$(SolutionDir)\openssl\include;$(SolutionDir)\bzip2;$(SolutionDir)\zlib;
target_include_directories(${PROJECT_NAME}
    PUBLIC
        # where top-level project will look for the library's public headers
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        # where external projects will look for the library's public headers
        #$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        zlib
        minizip
        # OpenSSL::SSL
        # OpenSSL::Crypto
        # boost
        # wtl
        # dht
)
