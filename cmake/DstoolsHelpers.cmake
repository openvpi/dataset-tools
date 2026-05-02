#[[
  DstoolsHelpers.cmake — Helper functions for dataset-tools project targets.

  Usage examples:

    # Static framework library with Qt, namespace alias, and install export:
    dstools_add_library(dsfw-core
        STATIC
        AUTOMOC
        NAMESPACE dsfw::core
        EXPORT_SET dsfwTargets
        DEPENDS
            PUBLIC dsfw-base dstools-types::dstools-types Qt6::Core Qt6::Network nlohmann_json::nlohmann_json
            PRIVATE $<$<PLATFORM_ID:Windows>:dbghelp>
    )

    # Interface (header-only) library:
    dstools_add_library(dstools-types
        INTERFACE
        NAMESPACE dstools-types::dstools-types
        EXPORT_SET dstools-typesTargets
    )

    # Application executable with deploy and platform settings:
    dstools_add_executable(GameInfer
        VERSION 0.1.0.0
        AUTOMOC AUTORCC AUTOUIC
        DEPLOY WIN32_EXECUTABLE MACOSX_BUNDLE WINRC
        DEPENDS
            PRIVATE dstools-ui-core dstools-widgets dsfw-ui-core game-infer::game-infer Qt6::Core Qt6::Widgets
    )

    # Application with sub-library subdirectories (explicit sources to avoid double-compilation):
    dstools_add_executable(PitchLabeler
        VERSION 0.1.0.0
        AUTOMOC AUTORCC AUTOUIC
        DEPLOY WIN32_EXECUTABLE MACOSX_BUNDLE WINRC
        SOURCES main.cpp PitchLabelerKeys.h
        DEPENDS
            PRIVATE dstools-ui-core pitchlabeler-page
    )
]]

