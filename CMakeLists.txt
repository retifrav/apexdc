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

add_executable(${CMAKE_PROJECT_NAME}) # WIN32

set(sources
    windows/ADLSearchFrame.cpp
    windows/ADLSProperties.cpp
    windows/AdvancedPage.cpp
    windows/AppearancePage.cpp
    windows/AVIPreview.cpp
    windows/BarShader.cpp
    windows/CDMDebugFrame.cpp
    windows/CertificatesPage.cpp
    windows/ChatCtrl.cpp
    windows/ClientsPage.cpp
    windows/CommandDlg.cpp
    windows/DetectionEntryDlg.cpp
    windows/DirectoryListingFrm.cpp
    windows/DownloadPage.cpp
    windows/EmoticonsDlg.cpp
    windows/EmoticonsManager.cpp
    windows/ExListViewCtrl.cpp
    windows/ExMessageBox.cpp
    windows/ExtendedTrace.cpp
    windows/FakeDetect.cpp
    windows/FavHubGroupsDlg.cpp
    windows/FavHubProperties.cpp
    windows/FavoriteDirsPage.cpp
    windows/FavoritesFrm.cpp
    windows/FolderTree.cpp
    windows/GeneralPage.cpp
    windows/HashToolDlg.cpp
    windows/HtmlToRtf.cpp
    windows/HubFrame.cpp
    windows/ImageDataObject.cpp
    windows/LimitPage.cpp
    windows/LineDlg.cpp
    windows/LogPage.cpp
    windows/MagnetDlg.cpp
    windows/main.cpp
    windows/MainFrm.cpp
    windows/Mapper_MiniUPnPc.cpp
    windows/Mapper_NATPMP.cpp
    windows/Mapper_WinUPnP.cpp
    windows/NetworkPage.cpp
    windows/NotepadFrame.cpp
    windows/OMenu.cpp
    windows/OperaColorsPage.cpp
    windows/ParamsPage.cpp
    windows/PluginApiWin.cpp
    windows/PluginsPage.cpp
    windows/PopupManager.cpp
    windows/Popups.cpp
    windows/PreviewDlg.cpp
    windows/PrivateFrame.cpp
    windows/PropertiesDlg.cpp
    windows/PropPage.cpp
    windows/PropPageTextStyles.cpp
    windows/PublicHubsFrm.cpp
    windows/QueueFrame.cpp
    windows/QueuePage.cpp
    windows/RawDlg.cpp
    windows/RawPage.cpp
    windows/RecentsFrm.cpp
    windows/SDCPage.cpp
    windows/SearchFrm.cpp
    windows/SharePage.cpp
    windows/ShellContextMenu.cpp
    windows/Sounds.cpp
    windows/SpyFrame.cpp
    windows/StatsFrame.cpp
    windows/stdafx.cpp
    windows/SystemFrame.cpp
    windows/TextFrame.cpp
    windows/toolbarpage.cpp
    windows/TransferView.cpp
    windows/TreePropertySheet.cpp
    windows/UCPage.cpp
    windows/UpdateDlg.cpp
    windows/UploadPage.cpp
    windows/UploadQueueFrame.cpp
    windows/UserListColours.cpp
    windows/UsersFrame.cpp
    windows/WindowsPage.cpp
    windows/WinUtil.cpp
)

target_sources(${CMAKE_PROJECT_NAME}
    PRIVATE
        ${sources}
)

# VC++ Directories > Include Directories
#set(CMAKE_VS_SDK_INCLUDE_DIRECTORIES "$(VC_IncludePath);$(WindowsSDK_IncludePath)")

target_precompile_headers(${CMAKE_PROJECT_NAME}
    PRIVATE
        windows/stdafx.h
)

set_target_properties(${CMAKE_PROJECT_NAME}
    PROPERTIES
        DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX}
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        #LINK_FLAGS
        #    "/NODEFAULTLIB:kernel32.lib;advapi32.lib;user32.lib;gdi32.lib;shell32.lib;comdlg32.lib;version.lib;mpr.lib;rasapi32.lib;winmm.lib;winspool.lib;vfw32.lib;secur32.lib;oleacc.lib;oledlg.lib;sensapi.lib /DELAYLOAD:oleaut32.dll"
)

target_compile_definitions(${CMAKE_PROJECT_NAME}
    PRIVATE
        "_WIN32_WINNT=0x600"
        "WINVER=0x600"
        "HAVE_NATUPNP_H"
        "HAS_PCH"
        "_UNICODE"
        "UNICODE"
        #"NDEBUG"
)

