# Unified test entry point
# Individual test targets are built by their respective infer libraries.
# This file registers them with CTest for `ctest` discovery.

include(CTest)

# ── Infer tests (controlled by per-library options) ──────────────────────────
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

# ── Unit tests (always built when BUILD_TESTS=ON) ────────────────────────────
add_subdirectory(types)
add_subdirectory(framework)
add_subdirectory(domain)
add_subdirectory(libs)

# ── Integration tests (require QApplication / UI, off by default) ────────────
option(BUILD_INTEGRATION_TESTS "Build integration and widget tests (require GUI)" OFF)
if(BUILD_INTEGRATION_TESTS)
    add_subdirectory(widgets)
endif()
