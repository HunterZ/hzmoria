cmake_minimum_required(VERSION 3.22.3)

# this is the windows port
project(hzmoria LANGUAGES C)

# TODO: get this working with visual studio too (only tested w/mingw)
# this probably requires finding (vcpkg-provided) pdcurses libs or something

# derive some source paths from the current directory
set(HZMORIA_ROOT_DIR   "${CMAKE_CURRENT_LIST_DIR}")  # project root dir
set(HZMORIA_SOURCE_DIR "${HZMORIA_ROOT_DIR}/source") # source files dir
set(HZMORIA_FILES_DIR  "${HZMORIA_ROOT_DIR}/files")  # data files dir
set(HZMORIA_DOCS_DIR   "${HZMORIA_FILES_DIR}/docs")  # doc files dir
# ...and some destination paths
set(HZMORIA_DIST_DIR   "${HZMORIA_ROOT_DIR}/dist")   # deployment dir

# target for building the game binary
add_executable(${PROJECT_NAME}
  ${HZMORIA_SOURCE_DIR}/config.h
  ${HZMORIA_SOURCE_DIR}/cp437utf.h
  ${HZMORIA_SOURCE_DIR}/externs.h
  ${HZMORIA_SOURCE_DIR}/types.h
  ${HZMORIA_SOURCE_DIR}/cp437utf.c
  ${HZMORIA_SOURCE_DIR}/create.c
  ${HZMORIA_SOURCE_DIR}/creature.c
  ${HZMORIA_SOURCE_DIR}/death.c
  ${HZMORIA_SOURCE_DIR}/desc.c
  ${HZMORIA_SOURCE_DIR}/dungeon.c
  ${HZMORIA_SOURCE_DIR}/eat.c
  ${HZMORIA_SOURCE_DIR}/files.c
  ${HZMORIA_SOURCE_DIR}/generate.c
  ${HZMORIA_SOURCE_DIR}/help.c
  ${HZMORIA_SOURCE_DIR}/io.c
  ${HZMORIA_SOURCE_DIR}/magic.c
  ${HZMORIA_SOURCE_DIR}/main.c
  ${HZMORIA_SOURCE_DIR}/misc1.c
  ${HZMORIA_SOURCE_DIR}/misc2.c
  ${HZMORIA_SOURCE_DIR}/misc3.c
  ${HZMORIA_SOURCE_DIR}/misc4.c
  ${HZMORIA_SOURCE_DIR}/monsters.c
  ${HZMORIA_SOURCE_DIR}/moria1.c
  ${HZMORIA_SOURCE_DIR}/moria2.c
  ${HZMORIA_SOURCE_DIR}/moria3.c
  ${HZMORIA_SOURCE_DIR}/moria4.c
  ${HZMORIA_SOURCE_DIR}/ms_misc.c
  ${HZMORIA_SOURCE_DIR}/player.c
  ${HZMORIA_SOURCE_DIR}/potions.c
  ${HZMORIA_SOURCE_DIR}/prayer.c
  ${HZMORIA_SOURCE_DIR}/recall.c
  ${HZMORIA_SOURCE_DIR}/rnd.c
  ${HZMORIA_SOURCE_DIR}/save.c
  ${HZMORIA_SOURCE_DIR}/scrolls.c
  ${HZMORIA_SOURCE_DIR}/sets.c
  ${HZMORIA_SOURCE_DIR}/signals.c
  ${HZMORIA_SOURCE_DIR}/spells.c
  ${HZMORIA_SOURCE_DIR}/staffs.c
  ${HZMORIA_SOURCE_DIR}/store1.c
  ${HZMORIA_SOURCE_DIR}/store2.c
  ${HZMORIA_SOURCE_DIR}/tables.c
  ${HZMORIA_SOURCE_DIR}/treasure.c
  ${HZMORIA_SOURCE_DIR}/variable.c
  ${HZMORIA_SOURCE_DIR}/wands.c
  ${HZMORIA_SOURCE_DIR}/wizard.c
)
target_include_directories(${PROJECT_NAME}
  PUBLIC "${HZMORIA_SOURCE_DIR}"
)
target_link_libraries(${PROJECT_NAME}
  pdcurses_wincon
  winmm
)
# assemble the binary path+name programmatically so we can get it ASAP
# yes, this is probably hacky and fragile
get_target_property(HZMORIA_BINARY_NAME ${PROJECT_NAME} NAME)
set(HZMORIA_BINARY_NAME "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_EXECUTABLE_PREFIX}${HZMORIA_BINARY_NAME}${CMAKE_EXECUTABLE_SUFFIX}")
# strip release build binary
add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  DEPENDS "${HZMORIA_BINARY_NAME}"
  COMMAND $<$<CONFIG:release>:${CMAKE_STRIP}>
  ARGS "${HZMORIA_BINARY_NAME}"
)
# copy files needed for running into the binary dir as a post-build step
# this is provided as a convenience for launching the game from an IDE
file(GLOB HZMORIA_RUN_FILES
  "${CMAKE_CURRENT_LIST_DIR}/MORIA.CNF"
  "${HZMORIA_FILES_DIR}/*"
)
foreach(HZMORIA_RUN_FILE ${HZMORIA_RUN_FILES})
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMENT "Copying ${HZMORIA_RUN_FILE} to ${CMAKE_CURRENT_BINARY_DIR}..."
    COMMAND "${CMAKE_COMMAND}" -E copy "${HZMORIA_RUN_FILE}" "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM
  )
