# Unified test entry point
# Individual test targets are built by their respective infer libraries.
# This file registers them with CTest for `ctest` discovery.

include(CTest)

if (TARGET TestGame)
    add_test(NAME TestGame COMMAND TestGame)
endif()

if (TARGET TestSome)
    add_test(NAME TestSome COMMAND TestSome)
endif()

if (TARGET TestRmvpe)
    add_test(NAME TestRmvpe COMMAND TestRmvpe)
endif()

if (TARGET TestAudioUtil)
    add_test(NAME TestAudioUtil COMMAND TestAudioUtil)
endif()

add_subdirectory(framework)
add_subdirectory(domain)
add_subdirectory(widgets)
