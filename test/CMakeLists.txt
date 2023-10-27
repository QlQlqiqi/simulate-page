add_executable(kv_test ${ROOT_DIR}/test/kv_test.cpp)
# target_link_libraries(kv_test
#   kv_lib
# )
target_compile_definitions(kv_test PRIVATE DISK_PATH="${ROOT_DIR}/test/disk")