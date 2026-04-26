# RunClangTidy.cmake - Script to run clang-tidy with automatic fixes

# Function to find all C++ source files
function(find_cpp_files root_dir result_var)
    set(file_list "")
    file(GLOB_RECURSE all_files 
        "${root_dir}/*.cpp"
        "${root_dir}/*.cc"
        "${root_dir}/*.cxx"
        "${root_dir}/*.c++"
        "${root_dir}/*.h"
        "${root_dir}/*.hh"
        "${root_dir}/*.hpp"
        "${root_dir}/*.hxx"
    )
    
    foreach(file IN LISTS all_files)
        # Exclude build directories and third-party code
        if(NOT file MATCHES "/(build|cmake-build|vcpkg|third_party|external)/")
            list(APPEND file_list "${file}")
        endif()
    endforeach()
    
    set(${result_var} "${file_list}" PARENT_SCOPE)
endfunction()

# Main script
message(STATUS "Running clang-tidy with automatic fixes...")

# Find all C++ files
find_cpp_files("${PROJECT_SOURCE_DIR}" CPP_FILES)
list(LENGTH CPP_FILES NUM_FILES)
message(STATUS "Found ${NUM_FILES} C++ files to process")

# Check if compile_commands.json exists
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/compile_commands.json")
    message(WARNING "compile_commands.json not found. Generating...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} 
            -S "${PROJECT_SOURCE_DIR}"
            -B "${PROJECT_SOURCE_DIR}/build_tidy"
            -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
            -G Ninja
        OUTPUT_QUIET
        ERROR_QUIET
    )
    
    if(EXISTS "${PROJECT_SOURCE_DIR}/build_tidy/compile_commands.json")
        file(COPY "${PROJECT_SOURCE_DIR}/build_tidy/compile_commands.json"
             DESTINATION "${PROJECT_SOURCE_DIR}")
        file(REMOVE_RECURSE "${PROJECT_SOURCE_DIR}/build_tidy")
        message(STATUS "Generated compile_commands.json")
    else()
        message(WARNING "Failed to generate compile_commands.json")
    endif()
endif()

# Run clang-tidy on each file
set(FIXED_COUNT 0)
set(ERROR_COUNT 0)
set(PROCESSED_COUNT 0)

foreach(file IN LISTS CPP_FILES)
    math(EXPR PROCESSED_COUNT "${PROCESSED_COUNT} + 1")
    
    # Get relative path for display
    file(RELATIVE_PATH rel_file "${PROJECT_SOURCE_DIR}" "${file}")
    message(STATUS "[${PROCESSED_COUNT}/${NUM_FILES}] Processing: ${rel_file}")
    
    # Run clang-tidy with fixes
    execute_process(
        COMMAND "${CLANG_TIDY_EXE}"
            "-p=${PROJECT_SOURCE_DIR}"
            "--config-file=${PROJECT_SOURCE_DIR}/.clang-tidy"
            "--fix"
            "--fix-errors"
            "${file}"
        OUTPUT_VARIABLE clang_tidy_output
        ERROR_VARIABLE clang_tidy_error
        RESULT_VARIABLE clang_tidy_result
    )
    
    # Check if fixes were applied
    if(clang_tidy_result EQUAL 0)
        # Count applied fixes
        string(REGEX MATCHALL "applied [0-9]+ fixes" fix_matches "${clang_tidy_output}")
        list(LENGTH fix_matches num_fixes)
        if(num_fixes GREATER 0)
            math(EXPR FIXED_COUNT "${FIXED_COUNT} + 1")
            message(STATUS "  Applied fixes")
        endif()
    else()
        math(EXPR ERROR_COUNT "${ERROR_COUNT} + 1")
        message(STATUS "  Error: ${clang_tidy_error}")
    endif()
endforeach()

message(STATUS "")
message(STATUS "clang-tidy results:")
message(STATUS "  Files processed: ${PROCESSED_COUNT}")
message(STATUS "  Files with fixes: ${FIXED_COUNT}")
message(STATUS "  Errors: ${ERROR_COUNT}")