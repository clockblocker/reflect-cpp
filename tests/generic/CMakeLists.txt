project(reflect-cpp-generic-tests)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "*.cpp")
list(FILTER SOURCES EXCLUDE REGEX ".*/compile_fail/.*")

add_executable(
    reflect-cpp-generic-tests 
    ${SOURCES}
)

target_include_directories(reflect-cpp-generic-tests SYSTEM PRIVATE "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")

target_link_libraries(reflect-cpp-generic-tests PRIVATE reflectcpp_tests_crt)

add_custom_command(TARGET reflect-cpp-generic-tests POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:reflect-cpp-generic-tests> $<TARGET_RUNTIME_DLLS:reflect-cpp-generic-tests>
  COMMAND_EXPAND_LISTS
)

find_package(GTest)
gtest_discover_tests(reflect-cpp-generic-tests)

# Flatten::operator=(Flatten<U>&&) has a hard compile error for cross-type
# assignment. This test tries to compile the buggy overload.
# WILL_FAIL FALSE: test passes when compilation succeeds (bug fixed).
add_test(
  NAME regression.flatten_cross_type_move_assign_should_compile
  COMMAND ${CMAKE_COMMAND}
    --build ${CMAKE_BINARY_DIR}
    --target generic-compile-fail-flatten-cross-type-move-assign
    --config $<CONFIG>
)
set_tests_properties(
  regression.flatten_cross_type_move_assign_should_compile
  PROPERTIES WILL_FAIL FALSE
)
add_library(generic-compile-fail-flatten-cross-type-move-assign OBJECT EXCLUDE_FROM_ALL
  compile_fail/flatten_cross_type_move_assign.cpp
)
target_link_libraries(generic-compile-fail-flatten-cross-type-move-assign PRIVATE reflectcpp_tests_crt)

# Compile-time alias collision checks must fail to compile (WILL_FAIL TRUE).
add_test(
  NAME compile_fail.alias_collides_with_primary
  COMMAND ${CMAKE_COMMAND}
    --build ${CMAKE_BINARY_DIR}
    --target generic-compile-fail-alias-collides-with-primary
    --config $<CONFIG>
)
set_tests_properties(
  compile_fail.alias_collides_with_primary
  PROPERTIES WILL_FAIL TRUE
)
add_library(generic-compile-fail-alias-collides-with-primary OBJECT EXCLUDE_FROM_ALL
  compile_fail/alias_collides_with_primary.cpp
)
target_link_libraries(generic-compile-fail-alias-collides-with-primary PRIVATE reflectcpp_tests_crt)

add_test(
  NAME compile_fail.alias_collides_with_alias
  COMMAND ${CMAKE_COMMAND}
    --build ${CMAKE_BINARY_DIR}
    --target generic-compile-fail-alias-collides-with-alias
    --config $<CONFIG>
)
set_tests_properties(
  compile_fail.alias_collides_with_alias
  PROPERTIES WILL_FAIL TRUE
)
add_library(generic-compile-fail-alias-collides-with-alias OBJECT EXCLUDE_FROM_ALL
  compile_fail/alias_collides_with_alias.cpp
)
target_link_libraries(generic-compile-fail-alias-collides-with-alias PRIVATE reflectcpp_tests_crt)

