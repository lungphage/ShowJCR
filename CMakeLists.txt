cmake_minimum_required(VERSION 3.16)

project(ShowJCR LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Sql REQUIRED)

set(PROJECT_SOURCES
    main.cpp
    showjcr.cpp
    showjcr.h
    sqlitedb.cpp
    sqlitedb.h
    aboutdialog.cpp
    aboutdialog.h
    tableselectordialog.h
    tableselectordialog.cpp
    trendchartdialog.h
    trendchartdialog.cpp
    batchquerydialog.h
    batchquerydialog.cpp
    showjcr.ui
    aboutdialog.ui
    tableselectordialog.ui
)

set(app_WINRC resources/logo.rc)
set(qrc_SOURCES resources/resources.qrc)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(ShowJCR WIN32
        ${PROJECT_SOURCES}
        ${app_WINRC}
        ${qrc_SOURCES}
    )
else()
    if(ANDROID)
        add_library(ShowJCR SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(ShowJCR
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(ShowJCR PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(ShowJCR PRIVATE Qt${QT_VERSION_MAJOR}::Sql)
