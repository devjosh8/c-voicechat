cmake_minimum_required(VERSION 3.20)
project(voicechat-in-c LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Wichtig für clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Statisches Linking erzwingen
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(BUILD_SHARED_LIBS OFF)

# libdatachannel Konfiguration - GLOBAL für das ganze Projekt setzen
set(USE_SYSTEM_OPENSSL OFF CACHE BOOL "Build OpenSSL from source")
set(USE_GNUTLS OFF CACHE BOOL "Use GnuTLS instead of OpenSSL")  
set(USE_NICE OFF CACHE BOOL "Use libnice instead of libjuice")
set(NO_WEBSOCKET ON CACHE BOOL "Disable WebSocket support")
set(NO_EXAMPLES ON CACHE BOOL "Disable examples")
set(NO_TESTS ON CACHE BOOL "Disable tests")


# OpenSSL-Variablen setzen BEVOR libdatachannel geladen wird
set(OPENSSL_ROOT_DIR /usr)
set(OPENSSL_CRYPTO_LIBRARY /usr/lib/libcrypto.so)
set(OPENSSL_SSL_LIBRARY /usr/lib/libssl.so)
set(OPENSSL_INCLUDE_DIR /usr/include)

add_subdirectory(external/libdatachannel)

# Optionale gemeinsame Bibliothek
add_subdirectory(common)

# Client und Server hinzufügen
add_subdirectory(client)
add_subdirectory(server)

