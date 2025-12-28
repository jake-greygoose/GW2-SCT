cmake_minimum_required(VERSION 3.20)
project(Detarget LANGUAGES CXX)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

function(detarget_pad VALUE WIDTH OUTPUT_VAR)
    set(_val "${VALUE}")
    string(LENGTH "${_val}" _len)
    set(_width "${WIDTH}")
    while(_len LESS _width)
        set(_val "0${_val}")
        math(EXPR _len "${_len} + 1")
    endwhile()
    set(${OUTPUT_VAR} "${_val}" PARENT_SCOPE)
endfunction()

set(_VERSION_PART_NAMES VER_MAJOR VER_MINOR VER_BUILD VER_REVISION)

if(NOT DEFINED DETARGET_VERSION OR DETARGET_VERSION STREQUAL "")
    string(TIMESTAMP CURRENT_YEAR "%Y")
    string(TIMESTAMP CURRENT_MONTH "%m")
    string(TIMESTAMP CURRENT_DAY "%d")
    string(TIMESTAMP CURRENT_HOUR "%H")
    string(TIMESTAMP CURRENT_MINUTE "%M")

    math(EXPR VER_MAJOR "${CURRENT_YEAR}")
    math(EXPR VER_MINOR "${CURRENT_MONTH}")
    math(EXPR VER_BUILD "${CURRENT_DAY}")
    math(EXPR VER_REVISION "(${CURRENT_HOUR} * 60) + ${CURRENT_MINUTE}")
else()
    string(REPLACE "." ";" VERSION_LIST "${DETARGET_VERSION}")
    list(LENGTH VERSION_LIST VERSION_PART_COUNT)
    if(NOT VERSION_PART_COUNT EQUAL 4)
        message(FATAL_ERROR "DETARGET_VERSION must be in the form YYYY.MM.DD.RRRR")
    endif()

    foreach(index RANGE 0 3)
        list(GET VERSION_LIST ${index} component)
        if(NOT component MATCHES "^[0-9]+$")
            message(FATAL_ERROR "Version component '${component}' is not numeric.")
        endif()
        string(REGEX REPLACE "^0*([0-9][0-9]*)$" "\\1" stripped "${component}")
        if(stripped STREQUAL "")
            set(stripped 0)
        endif()
        list(GET _VERSION_PART_NAMES ${index} component_name)
        set(${component_name} ${stripped})
    endforeach()
endif()

detarget_pad("${VER_MINOR}" 2 VER_MINOR_PAD)
detarget_pad("${VER_BUILD}" 2 VER_BUILD_PAD)
detarget_pad("${VER_REVISION}" 4 VER_REVISION_PAD)
set(DETARGET_VERSION "${VER_MAJOR}.${VER_MINOR_PAD}.${VER_BUILD_PAD}.${VER_REVISION_PAD}")
set(DETARGET_VERSION_CSV "${VER_MAJOR},${VER_MINOR},${VER_BUILD},${VER_REVISION}")
message(STATUS "Configuring Detarget Version: ${DETARGET_VERSION}")

set(AUTOVERSION_HEADER "${CMAKE_CURRENT_BINARY_DIR}/autoversion.h")
set(AUTOVERSION_RC "${CMAKE_CURRENT_BINARY_DIR}/autoversion.rc")

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/autoversion.h.in"
    "${AUTOVERSION_HEADER}"
    @ONLY
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/autoversion.rc.in"
    "${AUTOVERSION_RC}"
    @ONLY
)

add_library(Detarget SHARED
    src/entry.cpp
    src/RE/Addresses.cpp
    src/RE/AgentSelectHooks.cpp
    src/RE/Scanner.cpp
    "${AUTOVERSION_RC}"
)

target_include_directories(Detarget
    PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_compile_features(Detarget PRIVATE cxx_std_20)

set_target_properties(Detarget PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    OUTPUT_NAME Detarget
)

if(MSVC)
    target_compile_options(Detarget PRIVATE
        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:/Zi>
    )
    target_link_options(Detarget PRIVATE
        $<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>:/DEBUG>
    )
endif()
