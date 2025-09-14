cmake_minimum_required(VERSION 3.16)
project(QtImageOverlay LANGUAGES CXX)

# C++ standard & warnings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable strict warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(
        -Wall       # enable all warnings
        -Wextra     # even more warnings
        -Wpedantic  # strict ISO C++ conformance
        -Werror     # treat warnings as errors
    )
endif()

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

find_program(CLANG_TIDY_EXE
    NAMES clang-tidy clang-tidy-15 clang-tidy-16
    PATHS /opt/homebrew/opt/llvm /opt/homebrew/opt/llvm/bin
)
set(CLANG_TIDY /opt/homebrew/opt/llvm/bin/clang-tidy)

message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-checks=*,clang-analyzer-*;-warnings-as-errors=*")

# find_program(CLANG_FORMAT_EXE
#     NAMES clang-format clang-format-15 clang-format-16
#     PATHS /usr/local/bin /opt/homebrew/bin /opt/homebrew/opt/llvm/bin
# )

# if(CLANG_FORMAT_EXE)
#     message(STATUS "clang-format found: ${CLANG_FORMAT_EXE}")
#     file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
#     add_custom_target(format
#         COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_SRC}
#         COMMENT "Running clang-format on all sources..."
#         VERBATIM
#     )
# else()
#     message(WARNING "clang-format NOT found! Install it via brew: brew install clang-format")
# endif()