endforeach()

# follow-on logic for copying everything into dist folder for release purposes
# I know CMake has an "install" feature, but:
# - its IDE integration is terrible
# - the files shouldn't change much anyway
# - computers are fast enough now that it's easy to just clean rebuild as needed

# stuff that goes directly in dist/
set(HZMORIA_DIST_FILES
  "${HZMORIA_BINARY_NAME}"
  ${HZMORIA_RUN_FILES}
#  "${CMAKE_CURRENT_LIST_DIR}/readme.txt"
)
# stuff that goes in dist/docs/
file(GLOB HZMORIA_DOC_FILES
  "${HZMORIA_DOCS_DIR}/*"
)
# define custom copy commands for each file
# build depends list as we go
set(HZMORIA_DIST_DEPENDS "")
# files in dist/
foreach(HZMORIA_DIST_FILE ${HZMORIA_DIST_FILES})
  # get stripped filename
  get_filename_component(HZMORIA_TEMP_NAME "${HZMORIA_DIST_FILE}" NAME)
  # prepend destination path
  set(HZMORIA_TEMP_DEST "${HZMORIA_DIST_DIR}/${HZMORIA_TEMP_NAME}")
  add_custom_command(
    OUTPUT  "${HZMORIA_TEMP_DEST}"
    DEPENDS "${HZMORIA_DIST_FILE}"
    COMMENT "Copying dist      file ${HZMORIA_TEMP_NAME}"
    COMMAND "${CMAKE_COMMAND}" -E copy "${HZMORIA_DIST_FILE}" "${HZMORIA_DIST_DIR}"
    VERBATIM
  )
  # append output file to depends list
  set(HZMORIA_DIST_DEPENDS ${HZMORIA_DIST_DEPENDS} ${HZMORIA_TEMP_DEST})
endforeach()
# files in dist/doc/
foreach(HZMORIA_DOC_FILE ${HZMORIA_DOC_FILES})
  # get stripped filename
  get_filename_component(HZMORIA_TEMP_NAME "${HZMORIA_DOC_FILE}" NAME)
  # prepend destination path
  set(HZMORIA_TEMP_DEST "${HZMORIA_DIST_DIR}/docs/${HZMORIA_TEMP_NAME}")
  add_custom_command(
    OUTPUT  "${HZMORIA_TEMP_DEST}"
    DEPENDS "${HZMORIA_DOC_FILE}"
    COMMENT "Copying dist/docs file ${HZMORIA_TEMP_NAME}"
    COMMAND "${CMAKE_COMMAND}" -E copy "${HZMORIA_DOC_FILE}" "${HZMORIA_DIST_DIR}/docs"
    VERBATIM
  )
  # append output file to depends list
  set(HZMORIA_DIST_DEPENDS ${HZMORIA_DIST_DEPENDS} ${HZMORIA_TEMP_DEST})
endforeach()
# make a target that depends on all of the custom commands and the game binary
add_custom_target(dist
  DEPENDS ${HZMORIA_DIST_DEPENDS}
)
add_dependencies(dist ${PROJECT_NAME})
