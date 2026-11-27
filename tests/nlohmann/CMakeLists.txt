project(reflect-cpp-nlohmann-tests)

add_executable(
    reflect-cpp-nlohmann-tests
    test_nlohmann.cpp
    test_nlohmann_standalone.cpp
)

if (REFLECTCPP_JSON)
    target_compile_definitions(reflect-cpp-nlohmann-tests PRIVATE REFLECTCPP_HAS_JSON)
endif ()

target_link_libraries(reflect-cpp-nlohmann-tests PRIVATE reflectcpp_tests_crt)

add_custom_command(TARGET reflect-cpp-nlohmann-tests POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:reflect-cpp-nlohmann-tests> $<TARGET_RUNTIME_DLLS:reflect-cpp-nlohmann-tests>
  COMMAND_EXPAND_LISTS
)

find_package(GTest)
gtest_discover_tests(reflect-cpp-nlohmann-tests)
