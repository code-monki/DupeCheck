cmake_minimum_required(VERSION 3.21)
project(DupeCheck VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets Sql Concurrent)

qt_standard_project_setup()

# ---------------------------------------------------------------------------
# Engine static library — shared by the main executable and the test suite
# ---------------------------------------------------------------------------
add_library(dupecheck_engine STATIC
    src/engine/DupeEngine.h
    src/engine/DupeEngine.cpp
    src/engine/DataRepository.h
    src/engine/DataRepository.cpp
    src/types.h
)
target_include_directories(dupecheck_engine PUBLIC src)
target_link_libraries(dupecheck_engine PUBLIC Qt6::Sql)

# ---------------------------------------------------------------------------
# Main application
# ---------------------------------------------------------------------------
qt_add_executable(dupecheck
    src/main.cpp
    src/app/DupeApp.h
    src/app/DupeApp.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/ConfigDialog.h
    src/ui/ConfigDialog.cpp
    src/ui/HelpWindow.h
    src/ui/HelpWindow.cpp
)

target_include_directories(dupecheck PRIVATE src)

target_link_libraries(dupecheck
    PRIVATE
    dupecheck_engine
    Qt6::Widgets
    Qt6::Concurrent
)

set_target_properties(dupecheck PROPERTIES
    MACOSX_BUNDLE              TRUE
    MACOSX_BUNDLE_BUNDLE_NAME  "DupeCheck"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.dupecheck.app"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING
        "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    WIN32_EXECUTABLE TRUE
)

# ---------------------------------------------------------------------------
# Installation + deployment
# ---------------------------------------------------------------------------
install(TARGETS dupecheck
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(Qt6_VERSION VERSION_GREATER_EQUAL "6.3")
    qt_generate_deploy_app_script(
        TARGET dupecheck
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
