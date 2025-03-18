qas_wrap_cpp(_qasc_src inc/SomeCfg.h TARGET ${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE ${_qasc_src})