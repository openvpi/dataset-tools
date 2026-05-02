project(pitchlabeler-page)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC
    INCLUDE_DIRS
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ui>
               $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    DEPENDS
        PUBLIC dstools-widgets dstools-domain dsfw-ui-core pitch-editor
               Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets
)
