cmake_minimum_required(VERSION 3.9)

project(imgui VERSION 1.91)

set(include_dirs ${CMAKE_CURRENT_SOURCE_DIR})
set(source_files
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    )

add_library(imgui SHARED ${source_files})
target_include_directories(imgui
    PUBLIC ${include_dirs}
    )