find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

set(CMAKE_AUTOMOC ON)

set(DOMAIN_TESTS
    TestF0Curve
    TestPitchUtils
    TestDsDocument
    TestPhNumCalculator
    TestTranscriptionCsv
    TestStringUtils
    TestTranscriptionPipeline
    TestTextGridToCsv
    TestCsvToDsConverter
)

foreach(test_name ${DOMAIN_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
        dstools-domain
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# PitchProcessor test (links pitchlabeler-page instead of dstools-domain)
add_executable(TestPitchProcessor TestPitchProcessor.cpp)
target_link_libraries(TestPitchProcessor PRIVATE
    pitchlabeler-page
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestPitchProcessor COMMAND TestPitchProcessor)
