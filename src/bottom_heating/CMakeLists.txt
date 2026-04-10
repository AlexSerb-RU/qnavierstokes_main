add_library(bottom_heating STATIC
    bottom_heating_solver.cpp
    bottom_heating_solver.hpp
)

target_link_libraries(bottom_heating PRIVATE
    Qt6::Core
)

target_include_directories(
    bottom_heating INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
