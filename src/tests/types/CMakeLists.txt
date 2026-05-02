find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

set(CMAKE_AUTOMOC ON)

set(TYPES_TESTS
    TestTimePos
)

foreach(test_name ${TYPES_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
        dstools-types::dstools-types
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
