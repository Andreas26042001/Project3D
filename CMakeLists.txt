cmake_minimum_required(VERSION 3.20)
project(MonProjetOpenGL VERSION 1.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Dépendances
find_package(OpenGL REQUIRED)

# GLFW
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    add_subdirectory(3rdParty/glfw)
endif()

# GLM et STB (si pas déjà inclus)
include_directories(include)
include_directories(3rdParty/glm)
include_directories(3rdParty/stb)
include_directories(3rdParty/glad/include)

# Sources
set(SOURCES
    src/main.cpp
    src/Game.cpp
    src/Light.cpp
    3rdParty/glad/src/glad.c
)

# Exécutable
add_executable(${PROJECT_NAME} ${SOURCES})

# Racine des ressources runtime (shaders GLSL, modèles, textures) : chemin absolu pour que
# l'exécutable trouve les fichiers quel que soit le répertoire de travail courant.
target_compile_definitions(${PROJECT_NAME} PRIVATE
    "GAME_RESOURCE_DIR=\"${CMAKE_SOURCE_DIR}/resources\""
)

# Liens
target_link_libraries(${PROJECT_NAME}
    PUBLIC OpenGL::GL
    PUBLIC glfw
)

# Les shaders / modèles / textures sont lus depuis ${CMAKE_SOURCE_DIR}/resources (voir GAME_RESOURCE_DIR).