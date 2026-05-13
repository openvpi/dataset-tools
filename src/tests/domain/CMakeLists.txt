find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

set(CMAKE_AUTOMOC ON)

set(DOMAIN_TESTS
    TestF0Curve
    TestPitchUtils
    TestDsDocument
    TestDsProject
    TestPhNumCalculator
    TestTranscriptionCsv
    TestStringUtils
    TestTranscriptionPipeline
    TestTextGridToCsv
    TestCsvToDsConverter
    TestCurveTools
    TestDsTextDocument
    TestFormatAdapters
    TestProjectPaths
)

foreach(test_name ${DOMAIN_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
        dstools-domain
        dsfw-base
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endforeach()

# PitchProcessor test (links data-sources which includes pitch-editor)
add_executable(TestPitchProcessor TestPitchProcessor.cpp)
target_link_libraries(TestPitchProcessor PRIVATE
    data-sources
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestPitchProcessor COMMAND TestPitchProcessor)
set_tests_properties(TestPitchProcessor PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(TestPitchExtractionService TestPitchExtractionService.cpp)
target_link_libraries(TestPitchExtractionService PRIVATE
    data-sources
    game-infer
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestPitchExtractionService COMMAND TestPitchExtractionService)
set_tests_properties(TestPitchExtractionService PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(TestExportServiceMock TestExportServiceMock.cpp)
target_link_libraries(TestExportServiceMock PRIVATE
    data-sources
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestExportServiceMock COMMAND TestExportServiceMock)
set_tests_properties(TestExportServiceMock PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)

add_executable(TestSlicerService TestSlicerService.cpp)
target_link_libraries(TestSlicerService PRIVATE
    data-sources
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestSlicerService COMMAND TestSlicerService)
set_tests_properties(TestSlicerService PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
