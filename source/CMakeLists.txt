file(GLOB_RECURSE ASHENVALE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE ASHENVALE_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB_RECURSE ASHENVALE_CONFIGURES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/*.h.in")
file(GLOB_RECURSE ASHENVALE_GENERATEDS CONFIGURE_DEPENDS "${CMAKE_BINARY_DIR}/generated/*.h")
file(GLOB_RECURSE ASHENVALE_SHADERS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/shaders/*.hlsl")

set_source_files_properties(${ASHENVALE_SHADERS} PROPERTIES VS_TOOL_OVERRIDE "text")
set_source_files_properties(${ASHENVALE_CONFIGURES} PROPERTIES VS_TOOL_OVERRIDE "text")

configure_file(
    "${CMAKE_SOURCE_DIR}/configs/config.h.in"
    "${CMAKE_BINARY_DIR}/generated/configs/config.h"
    @ONLY
)

add_executable(Ashenvale WIN32)

target_sources(Ashenvale PRIVATE
     ${ASHENVALE_HEADERS}
     ${ASHENVALE_SOURCES}
     ${ASHENVALE_SHADERS}
     ${ASHENVALE_GENERATEDS}
     ${ASHENVALE_CONFIGURES}
)

source_group(TREE "${CMAKE_SOURCE_DIR}" PREFIX "Source" FILES ${ASHENVALE_SOURCES} ${ASHENVALE_HEADERS} ${ASHENVALE_SHADERS} ${ASHENVALE_CONFIGURES})
source_group(TREE "${CMAKE_BINARY_DIR}/generated" PREFIX "Generated" FILES ${ASHENVALE_GENERATEDS})

target_include_directories(Ashenvale PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(Ashenvale PRIVATE "${CMAKE_BINARY_DIR}/generated")
target_include_directories(Ashenvale PRIVATE "${CMAKE_SOURCE_DIR}/thirdparty/WinPixEventRuntime/Include")
target_include_directories(Ashenvale PRIVATE "${CMAKE_SOURCE_DIR}/thirdparty/dxcompiler/inc/")
target_include_directories(Ashenvale PRIVATE "${CMAKE_SOURCE_DIR}/thirdparty/stb")


set_target_properties(Ashenvale PROPERTIES CXX_STANDARD 20)
set_target_properties(Ashenvale PROPERTIES CXX_STANDARD_REQUIRED True)

target_link_libraries(Ashenvale flecs::flecs_static)
target_link_libraries(Ashenvale D3D12MemoryAllocator)
target_link_libraries(Ashenvale fastgltf)

if(WIN32)
    target_link_libraries(Ashenvale d3d12 dxguid dxgi windowsapp) 
    target_link_libraries(Ashenvale "${CMAKE_SOURCE_DIR}/thirdparty/WinPixEventRuntime/bin/x64/WinPixEventRuntime.lib")
    target_link_libraries(Ashenvale "${CMAKE_SOURCE_DIR}/thirdparty/dxcompiler/lib/x64/dxil.lib")
    target_link_libraries(Ashenvale "${CMAKE_SOURCE_DIR}/thirdparty/dxcompiler/lib/x64/dxcompiler.lib")
    target_compile_options(Ashenvale PRIVATE /utf-8)

    add_custom_command(TARGET Ashenvale POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/thirdparty/WinPixEventRuntime/bin/x64/WinPixEventRuntime.dll"
        $<TARGET_FILE_DIR:Ashenvale>)

    add_custom_command(TARGET Ashenvale POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/thirdparty/dxcompiler/bin/x64/dxcompiler.dll"
        $<TARGET_FILE_DIR:Ashenvale>)

    add_custom_command(TARGET Ashenvale POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/thirdparty/dxcompiler/bin/x64/dxil.dll"
        $<TARGET_FILE_DIR:Ashenvale>)


    add_custom_command(TARGET Ashenvale POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/resources/"
        "$<TARGET_FILE_DIR:Ashenvale>/resources/"
    )
    add_custom_command(TARGET Ashenvale POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders/"
        "$<TARGET_FILE_DIR:Ashenvale>/shaders/"
    )

endif()
