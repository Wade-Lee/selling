cmake_minimum_required(VERSION 3.17.0)
project(config VERSION 0.1.0)

add_library(${PROJECT_NAME} config.cpp)
target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    "D:/workspace/include/toml11"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)