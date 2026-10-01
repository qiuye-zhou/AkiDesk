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

    core/ai/AiProvider.h
    core/ai/AiProvider.cpp
    core/audio/SpeechRecognizer.h
    core/audio/SpeechRecognizer.cpp
    core/audio/VitsEngine.h
    core/audio/VitsEngine.cpp
    core/system/CommandExecutor.h
    core/system/CommandExecutor.cpp

    utils/plugin/AnimePlugin.h
    utils/plugin/AnimePlugin.cpp
    utils/plugin/PluginManager.h
    utils/plugin/PluginManager.cpp
    utils/ui/DragHelper.h
    utils/ui/DragHelper.cpp
    utils/ui/ScrollHelper.h
    utils/ui/ScrollHelper.cpp

    ui/characterwindow/characterwindow.h
    ui/characterwindow/characterwindow.cpp
    ui/characterwindow/characterwindow.ui

    ui/chatdialog/chatdialog.h
    ui/chatdialog/chatdialog.cpp
    ui/chatdialog/chatdialog.ui

    ui/chatdialog/components/historypanel.h
    ui/chatdialog/components/historypanel.cpp
    ui/chatdialog/components/historypanel.ui

    ui/settingswindow/settingswindow.h
    ui/settingswindow/settingswindow.cpp
    ui/settingswindow/settingswindow.ui

    ui/settingswindow/pages/general/page_general.h
    ui/settingswindow/pages/general/page_general.cpp
    ui/settingswindow/pages/general/page_general.ui

    ui/settingswindow/pages/general/page_about.h
    ui/settingswindow/pages/general/page_about.cpp
    ui/settingswindow/pages/general/page_about.ui

    ui/settingswindow/pages/ai/page_llm.h
    ui/settingswindow/pages/ai/page_llm.cpp
    ui/settingswindow/pages/ai/page_llm.ui

    ui/settingswindow/pages/ai/page_stt.h
    ui/settingswindow/pages/ai/page_stt.cpp
    ui/settingswindow/pages/ai/page_stt.ui

    ui/settingswindow/pages/ai/page_vits.h
    ui/settingswindow/pages/ai/page_vits.cpp
    ui/settingswindow/pages/ai/page_vits.ui

    ui/settingswindow/pages/character/page_character.h
    ui/settingswindow/pages/character/page_character.cpp
    ui/settingswindow/pages/character/page_character.ui

    ui/settingswindow/pages/character/page_plugin.h
    ui/settingswindow/pages/character/page_plugin.cpp
    ui/settingswindow/pages/character/page_plugin.ui

    assets/resources.qrc
)

qt_add_executable(AkiDesk
    WIN32
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

if(WIN32)
    target_link_libraries(AkiDesk PRIVATE dwmapi)
endif()

include(GNUInstallDirs)
install(TARGETS AkiDesk
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(AkiDesk)
