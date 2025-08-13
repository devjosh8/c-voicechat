cmake_minimum_required(VERSION 3.20)
project(voicechat-in-c LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Wichtig für clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optionale gemeinsame Bibliothek
add_subdirectory(common)

# Client und Server hinzufügen
add_subdirectory(client)
add_subdirectory(server)

