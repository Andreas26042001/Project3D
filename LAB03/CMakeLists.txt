project("LAB03")


set(SOURCES_EX_01 "exercices/ex01/ex01.cpp" "camera.h" "shader.h")
set(SOURCES_EX_02 "exercices/ex02/ex02.cpp" "camera.h" "shader.h" "exercices/object.h")
set(SOURCES_EX_03 "exercices/ex03/ex03.cpp" "camera.h" "shader.h" "exercices/object.h")
set(SOURCES_EX_04 "exercices/ex04/ex04.cpp" "camera.h" "shader.h" "exercices/object.h")
set(SOURCES_EX_05 "exercices/ex05/ex05.cpp" "camera.h" "shader.h" "exercices/object.h")
set(SOURCES_EX_06 "exercices/ex06/ex06.cpp" "camera.h" "shader.h" "exercices/object.h")
set(SOURCES_EX_07 "exercices/ex07/ex07.cpp" "camera.h" "shader.h" "exercices/object.h")
set(SOURCES_EX_08 "exercices/ex08/ex08.cpp" "camera.h" "shader.h" "exercices/object.h")
set(SOURCES_EX_09 "exercices/ex09/ex09.cpp" "camera.h" "shader.h" "exercices/object.h")
set(SOURCES_EX_10 "exercices/ex10/ex10.cpp" "camera.h" "shader.h" "exercices/object.h")
set(SOURCES_EX_11 "exercices/ex11/ex11.cpp")

#These commands are there to specify the path to the folder containing the object and textures files as macro
#With these you can just use PATH_TO_OBJECTS and PATH_TO_TEXTURE in your c++ code and the compiler will replace it by the correct expression
add_compile_definitions(PATH_TO_OBJECTS="${CMAKE_CURRENT_SOURCE_DIR}/objects")
add_compile_definitions(PATH_TO_TEXTURE="${CMAKE_CURRENT_SOURCE_DIR}/textures")




add_executable(${PROJECT_NAME}_ex01 ${SOURCES_EX_01})
target_link_libraries(${PROJECT_NAME}_ex01 PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex02 ${SOURCES_EX_02})
target_link_libraries(${PROJECT_NAME}_ex02 PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex03 ${SOURCES_EX_03})
target_link_libraries(${PROJECT_NAME}_ex03 PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex04 ${SOURCES_EX_04})
target_link_libraries(${PROJECT_NAME}_ex04 PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex05 ${SOURCES_EX_05})
target_link_libraries(${PROJECT_NAME}_ex05 PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex06 ${SOURCES_EX_06})
target_link_libraries(${PROJECT_NAME}_ex06 PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex07 ${SOURCES_EX_07})
target_link_libraries(${PROJECT_NAME}_ex07 PUBLIC OpenGL::GL glfw glad)


add_executable(${PROJECT_NAME}_ex08 ${SOURCES_EX_08})
target_link_libraries(${PROJECT_NAME}_ex08 PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex09 ${SOURCES_EX_09})
target_link_libraries(${PROJECT_NAME}_ex09 PUBLIC OpenGL::GL glfw glad)


add_executable(${PROJECT_NAME}_ex10 ${SOURCES_EX_10})
target_link_libraries(${PROJECT_NAME}_ex10 PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex11 ${SOURCES_EX_11})
target_link_libraries(${PROJECT_NAME}_ex11 PUBLIC OpenGL::GL glfw glad)


if(COMPILE_SOLUTIONS)

set(SOURCES_EX_01_SOL "solutions/ex01/sol01.cpp" "camera.h" "shader.h")
set(SOURCES_EX_02_SOL "solutions/ex02/sol02.cpp" "camera.h" "shader.h" "solutions/object.h")
set(SOURCES_EX_03_SOL "solutions/ex03/sol03.cpp" "camera.h" "shader.h" "solutions/object.h")
set(SOURCES_EX_04_SOL "solutions/ex04/sol04.cpp" "camera.h" "shader.h" "solutions/object.h")
set(SOURCES_EX_05_SOL "solutions/ex05/sol05.cpp" "camera.h" "shader.h" "solutions/object.h")

set(SOURCES_EX_06_SOL "solutions/ex06/sol06.cpp" "camera.h" "shader.h" "solutions/object.h")
set(SOURCES_EX_07_SOL "solutions/ex07/sol07.cpp" "camera.h" "shader.h" "solutions/object.h")
set(SOURCES_EX_08_SOL "solutions/ex08/sol08.cpp" "camera.h" "shader.h" "solutions/object.h")
set(SOURCES_EX_09_SOL "solutions/ex09/sol09.cpp" "camera.h" "shader.h" "solutions/object.h")
set(SOURCES_EX_10_SOL "solutions/ex10/sol10.cpp" "camera.h" "shader.h" "solutions/object.h")


add_executable(${PROJECT_NAME}_ex01_sol ${SOURCES_EX_01_SOL})
target_link_libraries(${PROJECT_NAME}_ex01_sol PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex02_sol ${SOURCES_EX_02_SOL})
target_link_libraries(${PROJECT_NAME}_ex02_sol PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex03_sol ${SOURCES_EX_03_SOL})
target_link_libraries(${PROJECT_NAME}_ex03_sol PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex04_sol ${SOURCES_EX_04_SOL})
target_link_libraries(${PROJECT_NAME}_ex04_sol PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex05_sol ${SOURCES_EX_05_SOL})
target_link_libraries(${PROJECT_NAME}_ex05_sol PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex06_sol ${SOURCES_EX_06_SOL})
target_link_libraries(${PROJECT_NAME}_ex06_sol PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex07_sol ${SOURCES_EX_07_SOL})
target_link_libraries(${PROJECT_NAME}_ex07_sol PUBLIC OpenGL::GL glfw glad)


add_executable(${PROJECT_NAME}_ex08_sol ${SOURCES_EX_08_SOL})
target_link_libraries(${PROJECT_NAME}_ex08_sol PUBLIC OpenGL::GL glfw glad)

add_executable(${PROJECT_NAME}_ex09_sol ${SOURCES_EX_09_SOL})
target_link_libraries(${PROJECT_NAME}_ex09_sol PUBLIC OpenGL::GL glfw glad)


add_executable(${PROJECT_NAME}_ex10_sol ${SOURCES_EX_10_SOL})
target_link_libraries(${PROJECT_NAME}_ex10_sol PUBLIC OpenGL::GL glfw glad)

endif()