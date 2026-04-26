message(STATUS "=== Layer Audit: Checking apps → libs → infer layering ===")

if(NOT APPS_DIR)
    message(FATAL_ERROR "APPS_DIR not set")
endif()

set(FORBIDDEN_INCLUDES
    "hubert-infer/"
    "rmvpe-infer/"
    "game-infer/"
    "audio-util/"
)

set(VIOLATIONS "")

file(GLOB_RECURSE APP_SOURCES
    "${APPS_DIR}/*.cpp"
    "${APPS_DIR}/*.h"
)

foreach(SOURCE_FILE ${APP_SOURCES})
    file(STRINGS "${SOURCE_FILE}" INCLUDE_LINES REGEX "^#include")
    foreach(LINE ${INCLUDE_LINES})
        foreach(FORBIDDEN ${FORBIDDEN_INCLUDES})
            string(FIND "${LINE}" "${FORBIDDEN}" POS)
            if(NOT POS EQUAL -1)
                string(REPLACE "${APPS_DIR}/" "" REL_PATH "${SOURCE_FILE}")
                list(APPEND VIOLATIONS "  ${REL_PATH}: ${LINE}")
            endif()
        endforeach()
    endforeach()
endforeach()

list(LENGTH VIOLATIONS NUM_VIOLATIONS)

if(NUM_VIOLATIONS GREATER 0)
    message(WARNING "Layer audit found ${NUM_VIOLATIONS} layering violations (apps should use libs, not infer directly):")
    foreach(V ${VIOLATIONS})
        message(WARNING "${V}")
    endforeach()
    message(WARNING "Run 'cmake --build . --target layer_audit' to check. Fix by using libs-layer wrappers instead of direct infer includes.")
else()
    message(STATUS "Layer audit: No violations found. All apps → libs → infer layering rules satisfied.")
endif()
