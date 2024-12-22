cmake_minimum_required(VERSION 3.29 FATAL_ERROR)

project(vulkan_glfw VERSION 1.0.0 LANGUAGES CXX)

set(SOURCES
    "src/main.cpp"
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(EXECUTABLE_NAME vulkan_glfw)

if(APPLE)
    file(GLOB RESOURCE_FILES resources)
    set_source_files_properties(${RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "MacOS")

    add_executable(${EXECUTABLE_NAME} MACOSX_BUNDLE ${SOURCES} ${RESOURCE_FILES})
else()
    add_executable(${EXECUTABLE_NAME} ${SOURCES})

    # file(COPY resources DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    # file(COPY data DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

set_target_properties(${EXECUTABLE_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON
)

target_compile_features(${EXECUTABLE_NAME} PRIVATE cxx_std_20)

# find_package(glad CONFIG REQUIRED)
# target_link_libraries(${EXECUTABLE_NAME} PRIVATE glad::glad)

find_package(Vulkan REQUIRED)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE Vulkan::Vulkan)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(${EXECUTABLE_NAME} PRIVATE glfw)

find_package(Stb REQUIRED)
target_include_directories(${EXECUTABLE_NAME} PRIVATE ${Stb_INCLUDE_DIR})
