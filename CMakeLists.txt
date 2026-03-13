cmake_minimum_required(VERSION 3.15)

project(RayTracer VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Wybierz typ budowania: Debug lub Release" FORCE)
endif()

message(STATUS "Budowanie projektu ${PROJECT_NAME} w trybie: ${CMAKE_BUILD_TYPE}")

find_package(embree 4 REQUIRED)
find_package(glm REQUIRED)
find_package(assimp REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    embree 
    glm::glm
    assimp::assimp
    OpenGL::GL
    glfw
)

add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    "${CMAKE_CURRENT_BINARY_DIR}/assets"
    COMMENT "Kopiowanie folderu assets/ do katalogu budowania..."
)

if(CMAKE_BUILD_TYPE MATCHES "Debug")
    message(STATUS "Włączono flagi dla trybu Debug (AddressSanitizer włączony)")
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -g                          
        -fsanitize=address         
        -fno-strict-aliasing
        -fno-omit-frame-pointer
    )
    target_link_options(${PROJECT_NAME} PRIVATE 
        -fsanitize=address
    )
elseif(CMAKE_BUILD_TYPE MATCHES "Release")
    message(STATUS "Włączono flagi dla trybu Release (Maksymalna wydajność)")
    target_compile_options(${PROJECT_NAME} PRIVATE 
        -O3             
        -march=native
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE NDEBUG) 
endif()