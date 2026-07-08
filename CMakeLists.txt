cmake_minimum_required(VERSION 3.16)
project(TuxAsistente VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar los componentes necesarios de Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Habilitar el procesamiento automático de MOC (necesario por tu #include "main.moc")
set(CMAKE_AUTOMOC ON)

# Definir el ejecutable
add_executable(tux src/main.cpp)

# Vincular las librerías de Qt6
target_link_libraries(tux PRIVATE Qt6::Widgets)
