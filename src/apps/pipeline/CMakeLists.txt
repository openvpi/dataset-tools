project(DatasetPipeline VERSION 1.0.0.0)

add_subdirectory(slicer)
add_subdirectory(lyricfa)
add_subdirectory(hubertfa)
add_subdirectory(pages)

dstools_add_executable(${PROJECT_NAME}
    VERSION ${PROJECT_VERSION}
    AUTOMOC AUTORCC AUTOUIC
    DEPLOY MACOSX_BUNDLE WINRC
    SOURCES
        main.cpp
        PipelinePage.h
        PipelinePage.cpp
    DEPENDS
        PRIVATE dstools-ui-core pipeline-pages dstools-domain dsfw-ui-core
                $<$<BOOL:${ONNXRUNTIME_ENABLE_DML}>:dxgi>
)

if(ONNXRUNTIME_ENABLE_DML)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ONNXRUNTIME_ENABLE_DML)
endif()

# DatasetPipeline uses WIN32 unconditionally (set in original)
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
