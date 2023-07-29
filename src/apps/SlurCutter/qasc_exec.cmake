qas_wrap_cpp(_qasc_src gui/DsSentence.h TARGET ${PROJECT_NAME})
qas_wrap_cpp(_qasc_src gui/SlurCutterCfg.h TARGET ${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${_qasc_src})