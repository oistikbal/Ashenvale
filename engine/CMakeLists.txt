add_library(Engine STATIC)

file(GLOB_RECURSE ENGINE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE ENGINE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h")

target_sources(Engine 
    PRIVATE ${ENGINE_HEADERS}
    PRIVATE ${ENGINE_SOURCES}
)

source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    PREFIX "Header Files"
    FILES ${ENGINE_HEADERS}
)

source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PREFIX "Source Files"
    FILES ${ENGINE_SOURCES}
)

set_target_properties(Engine PROPERTIES CXX_STANDARD 20)
set_target_properties(Engine PROPERTIES CMAKE_CXX_STANDARD_REQUIRED True)

target_include_directories(Engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc)

if(WIN32)
    target_link_libraries(Engine d3d11 dxguid dxgi) 
endif()
