file (GLOB sources CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/lib/*.c")

file (GLOB headers CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/include/*.h")

add_library(emu STATIC ${sources} ${headers})

target_include_directories(emu PUBLIC ${PROJECT_SOURCE_DIR}/include )