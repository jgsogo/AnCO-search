
# Initializes block variables
INIT_BIICODE_BLOCK()

# Actually create targets: EXEcutables and libraries.
ADD_BIICODE_TARGETS()


IF(APPLE)
   TARGET_COMPILE_OPTIONS(${BII_BLOCK_TARGET} INTERFACE "-std=c++11 -stdlib=libc++")
ELSEIF (WIN32 OR UNIX)
   TARGET_COMPILE_OPTIONS(${BII_BLOCK_TARGET} INTERFACE "-std=c++11")
ENDIF(APPLE)

add_definitions(-DN_MAX_COLONIES=100)
add_definitions(-DN_MAX_ANTS_PER_COLONY=500)