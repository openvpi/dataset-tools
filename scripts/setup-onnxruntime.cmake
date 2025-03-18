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

set(_version "1.17.3")
set(_version_dml "1.15.0")

macro(download_onnxruntime_from_github)
    set(_base_url "https://github.com/microsoft/onnxruntime/releases/download/v${_version}")
    set(_name      "onnxruntime-${_os}-${_arch}-${_full_version}")
    set(_name_zip  "onnxruntime-${_os}-${_arch}-${_full_version_zip}")
    set(_url       "${_base_url}/${_name}.${_ext}")
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
    file(MAKE_DIRECTORY ${_extract_dir}/${_name_zip}/include)
    file(MAKE_DIRECTORY ${_extract_dir}/${_name_zip}/lib)
    file(COPY ${_extract_dir}/${_name_zip}/include DESTINATION ${_extract_dir})
    file(COPY ${_extract_dir}/${_name_zip}/lib DESTINATION ${_extract_dir})
    file(REMOVE_RECURSE ${_extract_dir}/${_name_zip})
endmacro()

function(copy_contents _src _dst)
    file(GLOB SOURCE_FILES "${_src}/*")
    file(MAKE_DIRECTORY ${_dst})
    foreach(FILE ${SOURCE_FILES})
        file(COPY ${FILE} DESTINATION ${_dst})
    endforeach()
endfunction()

macro(download_onnxruntime_from_nuget)
    set(_url_ort "https://www.nuget.org/api/v2/package/Microsoft.ML.OnnxRuntime.DirectML/${_version}")
    set(_name_zip_ort "Microsoft.ML.OnnxRuntime.DirectML.${_version}.zip")
    set(_file_path_ort "${CMAKE_BINARY_DIR}/${_name_zip_ort}")
    message(STATUS "Downloading ONNX Runtime from ${_url_ort}")

    file(DOWNLOAD ${_url_ort} ${_file_path_ort}
            SHOW_PROGRESS
    )

    set(_extract_dir ${CMAKE_BINARY_DIR}/onnxruntime)
    set(_extract_dir_ort ${CMAKE_BINARY_DIR}/nuget_onnxruntime)

    file(MAKE_DIRECTORY ${_extract_dir_ort})

    file(ARCHIVE_EXTRACT INPUT ${_file_path_ort}
            DESTINATION ${_extract_dir_ort}
    )
    file(REMOVE ${_file_path_ort})

    file(COPY ${_extract_dir_ort}/build/native/include DESTINATION ${_extract_dir})
    file(MAKE_DIRECTORY "${extract_dir}/lib")
    copy_contents("${_extract_dir_ort}/runtimes/win-x64/native" "${_extract_dir}/lib")

    file(REMOVE_RECURSE ${_extract_dir_ort})
endmacro()

macro(download_dml_from_nuget)
    set(_url_dml "https://www.nuget.org/api/v2/package/Microsoft.AI.DirectML/${_version_dml}")
    set(_name_zip_dml "Microsoft.AI.DirectML.${_version_dml}.zip")
    set(_file_path_dml "${CMAKE_BINARY_DIR}/${_name_zip_dml}")
    message(STATUS "Downloading DirectML from ${_url_dml}")

    file(DOWNLOAD ${_url_dml} ${_file_path_dml}
            SHOW_PROGRESS
    )

    set(_extract_dir ${CMAKE_BINARY_DIR}/onnxruntime)
    set(_extract_dir_dml ${CMAKE_BINARY_DIR}/nuget_directml)

    file(MAKE_DIRECTORY ${_extract_dir_dml})

    file(ARCHIVE_EXTRACT INPUT ${_file_path_dml}
            DESTINATION ${_extract_dir_dml}
    )
    file(REMOVE ${_file_path_dml})

    file(COPY ${_extract_dir_dml}/include DESTINATION ${_extract_dir})
    file(MAKE_DIRECTORY "${extract_dir}/lib")
    copy_contents("${_extract_dir_dml}/bin/x64-win" "${_extract_dir}/lib")

    file(REMOVE_RECURSE ${_extract_dir_dml})
endmacro()

if(DEFINED ep AND "${ep}" STREQUAL gpu)
    set(_full_version      gpu-${_version})
    set(_full_version_zip  gpu-${_version})
    download_onnxruntime_from_github()
elseif(DEFINED ep AND "${ep}" STREQUAL gpu-cuda12)
    set(_full_version      gpu-cuda12-${_version})
    set(_full_version_zip  gpu-${_version})
    download_onnxruntime_from_github()
elseif(DEFINED ep AND "${ep}" STREQUAL dml)
    download_onnxruntime_from_nuget()
    download_dml_from_nuget()
else()
    set(_full_version      ${_version})
    set(_full_version_zip  ${_version})
    download_onnxruntime_from_github()
endif()
