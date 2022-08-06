
# Generic test that uses conan libs
add_library(vcd_tracer vcd_tracer.cpp)

target_compile_features(vcd_tracer PRIVATE cxx_std_17)
