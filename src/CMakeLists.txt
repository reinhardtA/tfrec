# search all cpp files
FILE(GLOB_RECURSE SOURCES_FILES "*.cpp")
MESSAGE(INFO " Sourcen : ${SOURCES_FILES}")

# search all headers
FILE(GLOB_RECURSE HEADER_FILES "*.h")
MESSAGE(INFO " Header : ${HEADER_FILES}")

INCLUDE_DIRECTORIES(".")

# 
ADD_EXECUTABLE(tfrec ${SOURCES_FILES})
TARGET_LINK_LIBRARIES(tfrec rtlsdr pthread)