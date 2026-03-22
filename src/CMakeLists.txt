
# Generic test that uses conan libs
add_library(vcd_tracer vcd_tracer.cpp)

target_compile_features(vcd_tracer PRIVATE cxx_std_17)
target_link_libraries(vcd_tracer PRIVATE project_options project_warnings)