find_package(minizip CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(miniupnpc CONFIG REQUIRED)
find_package(natpmp CONFIG REQUIRED)
find_package(wtl CONFIG REQUIRED)
find_package(zlib CONFIG REQUIRED)

add_subdirectory(boost)
add_subdirectory(dht)
add_subdirectory(client)

find_library(ShFolder_LIBRARY_PATH ShFolder)
find_library(WS2_32_LIBRARY_PATH WS2_32)
find_library(WinMM_LIBRARY_PATH WinMM)
find_library(powrprof_LIBRARY_PATH powrprof)

message(STATUS "ShFolder_LIBRARY_PATH: ${ShFolder_LIBRARY_PATH}")
message(STATUS "WS2_32_LIBRARY_PATH: ${WS2_32_LIBRARY_PATH}")
message(STATUS "WinMM_LIBRARY_PATH: ${WinMM_LIBRARY_PATH}")
message(STATUS "powrprof_LIBRARY_PATH: ${powrprof_LIBRARY_PATH}")

target_link_libraries(${CMAKE_PROJECT_NAME}
    PRIVATE
        boost
        dht
        client
        wtl
        ${ShFolder_LIBRARY_PATH}
        ${WS2_32_LIBRARY_PATH}
        ${WinMM_LIBRARY_PATH}
        ${powrprof_LIBRARY_PATH}
        miniupnpc::miniupnpc
        natpmp
)

# <AdditionalOptions>/FIXED:NO /SAFESEH:NO %(AdditionalOptions)</AdditionalOptions>

# <Midl>
#   <MkTypLibCompatible>true</MkTypLibCompatible>
#   <SuppressStartupBanner>true</SuppressStartupBanner>
#   <TargetEnvironment>X64</TargetEnvironment>
#   <TypeLibraryName>$(SolutionDir)compiled/$(ProjectName).tlb</TypeLibraryName>
# </Midl>

# <None Include="res\ADLSearch.ico" />
# <None Include="res\arrows.png" />
# <None Include="res\CDM.ico" />
# <None Include="res\cell.ico" />
# <None Include="res\cell_bg.png" />
# <None Include="res\Directory.ico" />
# <None Include="res\emoticon.png" />
# <None Include="res\Exec.ico" />
# <None Include="res\fav.png" />
# <None Include="res\Favorites.ico" />
# <None Include="res\FavoriteUser.ico" />
# <None Include="res\feed.ico" />
# <None Include="res\FinishedDL.ico" />
# <None Include="res\FinishedUL.ico" />
# <None Include="res\flags.png" />
# <None Include="res\folders.png" />
# <None Include="res\Hub.ico" />
# <None Include="res\HubOff.ico" />
# <None Include="res\magnet.ico" />
# <None Include="res\netstats.ico" />
# <None Include="res\notepad.ico" />
# <None Include="res\o_settings_dlg.png" />
# <None Include="res\PM.ico" />
# <None Include="res\PublicHubs.ico" />
# <None Include="res\queue.ico" />
# <None Include="res\Recents.ico" />
# <None Include="res\Search.ico" />
# <None Include="res\search_icons.png" />
# <None Include="res\searchspy.ico" />
# <None Include="res\shutdown.ico" />
# <None Include="res\sounds.ico" />
# <None Include="res\speeds.png" />
# <None Include="res\splash.png" />
# <None Include="res\state.png" />
# <None Include="res\stop_ikona.ico" />
# <None Include="res\StrongDC.ico" />
# <None Include="res\toolbar.png" />
# <None Include="res\toolbar20-highlight.png" />
# <None Include="res\toolbar20.png" />
# <None Include="res\TranferSpeeds.png" />
# <None Include="res\TranferUser.ico" />
# <None Include="res\transfer_compressed.ico" />
# <None Include="res\transfer_segmented.ico" />
# <None Include="res\TransferSpeedsBW.png" />
# <None Include="res\Update.ico" />
# <None Include="res\uqueue.ico" />
# <None Include="res\User.ico" />
# <None Include="res\UserOff.ico" />
# <None Include="res\users.png" />
# <None Include="res\WinampControl-highlight.png" />
# <None Include="res\WinampControl.png" />
# <None Include="changelog.txt" />
# <None Include="Compile.txt" />
# <None Include="Extensions.txt" />

# <ItemGroup>
#   <Image Include="res\statusbar.png" />
# </ItemGroup>

# <ItemGroup>
#   <ResourceCompile Include="StrongDC.rc" />
# </ItemGroup>

# for CMAKE_INSTALL_LIBDIR, CMAKE_INSTALL_BINDIR, CMAKE_INSTALL_INCLUDEDIR and others
include(GNUInstallDirs)

# install the target
install(TARGETS ${CMAKE_PROJECT_NAME})