# --------------------------------------------------------------------------- #
#  dstools_add_library
# --------------------------------------------------------------------------- #
function(dstools_add_library target_name)
    set(_options STATIC SHARED INTERFACE AUTOMOC AUTORCC AUTOUIC NO_PCH)
    set(_one_value NAMESPACE EXPORT_SET CXX_STANDARD)
    set(_multi_value DEPENDS DEFINITIONS INCLUDE_DIRS RESOURCES)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    # --- Determine library type ------------------------------------------------
    if(ARG_INTERFACE)
        set(_type INTERFACE)
    elseif(ARG_SHARED)
        set(_type SHARED)
    elseif(ARG_STATIC)
        set(_type STATIC)
    else()
        message(FATAL_ERROR "dstools_add_library(${target_name}): must specify STATIC, SHARED, or INTERFACE")
    endif()

    # --- Detect source layout --------------------------------------------------
    set(_has_include_dir FALSE)
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include")
        set(_has_include_dir TRUE)
    endif()

    # --- Collect sources (non-INTERFACE only) -----------------------------------
    if(NOT _type STREQUAL "INTERFACE")
        set(_sources "")
        if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src")
            file(GLOB_RECURSE _src_sources CONFIGURE_DEPENDS
                "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
            )
            list(APPEND _sources ${_src_sources})
        endif()
        if(_has_include_dir)
            file(GLOB_RECURSE _inc_headers CONFIGURE_DEPENDS
                "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
            )
            list(APPEND _sources ${_inc_headers})
        endif()
        if(NOT _has_include_dir)
            # Flat layout (libs/ pattern): glob from current dir
            file(GLOB_RECURSE _flat_sources CONFIGURE_DEPENDS
                "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
                "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
            )
            list(APPEND _sources ${_flat_sources})
        endif()

        # Add .qrc resources
        if(ARG_RESOURCES)
            list(APPEND _sources ${ARG_RESOURCES})
        endif()

        if(NOT _sources)
            message(WARNING "dstools_add_library(${target_name}): no sources found")
        endif()

        add_library(${target_name} ${_type} ${_sources})
    else()
        add_library(${target_name} INTERFACE)
    endif()

    # --- Namespace alias -------------------------------------------------------
    if(ARG_NAMESPACE)
        add_library(${ARG_NAMESPACE} ALIAS ${target_name})
    endif()

    # --- C++ standard ----------------------------------------------------------
    set(_std "${ARG_CXX_STANDARD}")
    if(NOT _std)
        set(_std 20)
    endif()
    if(_type STREQUAL "INTERFACE")
        target_compile_features(${target_name} INTERFACE cxx_std_${_std})
    else()
        target_compile_features(${target_name} PUBLIC cxx_std_${_std})
    endif()

    # --- Compiler warnings (non-INTERFACE) -------------------------------------
    if(NOT _type STREQUAL "INTERFACE")
        if(MSVC)
            target_compile_options(${target_name} PRIVATE /utf-8 /W4 /wd4251 /MP)
        else()
            target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
        endif()
    endif()

    # --- Include directories ---------------------------------------------------
    if(_type STREQUAL "INTERFACE")
        if(_has_include_dir)
            target_include_directories(${target_name} INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
            )
        else()
            target_include_directories(${target_name} INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            )
        endif()
    else()
        if(_has_include_dir)
            target_include_directories(${target_name} PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
            )
        else()
            target_include_directories(${target_name} PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
            )
        endif()
    endif()

    # --- Auto-tools (AUTOMOC / AUTORCC / AUTOUIC) ------------------------------
    if(NOT _type STREQUAL "INTERFACE")
        if(ARG_AUTOMOC)
            set_target_properties(${target_name} PROPERTIES AUTOMOC ON)
        endif()
        if(ARG_AUTORCC)
            set_target_properties(${target_name} PROPERTIES AUTORCC ON)
        endif()
        if(ARG_AUTOUIC)
            set_target_properties(${target_name} PROPERTIES AUTOUIC ON)
        endif()
    endif()

    # --- Dependencies (DEPENDS PUBLIC ... PRIVATE ...) -------------------------
    if(ARG_DEPENDS)
        _dstools_parse_visibility("${ARG_DEPENDS}" _pub_deps _priv_deps)
        if(_type STREQUAL "INTERFACE")
            # INTERFACE libs can only use INTERFACE visibility
            set(_all_deps ${_pub_deps} ${_priv_deps})
            if(_all_deps)
                target_link_libraries(${target_name} INTERFACE ${_all_deps})
            endif()
        else()
            if(_pub_deps)
                target_link_libraries(${target_name} PUBLIC ${_pub_deps})
            endif()
            if(_priv_deps)
                target_link_libraries(${target_name} PRIVATE ${_priv_deps})
            endif()
        endif()
    endif()

    # --- Compile definitions (DEFINITIONS PUBLIC ... PRIVATE ...) --------------
    if(ARG_DEFINITIONS)
        _dstools_parse_visibility("${ARG_DEFINITIONS}" _pub_defs _priv_defs)
        if(_type STREQUAL "INTERFACE")
            set(_all_defs ${_pub_defs} ${_priv_defs})
            if(_all_defs)
                target_compile_definitions(${target_name} INTERFACE ${_all_defs})
            endif()
        else()
            if(_pub_defs)
                target_compile_definitions(${target_name} PUBLIC ${_pub_defs})
            endif()
            if(_priv_defs)
                target_compile_definitions(${target_name} PRIVATE ${_priv_defs})
            endif()
        endif()
    endif()

    # --- Extra include directories (INCLUDE_DIRS) ------------------------------
    if(ARG_INCLUDE_DIRS)
        _dstools_parse_visibility("${ARG_INCLUDE_DIRS}" _pub_incs _priv_incs)
        if(_type STREQUAL "INTERFACE")
            set(_all_incs ${_pub_incs} ${_priv_incs})
            if(_all_incs)
                target_include_directories(${target_name} INTERFACE ${_all_incs})
            endif()
        else()
            if(_pub_incs)
                target_include_directories(${target_name} PUBLIC ${_pub_incs})
            endif()
            if(_priv_incs)
                target_include_directories(${target_name} PRIVATE ${_priv_incs})
            endif()
        endif()
    endif()

    # --- Install / export rules ------------------------------------------------
    # NOTE: This only registers the target in the export set and installs
    # headers. The export set itself (install(EXPORT ...)) must be handled
    # by the aggregating CMakeLists.txt (e.g. src/framework/CMakeLists.txt)
    # to avoid duplicate installs when multiple targets share an export set.
    if(ARG_EXPORT_SET)
        include(GNUInstallDirs)

        install(TARGETS ${target_name}
            EXPORT ${ARG_EXPORT_SET}
            LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )

        if(_has_include_dir)
            install(DIRECTORY include/
                DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
            )
        endif()
    endif()

    # --- Precompiled headers (PCH) ---------------------------------------------
    if(NOT _type STREQUAL "INTERFACE" AND NOT ARG_NO_PCH)
        set(_pch_headers
            <vector>
            <map>
            <memory>
            <string>
            <functional>
            <cstdint>
        )
        if(ARG_AUTOMOC)
            list(PREPEND _pch_headers
                <QString>
                <QStringList>
                <QObject>
                <QDebug>
            )
        endif()
        target_precompile_headers(${target_name} PRIVATE ${_pch_headers})
    endif()
