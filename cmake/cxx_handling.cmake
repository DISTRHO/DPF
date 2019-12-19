# DISTRHO Plugin Framework (DPF)
# Copyright (C) 2019-2020 Benjamin Heisch <benjaminheisch@yahoo.de>
#
# Permission to use, copy, modify, and/or distribute this software for any purpose with
# or without fee is hereby granted, provided that the above copyright notice and this
# permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
# TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
# NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
# DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
# IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
# CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

# This cmake-Module contains functions arround c++ handling .
# use_cxx(CXX_VERSION(11,14,17,20))  -> Sets the C++ Version
# create_export_header(TARGET, OUTDIR) -> Creates a header for exporting symbols
#

set(__DIR_OF_GENERATE_CMAKE ${CMAKE_CURRENT_LIST_DIR})
macro(use_cxx cxx_version)
    if(CMAKE_VERSION VERSION_GREATER "3.1.0.0")
        set (CMAKE_CXX_STANDARD_REQUIRED ON)
        set (CMAKE_CXX_STANDARD ${cxx_version})
        message(STATUS "Enable c++${cxx_version} ${CMAKE_CXX_STANDARD}")
    else()
        get_cxx_alias(${cxx_version} cxx_alias)
        message(STATUS "The Version of Cmake is to old for save c++ ${cxx_version} support. Use wrapper instead")
        include(CheckCXXCompilerFlag)
        CHECK_CXX_COMPILER_FLAG("-std=c++${cxx_version}" compiler_supports_cxx_version)
        CHECK_CXX_COMPILER_FLAG("-std=c++${cxx_alias}" compiler_supports_cxx_alias)
        if(compiler_supports_cxx_version)
            add_definitions("-std=c++${cxx_version}")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++${cxx_version}")
			message(STATUS "Use -std=c++${cxx_version} for compiling")
        elseif(compiler_supports_cxx_alias)
            add_definitions("-std=c++${cxx_alias}")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++${cxx_alias}")
			message(STATUS "Use -std=c++${cxx_alias} for compiling")
        else()
            message(WARNING "The compiler ${CMAKE_CXX_COMPILER} has no C++${cxx_version} support. Please use a different C++ compiler.")
        endif(COMPILER_SUPPORTS_CXX11)
    endif(CMAKE_VERSION VERSION_GREATER "3.1.0.0")
endmacro(use_cxx)

#Can be called with OUTDIR and TARGET, if not provided OUTDIR is BIN_DIR/generated and TARGET is the current PROJECT_NAME like CREATE_EXPORT_HEADER(TARGET Blub OUTDIR Bla)
function(CREATE_EXPORT_HEADER)
        set(oneValueArgs OUTDIR TARGET)
        cmake_parse_arguments(CREATE_EXPORT_HEADER "" "${oneValueArgs}" "" ${ARGN} )
        if(NOT CREATE_EXPORT_HEADER_OUTDIR)
                set(CREATE_EXPORT_HEADER_OUTDIR ${PROJECT_BINARY_DIR}/generated/)
        endif(NOT CREATE_EXPORT_HEADER_OUTDIR)
        if(NOT CREATE_EXPORT_HEADER_TARGET)
                set(CREATE_EXPORT_HEADER_TARGET ${PROJECT_NAME})
        endif(NOT CREATE_EXPORT_HEADER_TARGET)
        include(GenerateExportHeader)
        generate_export_header(${CREATE_EXPORT_HEADER_TARGET} EXPORT_FILE_NAME "${CREATE_EXPORT_HEADER_OUTDIR}/${CREATE_EXPORT_HEADER_TARGET}_export.h")
        target_include_directories(${CREATE_EXPORT_HEADER_TARGET} PUBLIC ${CREATE_EXPORT_HEADER_OUTDIR})
endfunction(CREATE_EXPORT_HEADER)


######################HELPER FUNCTIONS######################


#sets the cxx_alias variable to a cxx_alias(like 0x for c++11).
macro(get_cxx_alias cxx_number cxx_alias)
	if(${cxx_number} EQUAL 11)
		set(cxx_alias "0x")
	elseif(${cxx_number} EQUAL 14)
		set(cxx_alias "1y")
	else()
		message(WARNING "No supported cxx_alias. Please add an valid cxx_alias if possible")
	endif()
	message(STATUS "Use ${cxx_alias} as alias for ${cxx_number}")
endmacro(get_cxx_alias)

