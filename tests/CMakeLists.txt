add_executable(snapshot_tests snapshot_test.cpp)
target_link_libraries(snapshot_tests PRIVATE oop_lib)
target_include_directories(snapshot_tests SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/ext)
target_compile_definitions(snapshot_tests PRIVATE
    DOCTEST_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)
set_compiler_flags(RUN_SANITIZERS ${USE_ASAN} TARGET_NAMES snapshot_tests)
add_test(NAME snapshot_tests COMMAND snapshot_tests)

add_executable(unit_tests unit_test.cpp)
target_link_libraries(unit_tests PRIVATE oop_lib)
target_include_directories(unit_tests SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/ext)
set_compiler_flags(RUN_SANITIZERS ${USE_ASAN} TARGET_NAMES unit_tests)
add_test(NAME unit_tests COMMAND unit_tests)
