#
# Copyright 2009-2017 Alibaba Cloud All rights reserved.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.1)

cmake_policy(SET CMP0048 NEW)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#Project setting
file(STRINGS "VERSION" version)
project(alibabacloud-oss-cpp-sdk VERSION ${version})
message(STATUS "Project version: ${PROJECT_VERSION}")
set(SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(TARGET_OUTPUT_NAME_PREFIX "alibabacloud-oss-" CACHE STRING "The target's output name prefix")

#Options
option(BUILD_SHARED_LIBS  "Enable shared library" OFF)
option(BUILD_SAMPLE "Build sample" ON)
option(BUILD_TESTS "Build unit and perfermence tests" OFF)
option(ENABLE_COVERAGE "Flag to enable/disable building code with -fprofile-arcs and -ftest-coverage. Gcc only" OFF)


#Platform
if (CMAKE_CROSSCOMPILING)
	if (${CMAKE_SYSTEM_NAME} STREQUAL "Android")
		set(PLATFORM_ANDROID 1)
		set(TARGET_OS "ANDROID")
	else()
		message(FATAL_ERROR "Do not support target platform")
	endif()
else()
	if(CMAKE_HOST_APPLE)
		set(PLATFORM_APPLE 1)
		set(TARGET_OS "APPLE")
	elseif(CMAKE_HOST_UNIX)
		set(PLATFORM_LINUX 1)
		set(TARGET_OS "LINUX")
	elseif(CMAKE_HOST_WIN32)
		set(PLATFORM_WINDOWS 1)
		set(TARGET_OS "WINDOWS")
	else()
		message(FATAL_ERROR "Do not support unknown host OS")
	endif()
endif()

message(STATUS "TARGET_OS: ${TARGET_OS}")

add_definitions(-DPLATFORM_${TARGET_OS})

#Find dependency Library, curl, openssl
if (${TARGET_OS} STREQUAL "WINDOWS")
	set(WLIB_TARGET "Win32")
	if (CMAKE_CL_64)
	set(WLIB_TARGET "x64")
	endif()
	set(CRYPTO_LIBS 
		${CMAKE_SOURCE_DIR}/third_party/lib/${WLIB_TARGET}/ssleay32.lib 
		${CMAKE_SOURCE_DIR}/third_party/lib/${WLIB_TARGET}/libeay32.lib)
	set(CRYPTO_INCLUDE_DIRS 
		${CMAKE_SOURCE_DIR}/third_party/include) 

	set(CLIENT_LIBS 
		${CMAKE_SOURCE_DIR}/third_party/lib/${WLIB_TARGET}/libcurl.lib) 
	set(CLIENT_INCLUDE_DIRS 
		${CMAKE_SOURCE_DIR}/third_party/include) 
else()
	include(FindCURL)
	include(FindOpenSSL)

	if(NOT CURL_FOUND)
		message(FATAL_ERROR "Could not find curl")
	endif()

	if(NOT OPENSSL_FOUND)
		message(FATAL_ERROR "Could not find openssl")
	endif()

	set(CRYPTO_LIBS ${OPENSSL_LIBRARIES})
	set(CRYPTO_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIRS}) 
	set(CRYPTO_LIBS_ABSTRACT_NAME crypto ssl)

	set(CLIENT_LIBS ${CURL_LIBRARIES})
	set(CLIENT_INCLUDE_DIRS ${CURL_INCLUDE_DIRS}) 
	set(CLIENT_LIBS_ABSTRACT_NAME curl)
endif()

#Compiler flags
list(APPEND SDK_COMPILER_FLAGS "-std=c++11")
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	list(APPEND SDK_COMPILER_FLAGS "/MP")
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	list(APPEND SDK_COMPILER_FLAGS "-fno-exceptions" "-fPIC" "-fno-rtti")
	list(APPEND SDK_COMPILER_FLAGS "-Wall" "-Werror" "-pedantic" "-Wextra")
else()
	list(APPEND SDK_COMPILER_FLAGS "-fno-exceptions" "-fPIC" "-fno-rtti")
	list(APPEND SDK_COMPILER_FLAGS "-Wall" "-Werror" "-pedantic" "-Wextra")
	
	if (ENABLE_COVERAGE)
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
	SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
	SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fprofile-arcs -ftest-coverage")
	endif()
endif()


if (BUILD_SHARED_LIBS)
	set(STATIC_LIB_SUFFIX "-static")
else()
	set(STATIC_LIB_SUFFIX "")
endif()

include(ExternalProject)
include(GNUInstallDirs)

add_subdirectory(sdk)

if(BUILD_SAMPLE)
	add_subdirectory(sample)
endif()

if(BUILD_TESTS)
	add_subdirectory(test)
	add_subdirectory(ptest)
endif()
