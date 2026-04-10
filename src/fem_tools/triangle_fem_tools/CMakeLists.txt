add_library(triangle_fem_tools STATIC
    triangle_fem_tools.cpp
    triangle_fem_tools.hpp
)

target_link_libraries(triangle_fem_tools PRIVATE
    Qt6::Core
)

target_include_directories(
    triangle_fem_tools INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
