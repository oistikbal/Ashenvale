# Define the executable
add_executable(Ashenvale WIN32)

file(GLOB_RECURSE ASHENVALE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE ASHENVALE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE ASHENVALE_CONFIGURES "${CMAKE_SOURCE_DIR}/*.h.in")
file(GLOB_RECURSE ASHENVALE_SHADERS "${CMAKE_SOURCE_DIR}/shaders/*.hlsl")

set_source_files_properties(${ASHENVALE_SHADERS} PROPERTIES VS_TOOL_OVERRIDE "text")

set(SHADER_DIR_DEBUG "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_DIR_RELEASE "/shaders")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(SHADER_PATH "${SHADER_DIR_RELEASE}/")
else()
    set(SHADER_PATH "${SHADER_DIR_DEBUG}/")
endif()


configure_file(
    "${CMAKE_SOURCE_DIR}/configs/shader.h.in"
    "${CMAKE_BINARY_DIR}/generated/configs/shader.h"
    @ONLY
)

file(GLOB_RECURSE ASHENVALE_GENERATEDS "${CMAKE_BINARY_DIR}/generated/*.h")

target_sources(Ashenvale 
    PRIVATE ${ASHENVALE_HEADERS}
    PRIVATE ${ASHENVALE_SOURCES}
    PRIVATE ${ASHENVALE_SHADERS}
    PRIVATE ${ASHENVALE_GENERATEDS}
)

source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    PREFIX "Header Files"
    FILES ${ASHENVALE_HEADERS}
)

source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    PREFIX "Source Files"
    FILES ${ASHENVALE_SOURCES}
)

source_group(
    TREE "${CMAKE_SOURCE_DIR}/shaders"
    PREFIX "Shaders"
    FILES ${ASHENVALE_SHADERS}
)
source_group(
    TREE "${CMAKE_BINARY_DIR}/generated/configs/"
    PREFIX "Configs"
    FILES ${ASHENVALE_GENERATEDS}
)

add_library(ImGui STATIC 
    "${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui.cpp"
    "${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_draw.cpp"
    "${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_tables.cpp"
    "${CMAKE_SOURCE_DIR}/thirdparty/imgui/imgui_widgets.cpp"
    "${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_win32.cpp"
    "${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_dx11.cpp"
)

target_include_directories(ImGui PRIVATE 
"${CMAKE_SOURCE_DIR}/thirdparty/imgui/backends"
"${CMAKE_SOURCE_DIR}/thirdparty/imgui"
)


target_include_directories(Ashenvale PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Ashenvale PRIVATE "${CMAKE_BINARY_DIR}/generated")
target_include_directories(Ashenvale PRIVATE "${CMAKE_SOURCE_DIR}/thirdparty/WinPixEventRuntime/Include")
target_include_directories(Ashenvale PRIVATE "${CMAKE_SOURCE_DIR}/thirdparty/imgui")
target_include_directories(Ashenvale PRIVATE "${CMAKE_SOURCE_DIR}/thirdparty/")

add_definitions(-DNOMINMAX)

set_target_properties(Ashenvale PROPERTIES CXX_STANDARD 20)
set_target_properties(Ashenvale PROPERTIES CXX_STANDARD_REQUIRED True)

target_link_libraries(Ashenvale flecs::flecs_static)


if(WIN32)
    target_link_libraries(Ashenvale d3d11 dxguid dxgi d3dcompiler ImGui "${CMAKE_SOURCE_DIR}/thirdparty/WinPixEventRuntime/bin/x64/WinPixEventRuntime.lib") 
endif()

add_custom_command(TARGET Ashenvale POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/thirdparty/WinPixEventRuntime/bin/x64/WinPixEventRuntime.dll"
        $<TARGET_FILE_DIR:Ashenvale>
)
