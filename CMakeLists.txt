cmake_minimum_required(VERSION 3.16)

project(CustomizationTool
    VERSION 1.0.0
    DESCRIPTION "ROM Customization Toolkit"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# The code uses the Qt 5 QtMultimedia API (QMediaPlayer::setMedia etc.)
find_package(Qt5 5.15 REQUIRED COMPONENTS
    Core Gui Widgets Network Concurrent Multimedia MultimediaWidgets)

set(CT_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/core/logger.cpp
    src/core/logger.h
    src/core/platform.cpp
    src/core/platform.h
    src/core/shellrunner.cpp
    src/core/shellrunner.h
    src/core/unpacker.cpp
    src/core/unpacker.h
    src/core/patchrunner.cpp
    src/core/patchrunner.h
    src/core/easymd5.cpp
    src/core/easymd5.h
    src/theme/materialtheme.cpp
    src/theme/materialtheme.h
    src/theme/iconhelper.cpp
    src/theme/iconhelper.h
    src/dialogs/unpackleveldialog.cpp
    src/dialogs/unpackleveldialog.h
    src/dialogs/downloaddialog.cpp
    src/dialogs/downloaddialog.h
    src/dialogs/tutorialdialog.cpp
    src/dialogs/tutorialdialog.h
    # Icons and the precompiled translations (.qm) are embedded via this .qrc
    resources/resources.qrc
)

add_executable(CustomizationTool ${CT_SOURCES})

# Windows: build a GUI (no console) application
if(WIN32)
    set_target_properties(CustomizationTool PROPERTIES WIN32_EXECUTABLE ON)
endif()

target_compile_definitions(CustomizationTool PRIVATE
    APP_VERSION="${PROJECT_VERSION}")

target_compile_options(CustomizationTool PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wall -Wextra>
    # sources contain UTF-8 characters (e.g. arrows / dashes in strings)
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8 /W3>)

target_link_libraries(CustomizationTool PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::Concurrent
    Qt5::Multimedia
    Qt5::MultimediaWidgets)

# ---------------------------------------------------------------------------
# Runtime folders (tools, patches, scripts, keys)
# On Windows the app keeps bin/ patches/ keys/ ... next to the .exe (portable layout), so they are
# copied beside the executable after every build, and installed next to it by `cmake --install`.
# ---------------------------------------------------------------------------
set(CT_RUNTIME_DIRS bin patches scripts keys)

if(WIN32)
    foreach(dir ${CT_RUNTIME_DIRS})
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
            add_custom_command(TARGET CustomizationTool POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${CMAKE_CURRENT_SOURCE_DIR}/${dir}"
                        "$<TARGET_FILE_DIR:CustomizationTool>/${dir}"
                VERBATIM)
        endif()
    endforeach()

    # `cmake --build build --target deploy` copies the Qt DLLs/plugins next to the .exe
    get_target_property(_qmake_exe Qt5::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_exe}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_target(deploy
            COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations
                    "$<TARGET_FILE:CustomizationTool>"
            DEPENDS CustomizationTool
            COMMENT "Running windeployqt"
            VERBATIM)
    endif()

    install(TARGETS CustomizationTool RUNTIME DESTINATION .)
    set(CT_DATA_DEST .)
else()
    install(TARGETS CustomizationTool RUNTIME DESTINATION bin)
    set(CT_DATA_DEST share/CustomizationTool)
endif()

foreach(dir ${CT_RUNTIME_DIRS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
        install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${dir}" DESTINATION ${CT_DATA_DEST})
    endif()
endforeach()

# ---------------------------------------------------------------------------
# Translations
# The .qm files in translations/ are committed and embedded by resources.qrc.
# These optional targets refresh them from the sources:
#   cmake --build build --target update_translations   (rescan sources -> .ts)
#   cmake --build build --target release_translations  (.ts -> .qm)
# ---------------------------------------------------------------------------
find_package(Qt5 QUIET COMPONENTS LinguistTools)
if(Qt5LinguistTools_FOUND)
    set(CT_TS_FILES
        translations/ct_zh_CN.ts
        translations/ct_zh_TW.ts
        translations/ct_ja_JP.ts
        translations/ct_ru_RU.ts)

    add_custom_target(update_translations
        COMMAND Qt5::lupdate ${CMAKE_CURRENT_SOURCE_DIR}/src
                -ts ${CT_TS_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Updating .ts files from sources"
        VERBATIM)

    add_custom_target(release_translations
        COMMAND Qt5::lrelease ${CT_TS_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Compiling .ts files to .qm"
        VERBATIM)
endif()
