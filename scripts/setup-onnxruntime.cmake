# https://github.com/diffscope/dsinfer/blob/main/scripts/setup-onnxruntime.cmake
# Usage: cmake [-Dep=GPU] -P <this>

function(get_windows_proxy _out)
    execute_process(
        COMMAND reg query "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings" /v ProxyEnable
        OUTPUT_VARIABLE _proxy_enable_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(NOT _proxy_enable_output MATCHES "ProxyEnable[ \t\r\n]+REG_DWORD[ \t\r\n]+0x1")
        return()
    endif()

    execute_process(
        COMMAND reg query "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings" /v ProxyServer
        OUTPUT_VARIABLE _proxy_server_output
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(NOT _proxy_server_output MATCHES "ProxyServer[ \t\r\n]+REG_SZ[ \t\r\n]+(.*)")
        return()
    endif()

    set(${_out} ${CMAKE_MATCH_1} PARENT_SCOPE)
endfunction()

# Detect system proxy
if(WIN32)
    set(_proxy)
    get_windows_proxy(_proxy)

    if(_proxy)
        set(ENV{HTTP_PROXY} "http://${_proxy}")
        set(ENV{HTTPS_PROXY} "http://${_proxy}")
        set(ENV{ALL_PROXY} "http://${_proxy}")
        message(STATUS "Use system proxy: ${_proxy}")
    endif()
endif()

# Download OnnxRuntime Release
set(_os)
set(_ext "tgz")

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(_os "win")
    set(_ext "zip")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    set(_os "linux")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(_os "osx")
else()
    message(FATAL_ERROR "Unsupported operating system: ${CMAKE_SYSTEM_NAME}")
endif()

if(NOT DEFINED CMAKE_HOST_SYSTEM_PROCESSOR)
    if(WIN32)
        set(_win_arch $ENV{PROCESSOR_ARCHITECTURE})

        if(_win_arch STREQUAL "AMD64")
            set(_detected_arch "x64")
        elseif(_win_arch STREQUAL "x86")
            set(_detected_arch "x86")
        elseif(_win_arch STREQUAL "ARM64")
            set(_detected_arch "ARM64")
        endif()
    elseif(APPLE)
        execute_process(COMMAND uname -m OUTPUT_VARIABLE _detected_arch OUTPUT_STRIP_TRAILING_WHITESPACE)

        if(_detected_arch STREQUAL "x86_64")
            execute_process(COMMAND sysctl -n sysctl.proc_translated OUTPUT_VARIABLE _proc_translated OUTPUT_STRIP_TRAILING_WHITESPACE)

            if(_proc_translated STREQUAL "1")
                set(_detected_arch "arm64")
            endif()
        endif()
    else()

        execute_process(COMMAND uname -m OUTPUT_VARIABLE _detected_arch OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(_detected_arch STREQUAL "x86_64")
            set(_detected_arch "x64")
        else()
            message(FATAL_ERROR "Unsupported Architecture: Linux-${_detected_arch}")
        endif()
    endif()
endif()

set(_arch ${_detected_arch})

set(_version "1.17.0")
set(_base_url "https://github.com/microsoft/onnxruntime/releases/download/v${_version}")

if(DEFINED ep AND "${ep}" STREQUAL gpu)
    set(_full_version gpu-${_version})
else()
    set(_full_version ${_version})
endif()

set(_name "onnxruntime-${_os}-${_arch}-${_full_version}")
set(_url "${_base_url}/${_name}.${_ext}")
set(_file_path "${CMAKE_BINARY_DIR}/${_name}.${_ext}")

message(STATUS "Downloading ONNX Runtime from ${_url}")

file(DOWNLOAD ${_url} ${_file_path}

    # EXPECTED_HASH SHA256=14e0b7ed6cc504f8c4c1d8e57451ada6d8469394d08e10afa6db616f082fe035
    # TIMEOUT 60
    SHOW_PROGRESS
)

set(_extract_dir ${CMAKE_BINARY_DIR}/onnxruntime)

file(ARCHIVE_EXTRACT INPUT ${_file_path}
    DESTINATION ${_extract_dir}
)
file(REMOVE ${_file_path})
file(MAKE_DIRECTORY ${_extract_dir}/${_name}/include)
file(MAKE_DIRECTORY ${_extract_dir}/${_name}/lib)
file(COPY ${_extract_dir}/${_name}/include DESTINATION ${_extract_dir})
file(COPY ${_extract_dir}/${_name}/lib DESTINATION ${_extract_dir})
file(REMOVE_RECURSE ${_extract_dir}/${_name})