project(dsfw-ui-core)

# Standalone build guards — only activate when targets are not already defined
if(NOT TARGET dsfw::core)
    find_package(dsfw CONFIG REQUIRED)
endif()

if(NOT TARGET Qt6::Core)
    find_package(QT NAMES Qt6 COMPONENTS Core Gui Widgets REQUIRED)
    find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui Widgets REQUIRED)
endif()

find_package(QWindowKit CONFIG REQUIRED)

dstools_add_library(${PROJECT_NAME}
    STATIC
    AUTOMOC AUTORCC
    NAMESPACE dsfw::ui-core
    EXPORT_SET dsfwTargets
    RESOURCES res/ui-core.qrc
    DEPENDS
        PUBLIC dsfw-core Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets QWindowKit::Widgets
    INCLUDE_DIRS
        PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../widgets/include>
)

dstools_add_translations(${PROJECT_NAME} EXPORT_SET dsfwTargets)

# Install resource files
if(NOT DSTOOLS_DEPLOY_MODE)
    include(GNUInstallDirs)
    install(FILES res/ui-core.qrc
        DESTINATION "${CMAKE_INSTALL_DATADIR}/dsfw"
    )
endif()
