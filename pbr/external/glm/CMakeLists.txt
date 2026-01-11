############################################
#          VisComp Minimal OpenGl          #
#------------------------------------------#
# @author  Nikolaus Rauch / Stefan Spiss   #
# @date    15.10.2021                      #
# @updated 09.12.2024                      #
#------------------------------------------#
############################################
cmake_minimum_required( VERSION 3.5 )
project( viscomp-glm )

message("\n * Visual Computing Minimal OpenGL Example ")
message(STATUS "${PROJECT_NAME} build environment --")
message(STATUS "Build system:     ${CMAKE_SYSTEM_NAME}-${CMAKE_HOST_SYSTEM_PROCESSOR}")


#########################################
#                Options                #
#########################################
option(BUILD_GLFW "Build glfw from source" ON)
option(BUILD_GLM "Build glm from source" ON)


#########################################
#              CMake-Stuff              #
#########################################
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)

#########################################
#        Global Compiler Options        #
#########################################
set(GCC_COMPILE_OPTIONS "-Wall")
set(GCC_COMPILE_DEBUG_OPTIONS "${GCC_COMPILE_OPTIONS};-ggdb;-O0")
set(GCC_COMPILE_RELEASE_OPTIONS "${GCC_COMPILE_OPTIONS};-O3")

add_compile_options("$<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:DEBUG>>:${GCC_COMPILE_DEBUG_OPTIONS}>")
add_compile_options("$<$<AND:$<CXX_COMPILER_ID:GNU>,$<CONFIG:RELEASE>>:${GCC_COMPILE_RELEASE_OPTIONS}>")


#########################################
#     Build/Find External-Libraries     #
#########################################
add_subdirectory(external/glad)
add_subdirectory(external/stb_image)

if(BUILD_GLFW)
    add_subdirectory(external/glfw)
    set_property(TARGET glfw APPEND_STRING PROPERTY COMPILE_FLAGS " -w")
    target_include_directories(glfw PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/external/glfw/include>)
else()
    find_package(glfw3 3.4 REQUIRED)
endif()

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL 3.2 REQUIRED)

if(BUILD_GLM)
  add_subdirectory(external/glm)
else()
  find_package(glm REQUIRED)
endif()

#########################################
#            Build Example              #
#########################################
file(GLOB_RECURSE SRC src/*.cpp)
file(GLOB_RECURSE HDR src/*.h)
file(GLOB_RECURSE SHADER src/*.vert src/*.frag)

source_group(TREE  ${CMAKE_CURRENT_SOURCE_DIR}
             FILES ${SRC} ${HDR} ${SHADER})

add_executable(glm_demo ${SRC} ${HDR} ${SHADER})
target_link_libraries(glm_demo OpenGL::GL glfw glad stb_image glm)
target_include_directories(glm_demo PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
target_compile_features(glm_demo PUBLIC cxx_std_17)
set_target_properties(glm_demo PROPERTIES CXX_EXTENSIONS OFF)

#########################################
#            Visual Studio Flavors      #
#########################################
set_property( DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT glm_demo )
set_target_properties(
    glm_demo PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:glm_demo> )

#########################################
#  Copy Shaders/Assets to build folder  #
#########################################
add_custom_target(glm_demo_copy_shader ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shader
    $<TARGET_FILE_DIR:glm_demo>/shader
    )