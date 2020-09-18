cmake_minimum_required(VERSION 3.17.0)

project(selling
        VERSION     0.1.0
        DESCRIPTION "stock strategy with XTP"
        LANGUAGES    CXX)

add_compile_definitions(QT_MESSAGELOGCONTEXT)

# set output path
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_PREFIX_PATH "D:/Qt/Qt5.14.1/5.14.1/msvc2017_64")
find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_subdirectory(config)

find_library(
    xtp_trader
    NAMES xtptraderapi
    PATHS "${CMAKE_SOURCE_DIR}/../include/xtp"
    REQUIRED
)

find_library(
    xtp_quote
    NAMES xtpquoteapi
    PATHS "${CMAKE_SOURCE_DIR}/../include/xtp"
    REQUIRED
)

find_path(
    xtp_includes
    NAMES xtp_trader_api.h xtp_quote_api.h
    PATHS "${CMAKE_SOURCE_DIR}/../include/xtp"
)

file(GLOB APP_HEAD *.h)
file(GLOB APP_SRC *.cpp)
file(GLOB APP_UI *.ui)
add_executable(
    ${PROJECT_NAME}
    ${APP_SRC}
    ${APP_HEAD}
    ${APP_UI}
)

target_include_directories(
    ${PROJECT_NAME}
    PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/config>"
    "$<BUILD_INTERFACE:${xtp_includes}>"
)

target_link_libraries(
    ${PROJECT_NAME}
    PUBLIC
    config
    ${xtp_trader}
    ${xtp_quote}
    Qt5::Widgets
)
