file(GLOB_RECURSE COMMON_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE COMMON_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

add_library(common STATIC
    ${COMMON_SRC}
    ${COMMON_HEADERS}
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# libdatachannel an common library koppeln
target_link_libraries(common PUBLIC datachannel-static)
