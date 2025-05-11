add_executable(Ashenvale WIN32)

file(GLOB_RECURSE ASHENVALE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE ASHENVALE_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

target_sources(Ashenvale 
    PRIVATE ${ASHENVALE_HEADERS}
    PRIVATE ${ASHENVALE_SOURCES}
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

target_include_directories(Ashenvale PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

set_target_properties(Ashenvale PROPERTIES CXX_STANDARD 20)
set_target_properties(Ashenvale PROPERTIES CMAKE_CXX_STANDARD_REQUIRED True)

if(WIN32)
    target_link_libraries(Ashenvale d3d11 dxguid dxgi) 
endif()
