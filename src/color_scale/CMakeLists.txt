add_library(color_scale STATIC
    color_scale.cpp
    color_scale.hpp
)

target_link_libraries(color_scale PRIVATE
    settings
    Qt6::Gui
)

target_include_directories(
    color_scale INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
