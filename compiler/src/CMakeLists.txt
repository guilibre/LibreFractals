add_library(parser STATIC
    parser/tokenizer.cpp
    parser/parser.cpp
    parser/codegen.cpp
)
target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(renderer STATIC
    renderer/svg.cpp
)
target_include_directories(renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(renderer PRIVATE parser)

add_executable(LibreFractals main.cpp)
target_link_libraries(LibreFractals PRIVATE parser renderer)
