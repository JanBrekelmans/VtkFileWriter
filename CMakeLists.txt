cmake_minimum_required(VERSION 3.21...3.29)

project(
    VtkFileWriter
    VERSION 0.0.1
    LANGUAGES CXX
)

# ---- Include guards ----
if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---- Add dependencies via CPM ----
include(cmake/CPM.cmake)

CPMADDPACKAGE(
    NAME TINYXML2
    GIT_REPOSITORY https://github.com/leethomason/tinyxml2
    GIT_TAG v10.0.0
)

# ---- Add the project library ----
file(GLOB_RECURSE headers CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.cpp)

add_library(${PROJECT_NAME} STATIC ${headers} ${sources})

target_include_directories(${PROJECT_NAME}
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_link_libraries(${PROJECT_NAME} PUBLIC tinyxml2)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

if(PROJECT_IS_TOP_LEVEL)
    # ---- Add examples ----
    file(GLOB_RECURSE examples CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/examples/*.cxx)
    foreach(example ${examples})
        get_filename_component(EXAMPLE_NAME ${example} NAME_WE)
        add_executable(${EXAMPLE_NAME} ${example})
        target_link_libraries(${EXAMPLE_NAME} PRIVATE ${PROJECT_NAME})
        set_target_properties(${EXAMPLE_NAME} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
    endforeach()
endif()