cmake_minimum_required(VERSION 3.16)

project(Tyler_Clardy_Renderer)

# CMake FXC shader compilation, add any shaders you want compiled here
set(VERTEX_SHADERS 
	# add vertex shader (.hlsl) files here
	Shaders/VertexShader.hlsl
	Shaders/VertexShader_2D.hlsl
)

set(PIXEL_SHADERS 
	# add pixel shader (.hlsl) files here
	Shaders/PixelShader.hlsl
	Shaders/PixelShader_2D.hlsl
)

# Add any new C/C++ source code here
set(SOURCE_CODE
	# Header & CPP files go here
	Source/main.cpp
	Source/Utils/FileIntoString.h
	Source/Utils/Font.cpp
	Source/Utils/Font.h
	Source/Utils/h2bParser.h
	Source/Utils/load_data_oriented.h
	Source/Utils/Sprite.cpp
	Source/Utils/Sprite.h
	Source/Utils/tinyxml2.cpp
	Source/Utils/tinyxml2.h
	Source/Systems/renderer.h
)

if(WIN32)
# by default CMake selects "ALL_BUILD" as the startup project 
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
	PROPERTY VS_STARTUP_PROJECT Tyler_Clardy_Renderer)

	#this will tell Visual Studio to put the shaders under their own group in the solution explorer 
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
   	source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${VERTEX_SHADERS})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${PIXEL_SHADERS})	
endif()

# currently using unicode in some libraries on win32 but will change soon
ADD_DEFINITIONS(-DUNICODE)
ADD_DEFINITIONS(-D_UNICODE)

add_executable (Tyler_Clardy_Renderer 
	${SOURCE_CODE}
	${VERTEX_SHADERS}
	${PIXEL_SHADERS}
)

set_source_files_properties( ${VERTEX_SHADERS} PROPERTIES 
        VS_SHADER_TYPE Vertex 
        VS_SHADER_MODEL 5.0
        VS_SHADER_ENTRYPOINT main
        VS_TOOL_OVERRIDE "FXCompile" 
)
set_source_files_properties( ${PIXEL_SHADERS} PROPERTIES 
	    VS_SHADER_TYPE Pixel 
        VS_SHADER_MODEL 5.0
        VS_SHADER_ENTRYPOINT main
        VS_TOOL_OVERRIDE "FXCompile"
)

# add support for dds texture loading
include_directories(${CMAKE_SOURCE_DIR}/directxtk11/inc)
# Find the libraries
find_library(DDS_LIB_D NAMES DirectXTK11_x64_Debug PATHS ${CMAKE_SOURCE_DIR}/directxtk11/lib/)
find_library(DDS_LIB_R NAMES DirectXTK11_x64_Release PATHS ${CMAKE_SOURCE_DIR}/directxtk11/lib/)
# link the ktx sdk include and lib files
target_link_libraries(Tyler_Clardy_Renderer debug ${DDS_LIB_D} optimized ${DDS_LIB_R})
