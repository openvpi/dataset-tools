find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

set(CMAKE_AUTOMOC ON)

get_filename_component(QT6_PREFIX "${Qt6_DIR}/../../.." ABSOLUTE)
set(QT_PLATFORM_PLUGIN_PATH "${QT6_PREFIX}/plugins/platforms")
set(QT_TEST_ENV "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLATFORM_PLUGIN_PATH}")

set(DOMAIN_TESTS
        TestF0Curve
        TestPitchUtils
        TestDsDocument
        TestDsProject
        TestPhNumCalculator
        TestTranscriptionCsv
        TestStringUtils
        TestTextGridToCsv
        TestCsvToDsConverter
        TestCurveTools
        TestDsTextDocument
        TestFormatAdapters
        TestProjectPaths
        TestExportFormats
        TestPinyinG2PProvider
        TestProjectBackupManager
)

foreach (test_name ${DOMAIN_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
            dstools-domain
            dsfw-base
            dsfw-signal
            Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT "${QT_TEST_ENV}"
    )
endforeach ()

# PitchProcessor test (links data-sources which includes pitch-editor)
add_executable(TestPitchProcessor TestPitchProcessor.cpp)
target_link_libraries(TestPitchProcessor PRIVATE
        data-sources
        Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestPitchProcessor COMMAND TestPitchProcessor)
set_tests_properties(TestPitchProcessor PROPERTIES
        ENVIRONMENT "${QT_TEST_ENV}"
)

add_executable(TestPitchExtractionService TestPitchExtractionService.cpp)
target_link_libraries(TestPitchExtractionService PRIVATE
        data-sources
        game-infer
        Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestPitchExtractionService COMMAND TestPitchExtractionService)
set_tests_properties(TestPitchExtractionService PROPERTIES
        ENVIRONMENT "${QT_TEST_ENV}"
)

add_executable(TestSlicerService TestSlicerService.cpp)
target_link_libraries(TestSlicerService PRIVATE
        data-sources
        slicer-lib
        Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestSlicerService COMMAND TestSlicerService)
set_tests_properties(TestSlicerService PROPERTIES
        ENVIRONMENT "${QT_TEST_ENV}"
)

add_executable(TestDsTextDocBridgeRoundtrip TestDsTextDocBridgeRoundtrip.cpp)
target_link_libraries(TestDsTextDocBridgeRoundtrip PRIVATE
        dstools-domain
        dsfw-base
        shared-bridges
        Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestDsTextDocBridgeRoundtrip COMMAND TestDsTextDocBridgeRoundtrip)
set_tests_properties(TestDsTextDocBridgeRoundtrip PROPERTIES
        ENVIRONMENT "${QT_TEST_ENV}"
)

# CompositeInferenceService test (null-engine behavior)
add_executable(TestCompositeInferenceService TestCompositeInferenceService.cpp)
target_include_directories(TestCompositeInferenceService PRIVATE
        ${CMAKE_SOURCE_DIR}/src/apps/shared/data-sources
)
target_link_libraries(TestCompositeInferenceService PRIVATE
        data-sources
        dsfw-infer
        Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestCompositeInferenceService COMMAND TestCompositeInferenceService)
set_tests_properties(TestCompositeInferenceService PROPERTIES
        ENVIRONMENT "${QT_TEST_ENV}"
)
