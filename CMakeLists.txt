cmake_minimum_required(VERSION 3.10)
project ("MachXO2 I2C Flash Tool" C)

file(GLOB SOURCES src/*.c src/XO2_ECA/*.c)
add_executable(mxo2_i2c_flash ${SOURCES})

install(TARGETS mxo2_i2c_flash RUNTIME DESTINATION bin)
