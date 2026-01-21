cmake_minimum_required(VERSION 3.16)
project(codeit LANGUAGES CXX)

# Qt meta-object compiler (REQUIRED for Q_OBJECT in .cpp)
set(CMAKE_AUTOMOC ON)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# pkg-config
find_package(PkgConfig REQUIRED)

# ICU (Unicode)
pkg_check_modules(ICU REQUIRED icu-uc)

# ---- QScintilla (Qt6, Debian/Mint manual lookup) ----
find_path(QSCINTILLA_INCLUDE_DIR
    NAMES Qsci/qsciscintilla.h
    PATHS
        /usr/include/x86_64-linux-gnu/qt6
        /usr/include/qt6
        /usr/include
)

find_library(QSCINTILLA_LIBRARY
    NAMES qscintilla2_qt6
    PATHS
        /usr/lib/x86_64-linux-gnu
        /usr/lib
)

if (NOT QSCINTILLA_INCLUDE_DIR OR NOT QSCINTILLA_LIBRARY)
    message(FATAL_ERROR "QScintilla Qt6 not found. Install libqscintilla2-qt6-dev")
endif()

# Executable
add_executable(codeit
    main.cpp
)

# Include paths
target_include_directories(codeit PRIVATE
    ${ICU_INCLUDE_DIRS}
    ${QSCINTILLA_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(codeit PRIVATE
    Qt6::Widgets
    ${QSCINTILLA_LIBRARY}
    ${ICU_LIBRARIES}
)
