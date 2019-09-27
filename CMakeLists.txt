CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(TFREC)

# compiler options
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mfpu=vfp -march=armv6 -mfloat-abi=hard -ftree-vectorize -ffast-math -std=c++0x -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-write-strings")

#linker options
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

MESSAGE(INFO " Using CXX Flags : ${CMAKE_CXX_FLAGS}")
MESSAGE(INFO " Using LD  Flags : ${CMAKE_EXE_LINKER_FLAGS}")

ADD_SUBDIRECTORY(src)