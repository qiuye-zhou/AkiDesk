cmake_minimum_required(VERSION 3.16)

project(AkiDesk VERSION 1.0.0 LANGUAGES CXX)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/app_version.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/app_version.h"
)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/installer/AkiDesk.iss.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/installer/AkiDesk.iss"
    NEWLINE_STYLE WIN32
)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Multimedia Svg)

set(PROJECT_SOURCES
    main.cpp

    config/AppPaths.h
    config/JsonConfig.h
    config/JsonConfig.cpp

    core/AiProvider.h
    core/AiProvider.cpp
    core/VitsEngine.h
    core/VitsEngine.cpp

    utils/DragHelper.h
    utils/DragHelper.cpp
    utils/ScrollHelper.h
    utils/ScrollHelper.cpp
    utils/AnimePlugin.h
    utils/AnimePlugin.cpp
    utils/PluginManager.h
    utils/PluginManager.cpp

    ui/chatdialog.h
    ui/chatdialog.cpp
    ui/chatdialog.ui

    ui/characterwindow.h
    ui/characterwindow.cpp
    ui/characterwindow.ui

    ui/historypanel.h
    ui/historypanel.cpp
    ui/historypanel.ui

    ui/settingswindow.h
    ui/settingswindow.cpp
    ui/settingswindow.ui

    ui/pages/page_llm.h
    ui/pages/page_llm.cpp
    ui/pages/page_llm.ui

    ui/pages/page_character.h
    ui/pages/page_character.cpp
    ui/pages/page_character.ui

    ui/pages/page_vits.h
    ui/pages/page_vits.cpp
    ui/pages/page_vits.ui

    ui/pages/page_plugin.h
    ui/pages/page_plugin.cpp
    ui/pages/page_plugin.ui

    ui/pages/page_about.h
    ui/pages/page_about.cpp
    ui/pages/page_about.ui

    assets/resources.qrc
)

qt_add_executable(AkiDesk
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

target_include_directories(AkiDesk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(AkiDesk PRIVATE
    Qt6::Widgets
    Qt6::Network
    Qt6::Multimedia
    Qt6::Gui
    Qt6::Svg
)

if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(AkiDesk PRIVATE X11::X11 X11::Xext)
endif()

include(GNUInstallDirs)
install(TARGETS AkiDesk
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(AkiDesk)
