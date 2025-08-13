cmake_minimum_required(VERSION 3.20)

# Projektname und Sprache festlegen
project(VoicechatInC LANGUAGES C CXX)

# C++ Standard setzen
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # compile_commands.json generieren


# Alle Quell-Dateien aus src/ rekursiv finden
file(GLOB_RECURSE SRC_FILES
    "${CMAKE_SOURCE_DIR}/src/*.c"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

if(NOT SRC_FILES)
    message(FATAL_ERROR "No source files found in src/")
endif()


# Alle Header-Dateien aus include/ rekursiv finden
file(GLOB_RECURSE HEADER_FILES
    "${CMAKE_SOURCE_DIR}/include/*.h"
    "${CMAKE_SOURCE_DIR}/include/*.hpp"
)

# Executable erstellen (Header werden hier nur gelistet, um sie im Projektbaum zu sehen)
add_executable(${PROJECT_NAME}
    ${SRC_FILES}
    ${HEADER_FILES}
)

# Include-Verzeichnisse hinzufügen
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# ----------------- Header-only Libraries hinzufügen ------------------
    # stb als Interface-Library anlegen (kein eigener Build-Schritt)
    # add_library(stb INTERFACE)

    # Include-Verzeichnis bekannt machen
    # target_include_directories(stb INTERFACE
    #    ${CMAKE_SOURCE_DIR}/external/stb
    # )

    # In deinem Executable oder einer anderen Lib verwenden:
    # target_link_libraries(MyApp PRIVATE stb)

# ----------------- Projektlokale .so oder .a ------------------
    # Header-Verzeichnis hinzufügen
    # target_include_directories(MyApp PRIVATE
    #    ${CMAKE_SOURCE_DIR}/external/mylib/include
    # )

    # Lokale Library-Datei finden
    # find_library(MYLIB NAMES mylib
    #     PATHS ${CMAKE_SOURCE_DIR}/libs
    #     NO_DEFAULT_PATH
    #)

    # Mit dem Executable verlinken
    # target_link_libraries(MyApp PRIVATE ${MYLIB})

# ----------------- Systemweite Bibliothek .so oder .a ------------
    # find_library(FOO_LIB NAMES foo)
    # target_link_libraries(MyApp PRIVATE ${FOO_LIB})

# ----------------- Single C File -----------------
    # add_library(miniaudio ${CMAKE_SOURCE_DIR}/external/miniaudio/miniaudio.c)
    # target_link_libraries(MyApp PRIVATE miniaudio)
