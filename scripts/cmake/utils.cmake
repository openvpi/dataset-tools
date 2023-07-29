# Copied from ChorusKit
function(parse_version _prefix _version)
    string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ ${_version})

    foreach(_i RANGE 1 4)
        if(${CMAKE_MATCH_COUNT} GREATER_EQUAL ${_i})
            set(_tmp ${CMAKE_MATCH_${_i}})
        else()
            set(_tmp 0)
        endif()

        set(${_prefix}_${_i} ${_tmp} PARENT_SCOPE)
    endforeach()
endfunction()