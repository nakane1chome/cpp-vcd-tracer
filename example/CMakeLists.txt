include(CTest)

add_executable(signals signals.cpp)
target_link_libraries(signals PRIVATE project_warnings project_options vcd_tracer)
target_compile_features(signals PRIVATE cxx_std_17)

add_test(NAME signals_example COMMAND signals /dev/null)
