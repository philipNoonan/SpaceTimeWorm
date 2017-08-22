cmake_minimum_required(VERSION 3.0)

project(spaceTimeWorm)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})
include(BasicConfig)

#dlib
#include(${THIRD_PARTY_DIR}/dlib-18.18/dlib/cmake)
#freeglut
set(THIRD_PARTY_DIR $ENV{THIRD_PARTY_DIR})
set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${THIRD_PARTY_DIR}/freeglut/include)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${THIRD_PARTY_DIR}/freeglut/lib)
#glew
set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${THIRD_PARTY_DIR}/glew/include)
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${THIRD_PARTY_DIR}/glew/lib)
#PCL

find_package(OpenCV REQUIRED)
find_package(OpenGL REQUIRED)
#find_package(CUDA REQUIRED)
find_package(GLFW REQUIRED)
find_package(GLEW REQUIRED)
find_package(Freenect2 REQUIRED)
find_package(Aruco REQUIRED)


file(GLOB_RECURSE SRC "src/*")

add_executable(${PROJECT_NAME} ${SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_PDB_NAME_DEBUG "${PROJECT_NAME}${CMAKE_DEBUG_POSTFIX}")

#set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:WINDOWS")
#ADD_DEFINITIONS(-DUNICODE)
#ADD_DEFINITIONS(-D_UNICODE)
	
#target_include_directories(${PROJECT_NAME} PUBLIC ${CUDA_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${OPENGL_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${GLFW_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${FREENECT2_INCLUDE_PATH})
target_include_directories(${PROJECT_NAME} PUBLIC ${ARUCO_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${GLEW_INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${GLUT_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC ${OpenCV_LIBS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENGL_gl_LIBRARY})
target_link_libraries(${PROJECT_NAME} PUBLIC ${GLFW_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${FREENECT2_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PUBLIC ${GLEW_LIBRARY})
target_link_libraries(${PROJECT_NAME} PUBLIC ${ARUCO_LIBRARIES})
#target_link_libraries(${PROJECT_NAME} PUBLIC ${PCL_COMMON_LIBRARIES}) #this includes a lot of vtk libs

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ./resources $<TARGET_FILE_DIR:${PROJECT_NAME}>)
					   
MESSAGE(STATUS "Copying resources to ${PROJECT_BINARY_DIR}")
file(COPY ./resources DESTINATION ${PROJECT_BINARY_DIR})
#file(COPY ../../resources DESTINATION ${PROJECT_BINARY_DIR}/bin/release)


