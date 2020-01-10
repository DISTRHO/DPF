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

# This cmake-Module contains functions arround versioning.
# set_version(MAJOR,MINOR,PATCH,BUILD)  -> Sets the Version, if BUILD is not given, this
#                   Module tries to detect the Build version with git or svn.
#
#
#

# SET_VERSION(1 0 1) or SET_VERSION(1 0 1 10204)
# If the 4th Component not given, it will be detected with git or svn
# (if git and svn are present(whatever this happened) git is preferred
# this function use the current projectname.
function(set_version major minor patch)
	set(TEMP_PROJECT_NAME ${PROJECT_NAME})
	#setze default_value
        if(NOT ARGV3)
            set(TEMP_CURRENT_REVISON 0)
            set_subversion_revision(TEMP_CURRENT_REVISON)
            set_git_revision(TEMP_CURRENT_REVISON)
        else(NOT ARGV3)
	set(TEMP_CURRENT_REVISON ${ARGV3})
        endif(NOT ARGV3)
	
	if(POLICY CMP0048)
		cmake_policy(SET CMP0048 NEW)
	endif(POLICY CMP0048)
	set(${TEMP_PROJECT_NAME}_VERSION "${major}.${minor}.${patch}.${TEMP_CURRENT_REVISON}" PARENT_SCOPE)
	set(${TEMP_PROJECT_NAME}_VERSION_MAJOR ${major} PARENT_SCOPE)
	set(${TEMP_PROJECT_NAME}_VERSION_MINOR ${minor} PARENT_SCOPE)
	set(${TEMP_PROJECT_NAME}_VERSION_PATCH ${patch} PARENT_SCOPE)
	set(${TEMP_PROJECT_NAME}_VERSION_TWEAK ${TEMP_CURRENT_REVISON} PARENT_SCOPE)
	message(STATUS "Set Version to: ${major}.${minor}.${patch}.${TEMP_CURRENT_REVISON}")
endfunction(set_version)

set(__DIR_OF_GENERATE_CMAKE ${CMAKE_CURRENT_LIST_DIR})

# Can be called with OUTDIR and TARGET, if not provided OUTDIR is BIN_DIR/generated and TARGET is
# the current PROJECT_NAME use itlike CREATE_VERSION_HEADER(TARGET Blub OUTDIR Bla)
# the versionheader is included to the given target.
function(CREATE_VERSION_HEADER)
        set(oneValueArgs OUTDIR TARGET)
        cmake_parse_arguments(CREATE_VERSION_HEADER "" "${oneValueArgs}" "" ${ARGN} )
        if(NOT CREATE_VERSION_HEADER_OUTDIR)
                set(CREATE_VERSION_HEADER_OUTDIR ${PROJECT_BINARY_DIR}/generated/)
        endif(NOT CREATE_VERSION_HEADER_OUTDIR)
        if(NOT CREATE_VERSION_HEADER_TARGET)
                set(CREATE_VERSION_HEADER_TARGET ${PROJECT_NAME})
        endif(NOT CREATE_VERSION_HEADER_TARGET)
        configure_file (
         ${__DIR_OF_GENERATE_CMAKE}/version.h.in
         "${CREATE_VERSION_HEADER_OUTDIR}/${CREATE_VERSION_HEADER_TARGET}_version.h"
         )
     target_include_directories(${CREATE_VERSION_HEADER_TARGET} PUBLIC ${CREATE_VERSION_HEADER_OUTDIR})
endfunction(CREATE_VERSION_HEADER)












#########################Helperfunctions#########################
macro(set_git_revision internal_revision_var)
	find_package(Git)
	if(GIT_FOUND)
		set(possible_working_copies ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
		foreach(working_copy_path ${possible_working_copies})
			if(EXISTS "${working_copy_path}/.git")
				# Get the latest abbreviated commit hash of the working branch
				execute_process(
				  COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
				  WORKING_DIRECTORY ${working_copy_path}
				  OUTPUT_VARIABLE GIT_COMMIT_HASH
				  OUTPUT_STRIP_TRAILING_WHITESPACE
				)
				set(${internal_revision_var} "0x${GIT_COMMIT_HASH}")
				message(STATUS "Use current git-hash ${GIT_COMMIT_HASH} as tweak-version")
				break()
			endif(EXISTS "${working_copy_path}/.git")
		endforeach(working_copy_path)
	endif(GIT_FOUND)
endmacro(set_git_revision)

macro(set_subversion_revision internal_revision_var)
	find_package(Subversion)
	if(SUBVERSION_FOUND)
		set(possible_working_copies ${PROJECT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
		foreach(working_copy_path ${possible_working_copies})
			if(EXISTS "${working_copy_path}/.svn")
				Subversion_WC_INFO(${working_copy_path} TEMP_SVN_INFO_LOG)
				#Subversion_WC_LOG(${working_copy_path} TEMP_SVN_INFO_LOG)
				if(${TEMP_SVN_INFO_LOG_WC_REVISION})
					set(${internal_revision_var} ${TEMP_SVN_INFO_LOG_WC_REVISION})
					message(STATUS "Use current Subversion-Revision ${TEMP_SVN_INFO_LOG_WC_REVISION} as tweak-version")
					break()
				endif(${TEMP_SVN_INFO_LOG_WC_REVISION})
			endif(EXISTS "${working_copy_path}/.svn")
		endforeach(working_copy_path)
	endif(SUBVERSION_FOUND)
endmacro(set_subversion_revision)
