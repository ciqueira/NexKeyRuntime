# NOT buildable as-is today: NexKeyRuntime has not published a binary
# Release yet, so there is no `NexKeyRuntimeConfig.cmake` for find_package
# to locate. This documents the intended consumption pattern once one
# exists -- the same shape the private monorepo's own package-consumer
# test already uses against a locally installed build
# (MCSDK/tests/package-consumer/CMakeLists.txt).

cmake_minimum_required(VERSION 3.20)

project(NexKeyRuntimeConsumerExample LANGUAGES C)

find_package(NexKeyRuntime 0.1 CONFIG REQUIRED)

add_executable(nexkeyruntime_consumer_example ../minimal/main.c)
target_link_libraries(nexkeyruntime_consumer_example PRIVATE NexKeyRuntime::NexKeyRuntime)
