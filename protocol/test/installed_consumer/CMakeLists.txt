cmake_minimum_required(VERSION 3.20)

project(EasyLoRaProtocolInstalledConsumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(EasyLoRaProtocol CONFIG REQUIRED)

add_executable(installed_consumer ../external_consumer/main.cpp)
target_link_libraries(installed_consumer PRIVATE EasyLoRa::protocol)
