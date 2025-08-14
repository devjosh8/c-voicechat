# Alle Quell- und Header-Dateien finden
file(GLOB_RECURSE CLIENT_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)
file(GLOB_RECURSE CLIENT_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

add_executable(client
    ${CLIENT_SRC}
    ${CLIENT_HEADERS}
)

# Include-Verzeichnisse
target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Gemeinsame Bibliothek verlinken, falls vorhanden
if(TARGET common)
    target_link_libraries(client PRIVATE common)
endif()

target_include_directories(client PRIVATE
    ${CMAKE_SOURCE_DIR}/external/portaudio/include
)
target_link_libraries(client PRIVATE
    ${CMAKE_SOURCE_DIR}/external/portaudio/lib/libportaudio.a
    m
    jack
    pthread
    rt
    asound
)


