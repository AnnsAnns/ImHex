cmake_minimum_required(VERSION 3.16)
# https://github.com/epezent/implot
project(imgui_implot)

set(CMAKE_CXX_STANDARD 17)

add_library(imgui_implot OBJECT
    source/implot.cpp
    source/implot_items.cpp
    source/implot_demo.cpp
)

target_include_directories(imgui_implot PUBLIC
    include
)

target_link_libraries(imgui_implot PRIVATE imgui_includes)
set_property(TARGET imgui_implot PROPERTY POSITION_INDEPENDENT_CODE ON)
