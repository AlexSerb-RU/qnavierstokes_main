add_library(side_heating STATIC
    side_heating_solver.cpp
    side_heating_solver.hpp
)

target_link_libraries(side_heating PRIVATE
    los
    csr_tools
    triangle_fem_tools
    Qt6::Core
)

target_include_directories(
    side_heating INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
)
