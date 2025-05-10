add_executable(Editor WIN32)

file(GLOB_RECURSE EDITOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE EDITOR_HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/inc/*.h")

target_sources(Editor 
    PRIVATE ${EDITOR_SOURCES}
    PRIVATE ${EDITOR_HEADERS}
)

source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/inc"
    PREFIX "Header Files"
    FILES ${EDITOR_HEADERS}
)

source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}/src"
    PREFIX "Source Files"
    FILES ${EDITOR_SOURCES}
)

set_target_properties(Editor PROPERTIES CXX_STANDARD 20)
set_target_properties(Editor PROPERTIES CMAKE_CXX_STANDARD_REQUIRED True)

target_include_directories(Editor PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

target_link_libraries(Editor Engine)