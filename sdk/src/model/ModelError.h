/*
 * Copyright 2009-2017 Alibaba Cloud All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once
#include <alibabacloud/oss/client/Error.h>

namespace AlibabaCloud
{
namespace OSS
{
	const char * GetModelErrorMsg(const int code);

    /*Error For Argument Check*/
    const int ARG_ERROR_BASE   = ERROR_CLIENT_BASE + 1000;
    const int ARG_ERROR_START  = ARG_ERROR_BASE;
    const int ARG_ERROR_END    = ARG_ERROR_START +  999;

    /*Default*/
    const int ARG_ERROR_DEFAULT     = ARG_ERROR_BASE + 0;
    /*Common*/
    const int ARG_ERROR_BUCKET_NAME = ARG_ERROR_BASE + 1;
    const int ARG_ERROR_OBJECT_NAME = ARG_ERROR_BASE + 2;
    /*CORS*/
    const int ARG_ERROR_CORS_RULE_LIMIT                    = ARG_ERROR_BASE + 3;
    const int ARG_ERROR_CORS_ALLOWEDORIGINS_EMPTY          = ARG_ERROR_BASE + 4;
    const int ARG_ERROR_CORS_ALLOWEDORIGINS_ASTERISK_COUNT = ARG_ERROR_BASE + 5;
    const int ARG_ERROR_CORS_ALLOWEDMETHODS_EMPTY          = ARG_ERROR_BASE + 6;
    const int ARG_ERROR_CORS_ALLOWEDMETHODS_VALUE          = ARG_ERROR_BASE + 7;
    const int ARG_ERROR_CORS_ALLOWEDHEADERS_ASTERISK_COUNT = ARG_ERROR_BASE + 8;
    const int ARG_ERROR_CORS_EXPOSEHEADERS_ASTERISK_COUNT  = ARG_ERROR_BASE + 9;
    const int ARG_ERROR_CORS_MAXAGESECONDS_RANGE           = ARG_ERROR_BASE + 10;
    /*Logging*/
    const int ARG_ERROR_LOGGING_TARGETPREFIX_INVALID  = ARG_ERROR_BASE + 11;
    /*StorageCapacity*/
    const int ARG_ERROR_STORAGECAPACITY_INVALID = ARG_ERROR_BASE + 12;
    /*WebSiet*/
    const int ARG_ERROR_WEBSITE_INDEX_DOCCUMENT_EMPTY        = ARG_ERROR_BASE + 13;
    const int ARG_ERROR_WEBSITE_INDEX_DOCCUMENT_NAME_INVALID = ARG_ERROR_BASE + 14;
    const int ARG_ERROR_WEBSITE_ERROR_DOCCUMENT_NAME_INVALID = ARG_ERROR_BASE + 15;
    /*iostream request body*/
    const int ARG_ERROR_REQUEST_BODY_NULLPTR    = ARG_ERROR_BASE + 16;
    const int ARG_ERROR_REQUEST_BODY_FAIL_STATE = ARG_ERROR_BASE + 17;
    const int ARG_ERROR_REQUEST_BODY_BAD_STATE  = ARG_ERROR_BASE + 18;
    /*MultipartUpload*/
    const int ARG_ERROR_MULTIPARTUPLOAD_PARTLIST_EMPTY   = ARG_ERROR_BASE + 19;
    const int ARG_ERROR_MULTIPARTUPLOAD_PARTSIZE_RANGE   = ARG_ERROR_BASE + 20;
    const int ARG_ERROR_MULTIPARTUPLOAD_PARTNUMBER_RANGE = ARG_ERROR_BASE + 21;
    /*Lifecycle Rules*/
    const int ARG_ERROR_LIFECYCLE_RULE_LIMIT       = ARG_ERROR_BASE + 22;
    const int ARG_ERROR_LIFECYCLE_RULE_EMPTY       = ARG_ERROR_BASE + 23;
    const int ARG_ERROR_LIFECYCLE_RULE_EXPIRATION  = ARG_ERROR_BASE + 24;
    const int ARG_ERROR_LIFECYCLE_RULE_ONLY_ONE_FOR_BUCKET = ARG_ERROR_BASE + 25;
    const int ARG_ERROR_LIFECYCLE_RULE_CONFIG_EMPTY = ARG_ERROR_BASE + 26;
    /*GetObject*/
    const int ARG_ERROR_OBJECT_RANGE_INVALID = ARG_ERROR_BASE + 27;
}
}

