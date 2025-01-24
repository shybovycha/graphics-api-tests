cmake_minimum_required(VERSION 3.29 FATAL_ERROR)

project(directx12 VERSION 1.0.0 LANGUAGES CXX)

set(SOURCES
    "src/main.cpp"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} WIN32 ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE "UNICODE" "_UNICODE")
target_link_libraries(${PROJECT_NAME} PRIVATE "d3d12.lib" "dxgi.lib" "d3dcompiler.lib")

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# list(FILTER _shaders INCLUDE REGEX "\\.hlsl")
# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy ${_shaders} $<TARGET_FILE_DIR:${PROJECT_NAME}> )

file(COPY shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# find_package(glad CONFIG REQUIRED)
# target_link_libraries(${PROJECT_NAME} PRIVATE glad::glad)

# find_package(SDL3 CONFIG REQUIRED)
# target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
