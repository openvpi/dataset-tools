find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

set(CMAKE_AUTOMOC ON)

set(LIBS_TESTS
    TestMinLabelService
)

foreach(test_name ${LIBS_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
        minlabel-lib
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

add_executable(TestSlicerProcessor TestSlicerProcessor.cpp)
target_link_libraries(TestSlicerProcessor PRIVATE
    slicer-lib
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestSlicerProcessor COMMAND TestSlicerProcessor)
