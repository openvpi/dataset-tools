cmake_minimum_required(VERSION 3.21)

# Build tests only if BUILD_TESTS is ON
if(NOT BUILD_TESTS)
    return()
endif()

# FFmpeg already found by parent CMakeLists.txt; reuse variables
# If building tests standalone, find it:
if(NOT FFMPEG_FOUND)
    find_package(FFmpeg REQUIRED)
endif()

# ── TestAudioBuffer ──────────────────────────────────────────────────────────
add_executable(TestAudioBuffer TestAudioBuffer.cpp)
target_link_libraries(TestAudioBuffer PRIVATE dsfw-audio)
add_test(NAME TestAudioBuffer COMMAND TestAudioBuffer)

# ── TestAudioDecoder (requires FFmpeg) ───────────────────────────────────────
add_executable(TestAudioDecoder TestAudioDecoder.cpp)
target_link_libraries(TestAudioDecoder PRIVATE dsfw-audio ${FFMPEG_LIBRARIES})
add_test(NAME TestAudioDecoder COMMAND TestAudioDecoder)

# ── TestAudioResampler (requires FFmpeg) ─────────────────────────────────────
add_executable(TestAudioResampler TestAudioResampler.cpp)
target_link_libraries(TestAudioResampler PRIVATE dsfw-audio ${FFMPEG_LIBRARIES})
add_test(NAME TestAudioResampler COMMAND TestAudioResampler)

# ── TestAudioPipeline (requires FFmpeg) ──────────────────────────────────────
add_executable(TestAudioPipeline TestAudioPipeline.cpp)
target_link_libraries(TestAudioPipeline PRIVATE dsfw-audio ${FFMPEG_LIBRARIES})
add_test(NAME TestAudioPipeline COMMAND TestAudioPipeline)