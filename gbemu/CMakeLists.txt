set(MAIN_SOURCES
  main.c
)

add_executable(gbemu ${HEADERS} ${MAIN_SOURCES})
target_link_libraries(gbemu emu)