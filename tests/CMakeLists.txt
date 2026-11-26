# PVZU: doctest unit tests for src/Mod. Run: ctest --test-dir build-mac --output-on-failure
file(GLOB PVZU_TEST_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(pvzu_tests ${PVZU_TEST_SOURCES})
target_link_libraries(pvzu_tests PRIVATE pvzu_core)
target_include_directories(pvzu_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../third_party ${CMAKE_CURRENT_SOURCE_DIR}/fixtures)
target_compile_features(pvzu_tests PRIVATE cxx_std_20)
target_compile_definitions(pvzu_tests PRIVATE PVZU_FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")
add_test(NAME pvzu_tests COMMAND pvzu_tests)
