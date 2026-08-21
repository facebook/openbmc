################################################################################
##
## The University of Illinois/NCSA
## Open Source License (NCSA)
##
## Copyright (c) 2014-2017, Advanced Micro Devices, Inc. All rights reserved.
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to
## deal with the Software without restriction, including without limitation
## the rights to use, copy, modify, merge, publish, distribute, sublicense,
## and#or sell copies of the Software, and to permit persons to whom the
## Software is furnished to do so, subject to the following conditions:
##
##  - Redistributions of source code must retain the above copyright notice,
##    this list of conditions and the following disclaimers.
##  - Redistributions in binary form must reproduce the above copyright
##    notice, this list of conditions and the following disclaimers in
##    the documentation and#or other materials provided with the distribution.
##  - Neither the names of Advanced Micro Devices, Inc,
##    nor the names of its contributors may be used to endorse or promote
##    products derived from this Software without specific prior written
##    permission.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
## THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
## OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
## ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
## DEALINGS WITH THE SOFTWARE.
##
################################################################################

## Parses the VERSION_STRING variable and places
## the first, second and third number values in
## the major, minor and patch variables.
function( parse_version VERSION_STRING )

    string ( FIND ${VERSION_STRING} "-" STRING_INDEX )

    if ( ${STRING_INDEX} GREATER -1 )
        math ( EXPR STRING_INDEX "${STRING_INDEX} + 1" )
        string ( SUBSTRING ${VERSION_STRING} ${STRING_INDEX} -1 VERSION_BUILD )
    endif ()

    string ( REGEX MATCHALL "[0123456789]+" VERSIONS ${VERSION_STRING} )
    list ( LENGTH VERSIONS VERSION_COUNT )

    if ( ${VERSION_COUNT} GREATER 0)
        list ( GET VERSIONS 0 MAJOR )
        set ( VERSION_MAJOR ${MAJOR} PARENT_SCOPE )
        set ( TEMP_VERSION_STRING "${MAJOR}" )
    endif ()

    if ( ${VERSION_COUNT} GREATER 1 )
        list ( GET VERSIONS 1 MINOR )
        set ( VERSION_MINOR ${MINOR} PARENT_SCOPE )
        set ( TEMP_VERSION_STRING "${TEMP_VERSION_STRING}.${MINOR}" )
    endif ()

    if ( ${VERSION_COUNT} GREATER 2 )
        list ( GET VERSIONS 2 PATCH )
        set ( VERSION_PATCH ${PATCH} PARENT_SCOPE )
        set ( TEMP_VERSION_STRING "${TEMP_VERSION_STRING}.${PATCH}" )
    endif ()

    if ( DEFINED VERSION_BUILD )
        set ( VERSION_BUILD "${VERSION_BUILD}" PARENT_SCOPE )
    endif ()

    set ( VERSION_STRING "${TEMP_VERSION_STRING}" PARENT_SCOPE )

endfunction ()

## Gets the current version of the repository
## using versioning tags and git describe.
## Passes back a packaging version string
## and a library version string.
function(get_version DEFAULT_VERSION_STRING VERSION_PREFIX)

    parse_version ( ${DEFAULT_VERSION_STRING} )

    find_program ( GIT NAMES git )

    if ( GIT )
        execute_process ( COMMAND git describe --tags --dirty --long --match ${VERSION_PREFIX}-[0-9.]*
                          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                          OUTPUT_VARIABLE GIT_TAG_STRING
                          OUTPUT_STRIP_TRAILING_WHITESPACE
                          RESULT_VARIABLE RESULT )

        if ( ${RESULT} EQUAL 0 )

            parse_version ( ${GIT_TAG_STRING} )

        endif ()

    endif ()

    set( VERSION_STRING "${VERSION_STRING}" PARENT_SCOPE )
    set( VERSION_MAJOR  "${VERSION_MAJOR}" PARENT_SCOPE )
    set( VERSION_MINOR  "${VERSION_MINOR}" PARENT_SCOPE )
    set( VERSION_PATCH  "${VERSION_PATCH}" PARENT_SCOPE )
    set( VERSION_BUILD  "${VERSION_BUILD}" PARENT_SCOPE )

endfunction()
