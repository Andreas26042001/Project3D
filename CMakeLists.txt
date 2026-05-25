#Template for INFOH502
#Heavily inspired from https://github.com/Polytonic/Glitter/blob/master/CMakeLists.txt

cmake_minimum_required(VERSION 3.20 FATAL_ERROR)

#project setup
project(SpacePuzzle)

#set the minimal version of cmake and the minimal version of the c++ language
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_VERBOSE_MAKEFILE ON)

# Set output directories for binaries and libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)


find_package(OpenGL REQUIRED)

#for glad library
add_library( glad STATIC 3rdParty/glad/src/glad.c)
set(GLAD_INCLUDE "3rdParty/glad/include")
target_include_directories(glad PUBLIC ${GLAD_INCLUDE})

#for GLFW
option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)

#Search for glfw in case it is installed
find_package(glfw3 QUIET)
if (NOT glfw3_FOUND)
    #if glfw is not found, we will use the sources from the submodules
    add_subdirectory(3rdParty/glfw)
    include_directories(3rdParty/glfw/include/)
endif()


include_directories(3rdParty/glad/include/
                    3rdParty/glm/
                    3rdParty/stb/)


add_compile_definitions(PATH_TO_OBJECTS="${CMAKE_CURRENT_SOURCE_DIR}/assets/objects")
add_compile_definitions(PATH_TO_TEXTURE="${CMAKE_CURRENT_SOURCE_DIR}/assets/textures")
add_compile_definitions(PATH_TO_SHADER="${CMAKE_CURRENT_SOURCE_DIR}/shader")
add_compile_definitions(PATH_TO_CUBESMAPS="${CMAKE_CURRENT_SOURCE_DIR}/assets/cubesmaps")


set(SOURCES_COMMON
    src/Game.cpp
    src/Game_Render.cpp
    src/Game_Physics.cpp
    src/GameInternal.cpp
    src/Collider.cpp
    src/Skybox.cpp
    src/ParticleSystem.cpp
    src/ShadowMap.cpp
    src/Player.cpp
    src/Texture.cpp
    src/shader.cpp
    src/camera.cpp
    src/object.cpp
    src/LightProjectileSystem.cpp
    src/PuzzleSystem.cpp
    src/CrosshairRenderer.cpp
    src/BeamRenderer.cpp
    src/PrismRenderer.cpp
)

set(SOURCES_main
    main.cpp
    ${SOURCES_COMMON}
)


if (NOT glfw3_FOUND)
    add_executable(${PROJECT_NAME} ${SOURCES_main})
    target_include_directories(${PROJECT_NAME} PUBLIC ${GLAD_INCLUDE} 3rdParty/glfw/include/ src)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenGL::GL glfw glad)

else()
    add_executable(${PROJECT_NAME} ${SOURCES_main})
    target_include_directories(${PROJECT_NAME} PUBLIC ${GLAD_INCLUDE} src)
    target_link_libraries(${PROJECT_NAME} PUBLIC OpenGL::GL glfw glad)

endif()
