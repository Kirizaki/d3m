cmake_minimum_required(VERSION 3.16)
project(QtImageOverlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake to run moc automatically
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(GDCM REQUIRED)

add_executable(QtImageOverlay
    src/main.cpp
    src/gui/main_window.cpp
    src/gui/image_view.cpp
    include/gui/main_window.h
    include/gui/image_view.h
    include/dicom/dicom_utils.h
)

target_include_directories(QtImageOverlay PRIVATE
    include
    include/dicom
    include/gui
)

target_link_libraries(QtImageOverlay PRIVATE Qt6::Widgets gdcmMSFF)

target_compile_options(QtImageOverlay PRIVATE
    -Wall       # enable all warnings
    -Wextra     # even more warnings
    -Wpedantic  # strict ISO C++ conformance
    -Werror     # treat warnings as errors
)
