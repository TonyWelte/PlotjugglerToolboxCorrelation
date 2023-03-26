cmake_minimum_required(VERSION 3.7)

project(plotjuggler_toolbox_correlation)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

add_definitions(${QT_DEFINITIONS})
add_definitions(-DQT_PLUGIN)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTOUIC ON)

find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    Concurrent
    Xml
    Svg
    OpenGL
    )
find_package(Qwt REQUIRED)

include_directories(
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Xml_INCLUDE_DIRS}
    ${Qt5Svg_INCLUDE_DIRS}
    ${QWT_INCLUDE_DIRS} )

set(QT_LIBRARIES
    Qt5::Core
    Qt5::Widgets
    Qt5::Xml
    Qt5::Svg )

add_definitions( ${QT_DEFINITIONS} -DQT_PLUGIN )
set( PJ_LIBRARIES ${QT_LIBRARIES} )

find_package(plotjuggler REQUIRED)
message(STATUS "PlotJuggler FOUND")
message(STATUS "plotjuggler_INCLUDE_DIR: ${plotjuggler_INCLUDE_DIR}")
message(STATUS "plotjuggler_LIBRARIES: ${plotjuggler_LIBRARIES}")

include_directories(${plotjuggler_INCLUDE_DIR})
list(APPEND PJ_LIBRARIES ${plotjuggler_LIBRARIES} )
set(PJ_PLUGIN_INSTALL_DIRECTORY bin )

add_subdirectory(KissFFT)

QT5_WRAP_UI ( UI_SRC
    toolbox_correlation.ui
    )

add_library(ToolboxCorrelation SHARED
    toolbox_correlation.cpp
    toolbox_correlation.h
    ${UI_SRC}
    )

target_link_libraries(ToolboxCorrelation
    ${PJ_LIBRARIES}
    kissfft
    plotjuggler_qwt
    )

install(TARGETS ToolboxCorrelation DESTINATION ${PJ_PLUGIN_INSTALL_DIRECTORY}  )
