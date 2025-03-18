project(TestRmvpe)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
        rmvpe-infer::rmvpe-infer
)