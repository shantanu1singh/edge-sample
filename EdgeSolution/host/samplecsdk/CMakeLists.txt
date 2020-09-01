cmake_minimum_required(VERSION 3.10)
set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)
project(samplec)

# macro(compileAsC99)
#   if (CMAKE_VERSION VERSION_LESS "3.1")
#     if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
#       set (CMAKE_C_FLAGS "--std=c99 ${CMAKE_C_FLAGS}")
#     if (NOT IN_OPENWRT)
#       set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
#     endif()
#     endif()
#   else()
#     set (CMAKE_C_STANDARD 99)
#     set (CMAKE_CXX_STANDARD 11)
#   endif()
# endmacro(compileAsC99)

# compileAsC99()

# OPTION(BUILD_EXE "Set the project to be built as an exe" OFF) # Disabled by default

# # Add a custom CMake Modules directory
# set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake ${CMAKE_MODULE_PATH})

# # Load default environment for CMakeLists.txt
# find_package(AutomotiveEdge)
# set_project_guard(${PROJECT_NAME})

# Enable static code analysis for project
# find_package(StaticCodeAnalysis)
find_package(azure_c_shared_utility REQUIRED CONFIG)

# find_package(Boost COMPONENTS REQUIRED system)

set(AZUREIOT_INC_FOLDER ".." "/usr/local/include/azureiot" "/usr/local/include/azureiot/inc")

include_directories(${AZUREIOT_INC_FOLDER})
# include_directories("/usr/include")

# set(
#     HEADER_LIST 
#     "${PROJECT_SOURCE_DIR}/include/iot_hub_client.h"
#     "${PROJECT_SOURCE_DIR}/include/iot_hub_client_exception.h"
#     "${PROJECT_SOURCE_DIR}/include/iot_hub_device_client.h")

# set(
#     project_default_SOURCES 
#     "${PROJECT_SOURCE_DIR}/iot_hub_client.cpp"
#     "${PROJECT_SOURCE_DIR}/iot_hub_device_client.cpp")

    message("Build as exe: Enabled")
    add_executable(${PROJECT_NAME} ./sample.c)

# target_include_directories(${PROJECT_NAME} PUBLIC
#     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
#     $<INSTALL_INTERFACE:include>
#     ${MQTT_INC_FOLDER}
#     PRIVATE src)

target_link_libraries(${PROJECT_NAME}
    iothub_client
    iothub_client_mqtt_transport 
    iothub_client_mqtt_ws_transport
    umqtt
    aziotsharedutil
    parson
)