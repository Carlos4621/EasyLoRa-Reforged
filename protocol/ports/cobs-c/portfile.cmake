vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO cmcqueen/cobs-c

    REF 352dbcdeb804d49281c4c6fc60cf2c92cc4ac6b8

    SHA512 cabbc81f7ee657fed57da4361cab82bbce9764aace585565dc9d40760c4fcc41f4b26b3846c1002c92047f7849990453c723ffd4b37da1f27c9050ceff1f30e0

    HEAD_REF main
)

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt"
    "${SOURCE_PATH}/CMakeLists.txt"
    COPYONLY
)

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/cobs-c-config.cmake.in"
    "${SOURCE_PATH}/cobs-c-config.cmake.in"
    COPYONLY
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(
    PACKAGE_NAME cobs-c
    CONFIG_PATH lib/cmake/cobs-c
)

# Los headers son iguales para Debug y Release.
file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
)

vcpkg_install_copyright(
    FILE_LIST "${SOURCE_PATH}/LICENSE.txt"
)