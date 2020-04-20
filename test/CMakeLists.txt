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
project(cpp-sdk-test VERSION ${version})
	
file(GLOB test_gtest_src "external/gtest/*")
file(GLOB test_main_src "src/*")
file(GLOB test_accesskey_src "src/AccessKey/*")
file(GLOB test_service_src "src/Service/*")
file(GLOB test_bucket_src "src/Bucket/*")
file(GLOB test_object_src "src/Object/*")
file(GLOB test_presignedurl_src "src/PresignedUrl/*")
file(GLOB test_multipartupload_src "src/MultipartUpload/*")
file(GLOB test_resumable_src "src/Resumable/*")
file(GLOB test_other_src "src/Other/*")
file(GLOB test_livechannel_src "src/LiveChannel/*")
file(GLOB test_encryption_src "src/Encryption/*")
	
add_executable(${PROJECT_NAME} 
	${test_main_src}
	${test_gtest_src} 
	${test_accesskey_src}
	${test_service_src}
	${test_bucket_src}
	${test_object_src}
	${test_presignedurl_src}
	${test_multipartupload_src}
	${test_resumable_src}
	${test_other_src}
	${test_livechannel_src}
	${test_encryption_src})

target_include_directories(${PROJECT_NAME}
	PRIVATE ${CMAKE_SOURCE_DIR}/sdk/include
	PRIVATE ${CMAKE_SOURCE_DIR}/sdk/
	PRIVATE ${CMAKE_SOURCE_DIR}/test/external)

if (${TARGET_OS} STREQUAL "WINDOWS")
target_include_directories(${PROJECT_NAME}
	PRIVATE ${CMAKE_SOURCE_DIR}/third_party/include)
endif()	
	
target_link_libraries(${PROJECT_NAME} cpp-sdk${STATIC_LIB_SUFFIX})	
target_link_libraries(${PROJECT_NAME} ${CRYPTO_LIBS})
target_link_libraries(${PROJECT_NAME} ${CLIENT_LIBS})
if (${TARGET_OS} STREQUAL "LINUX")
target_link_libraries(${PROJECT_NAME} pthread)	
endif()

target_compile_options(${PROJECT_NAME} 
	PRIVATE "${SDK_COMPILER_FLAGS}")

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
target_compile_options(${PROJECT_NAME} 
	PRIVATE "-Wno-invalid-source-encoding")
endif()