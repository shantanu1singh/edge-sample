cmake_minimum_required(VERSION 3.10)

set (CMAKE_C_STANDARD 99)
set (CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_EDGE_MODULES")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DUSE_EDGE_MODULES -Wall")

set(AZUREIOT_INC_FOLDER "/usr/include/azureiot" "/usr/include/azureiot/inc")

include_directories(${AZUREIOT_INC_FOLDER})

set(main_files
    ./main.c
)

add_executable(cperf ${main_files})

target_link_libraries(cperf
    iothub_client_mqtt_transport
    iothub_client
    parson
    umqtt
    prov_auth_client
    hsm_security_client
    uhttp
    aziotsharedutil
    pthread
    curl
    ssl
    crypto
    m
    uuid
)
