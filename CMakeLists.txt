CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(TFREC)

MESSAGE(INFO " ${CMAKE_SYSTEM_PROCESSOR}")

# compiler options
IF(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mfpu=vfp -march=armv6 -mfloat-abi=hard -ftree-vectorize -ffast-math -std=c++0x -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-write-strings")

ELSEIF(${CMAKE_SYSTEM_PROCESSOR} MATCHES  "x86_64")
   SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -ftree-vectorize -ffast-math -std=c++0x -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable -Wno-write-strings")

ELSE()
   MESSAGE(ERROR " System not supported, yet")

ENDIF()

#linker options
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")

MESSAGE(INFO " Using CXX Flags : ${CMAKE_CXX_FLAGS}")
MESSAGE(INFO " Using LD  Flags : ${CMAKE_EXE_LINKER_FLAGS}")

ADD_SUBDIRECTORY(src)
