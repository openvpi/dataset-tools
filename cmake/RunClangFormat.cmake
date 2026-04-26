# RunClangFormat.cmake - Script to format all source files with clang-format

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
message(STATUS "Formatting source files with clang-format...")

# Find all C++ files
find_cpp_files("${PROJECT_SOURCE_DIR}" CPP_FILES)
list(LENGTH CPP_FILES NUM_FILES)
message(STATUS "Found ${NUM_FILES} C++ files to format")

# Run clang-format on each file
set(FORMATTED_COUNT 0)
set(ERROR_COUNT 0)
set(PROCESSED_COUNT 0)

foreach(file IN LISTS CPP_FILES)
    math(EXPR PROCESSED_COUNT "${PROCESSED_COUNT} + 1")
    
    # Get relative path for display
    file(RELATIVE_PATH rel_file "${PROJECT_SOURCE_DIR}" "${file}")
    message(STATUS "[${PROCESSED_COUNT}/${NUM_FILES}] Formatting: ${rel_file}")
    
    # First check if formatting is needed
    execute_process(
        COMMAND "${CLANG_FORMAT_EXE}"
            "--style=file"
            "--dry-run"
            "--Werror"
            "${file}"
        OUTPUT_QUIET
        ERROR_QUIET
        RESULT_VARIABLE check_result
    )
    
    if(check_result EQUAL 0)
        message(STATUS "  Already formatted")
    else()
        # Format the file
        execute_process(
            COMMAND "${CLANG_FORMAT_EXE}"
                "--style=file"
                "-i"
                "${file}"
            RESULT_VARIABLE format_result
        )
        
        if(format_result EQUAL 0)
            math(EXPR FORMATTED_COUNT "${FORMATTED_COUNT} + 1")
            message(STATUS "  Formatted")
        else()
            math(EXPR ERROR_COUNT "${ERROR_COUNT} + 1")
            message(STATUS "  Formatting failed")
        endif()
    endif()
endforeach()

message(STATUS "")
message(STATUS "clang-format results:")
message(STATUS "  Files processed: ${PROCESSED_COUNT}")
message(STATUS "  Files formatted: ${FORMATTED_COUNT}")
message(STATUS "  Errors: ${ERROR_COUNT}")