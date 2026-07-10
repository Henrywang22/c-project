cmake_minimum_required(VERSION 3.21)
cmake_policy(SET CMP0207 NEW)

if(NOT DEFINED STAGE_DIR OR NOT DEFINED RUNTIME_BIN)
    message(FATAL_ERROR "STAGE_DIR and RUNTIME_BIN must be provided.")
endif()

file(TO_CMAKE_PATH "${STAGE_DIR}" STAGE_DIR)
file(TO_CMAKE_PATH "${RUNTIME_BIN}" RUNTIME_BIN)

set(game_executable "${STAGE_DIR}/FishingVoyage.exe")
if(NOT EXISTS "${game_executable}")
    message(FATAL_ERROR "Game executable not found: ${game_executable}")
endif()
if(NOT IS_DIRECTORY "${RUNTIME_BIN}")
    message(FATAL_ERROR "Compiler runtime directory not found: ${RUNTIME_BIN}")
endif()

# Include the Qt plugins already copied by windeployqt.  This makes their
# dependencies (image codecs, platform backend, etc.) part of the same scan.
file(GLOB_RECURSE deployed_modules LIST_DIRECTORIES FALSE "${STAGE_DIR}/*.dll")

file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${game_executable}"
    LIBRARIES ${deployed_modules}
    DIRECTORIES "${STAGE_DIR}" "${RUNTIME_BIN}"
    RESOLVED_DEPENDENCIES_VAR resolved_dependencies
    UNRESOLVED_DEPENDENCIES_VAR unresolved_dependencies
    PRE_EXCLUDE_REGEXES
        "^[Aa][Pp][Ii]-[Mm][Ss]-[Ww][Ii][Nn]-.*"
        "^[Ee][Xx][Tt]-[Mm][Ss]-[Ww][Ii][Nn]-.*"
    POST_EXCLUDE_REGEXES
        "^[A-Za-z]:[/\\\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss]([/\\\\].*)?$"
)

if(unresolved_dependencies)
    list(JOIN unresolved_dependencies ", " unresolved_text)
    message(FATAL_ERROR "Unresolved runtime dependencies: ${unresolved_text}")
endif()

if(VERIFY_ONLY)
    set(external_dependencies)
    foreach(dependency IN LISTS resolved_dependencies)
        string(FIND "${dependency}" "${STAGE_DIR}/" stage_prefix)
        if(NOT stage_prefix EQUAL 0)
            list(APPEND external_dependencies "${dependency}")
        endif()
    endforeach()

    if(external_dependencies)
        list(JOIN external_dependencies "\n  " external_text)
        message(FATAL_ERROR "Runtime dependencies were not staged:\n  ${external_text}")
    endif()
    message(STATUS "Runtime dependency verification passed.")
    return()
endif()

set(copied_count 0)
foreach(dependency IN LISTS resolved_dependencies)
    get_filename_component(dependency_name "${dependency}" NAME)
    set(destination "${STAGE_DIR}/${dependency_name}")
    if(NOT EXISTS "${destination}")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${dependency}" "${destination}"
            RESULT_VARIABLE copy_result
        )
        if(NOT copy_result EQUAL 0)
            message(FATAL_ERROR "Failed to copy runtime dependency: ${dependency}")
        endif()
        math(EXPR copied_count "${copied_count} + 1")
    endif()
endforeach()

message(STATUS "Bundled ${copied_count} additional runtime dependency DLL(s).")
