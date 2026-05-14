# Shim config that satisfies drogon's find_package(yaml-cpp) using the
# FetchContent-provided target, bypassing install-layout path checks.
if(NOT TARGET yaml-cpp)
    message(FATAL_ERROR "yaml-cpp target not found — ensure FetchContent yaml-cpp is made available before this config is loaded")
endif()

set(yaml-cpp_FOUND TRUE)
set(YAML_CPP_FOUND TRUE)
get_target_property(YAML_CPP_INCLUDE_DIR yaml-cpp INTERFACE_INCLUDE_DIRECTORIES)
set(YAML_CPP_INCLUDE_DIRS ${YAML_CPP_INCLUDE_DIR})
set(YAML_CPP_LIBRARIES yaml-cpp)
