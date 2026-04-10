add_executable(qnavierstokes
    about_window.cpp
    about_window.hpp
    about_window.ui
    ipainter.hpp
    main.cpp
    main_window.cpp
    main_window.hpp
    main_window.ui
    scientific_spinbox.cpp
    scientific_spinbox.hpp
    paint_aim.hpp
)

target_link_libraries(qnavierstokes PRIVATE
    bottom_heating
    side_heating
    opengl_painter
    color_scale
    settings
    reference
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

set_target_properties(qnavierstokes PROPERTIES
    WIN32_EXECUTABLE ON
)