endfunction()

# --------------------------------------------------------------------------- #
#  dstools_add_executable
# --------------------------------------------------------------------------- #
function(dstools_add_executable target_name)
    set(_options DEPLOY WIN32_EXECUTABLE MACOSX_BUNDLE WINRC AUTOMOC AUTORCC AUTOUIC NO_PCH)
    set(_one_value VERSION)
    set(_multi_value DEPENDS SOURCES)
    cmake_parse_arguments(ARG "${_options}" "${_one_value}" "${_multi_value}" ${ARGN})

    # --- Collect sources -------------------------------------------------------
    if(ARG_SOURCES)
        # Explicit source list — use as-is (for apps with sub-library subdirectories)
        set(_sources ${ARG_SOURCES})
    else()
        # Auto-glob all sources in current directory
        file(GLOB_RECURSE _sources CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
        )
    endif()
    if(NOT _sources)
        message(WARNING "dstools_add_executable(${target_name}): no sources found")
    endif()

    add_executable(${target_name} ${_sources})

    # --- Include dirs ----------------------------------------------------------
    target_include_directories(${target_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

    # --- C++ standard ----------------------------------------------------------
    target_compile_features(${target_name} PRIVATE cxx_std_20)

    # --- Compiler warnings -----------------------------------------------------
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /utf-8 /W4 /wd4251 /MP)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    # --- Auto-tools ------------------------------------------------------------
    if(ARG_AUTOMOC)
        set_target_properties(${target_name} PROPERTIES AUTOMOC ON)
    endif()
    if(ARG_AUTORCC)
        set_target_properties(${target_name} PROPERTIES AUTORCC ON)
    endif()
    if(ARG_AUTOUIC)
        set_target_properties(${target_name} PROPERTIES AUTOUIC ON)
    endif()

    # --- Version ---------------------------------------------------------------
    if(ARG_VERSION)
        target_compile_definitions(${target_name} PRIVATE APP_VERSION="${ARG_VERSION}")
    endif()

    # --- Dependencies ----------------------------------------------------------
    if(ARG_DEPENDS)
        _dstools_parse_visibility("${ARG_DEPENDS}" _pub_deps _priv_deps)
        # Executables: treat all as PRIVATE
        set(_all_deps ${_pub_deps} ${_priv_deps})
        if(_all_deps)
            target_link_libraries(${target_name} PRIVATE ${_all_deps})
        endif()
    endif()

    # --- Platform flags --------------------------------------------------------
    if(ARG_WIN32_EXECUTABLE AND WIN32)
        if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
            set_target_properties(${target_name} PROPERTIES WIN32_EXECUTABLE TRUE)
        endif()
    endif()

    if(ARG_MACOSX_BUNDLE AND APPLE)
        set_target_properties(${target_name} PROPERTIES MACOSX_BUNDLE TRUE)
    endif()

    # --- Windows resource file -------------------------------------------------
    if(ARG_WINRC AND WIN32)
        include(${PROJECT_CMAKE_MODULES_DIR}/winrc.cmake)
    endif()

    # --- Deploy ----------------------------------------------------------------
    if(ARG_DEPLOY)
        set_property(TARGET DeployedTargets APPEND PROPERTY TARGETS ${target_name})
    endif()

    # --- Precompiled headers (PCH) ---------------------------------------------
    if(NOT ARG_NO_PCH)
        set(_pch_headers
            <vector>
            <map>
            <memory>
            <string>
            <functional>
            <cstdint>
        )
        if(ARG_AUTOMOC)
            list(PREPEND _pch_headers
                <QString>
                <QStringList>
                <QObject>
                <QDebug>
            )
        endif()
        target_precompile_headers(${target_name} PRIVATE ${_pch_headers})
    endif()
endfunction()

# --------------------------------------------------------------------------- #
#  Internal: Parse "PUBLIC a b PRIVATE c d" into two lists
# --------------------------------------------------------------------------- #
function(_dstools_parse_visibility items out_public out_private)
    set(_pub "")
    set(_priv "")
    set(_current "_pub")

    foreach(_item IN LISTS items)
        if(_item STREQUAL "PUBLIC")
            set(_current "_pub")
        elseif(_item STREQUAL "PRIVATE")
            set(_current "_priv")
        else()
            list(APPEND ${_current} "${_item}")
        endif()
    endforeach()

    set(${out_public} "${_pub}" PARENT_SCOPE)
    set(${out_private} "${_priv}" PARENT_SCOPE)
endfunction()
