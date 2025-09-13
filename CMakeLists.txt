cmake_minimum_required(VERSION 3.16)
project(QtImageOverlay LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake to run moc automatically
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(DEBUG 1)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package(GDCM REQUIRED)

add_executable(QtImageOverlay
    main.cpp
    MainWindow.cpp
    MainWindow.h
)

target_link_libraries(QtImageOverlay PRIVATE Qt6::Widgets gdcmMSFF)

target_compile_options(QtImageOverlay PRIVATE
    -Wall       # enable all warnings
    -Wextra     # even more warnings
    -Wpedantic  # strict ISO C++ conformance
    -Werror     # treat warnings as errors
)
