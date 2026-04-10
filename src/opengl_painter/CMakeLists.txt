add_library(opengl_painter STATIC
    opengl_painter.cpp
    opengl_painter.hpp
)

target_link_libraries(opengl_painter PRIVATE
    color_scale
    settings
    ${OPENGL_LIBRARIES}
    Qt6::Core
    Qt6::Gui
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

target_include_directories(
    opengl_painter INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
