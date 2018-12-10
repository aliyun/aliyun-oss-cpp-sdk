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

#include "ModelError.h"

using namespace AlibabaCloud::OSS;

static const char * GetArgErrorMsg(const int code)
{
    static const char * msg[] =
    {
        "Argument is invalid, please check.",
        /*Common 1-2*/
        "The bucket name is invalid. A bucket name must  be comprised of lower-case characters, numbers or dash(-) with 3-63 characters long.",
        "The object key is invalid. An object name should be between 1-1023 bytes long and cannot begin with '/' or '\\'",
        /*CORS   3-10*/
        "One bucket not allow exceed ten item of CORSRules.",
        "CORSRule.AllowedOrigins should not be empty.",
        "CORSRule.AllowedOrigins allowes at most one asterisk wildcard.",
        "CORSRule.AllowedMethods should not be empty.",
        "CORSRule.AllowedMethods only supports GET/PUT/DELETE/POST/HEAD.",
        "CORSRule.AllowedHeaders allowes at most one asterisk wildcard.",
        "CORSRule.ExposedHeader dose not allowe asterisk wildcard.",
        "CORSRule.MaxAgeSeconds should not be less than 0 or greater than 999999999.",
        /*Logging -11*/
        "Invalid logging prefix.",
        /*storageCapacity -12*/
        "Storage capacity must greater than -1.",
        /*WebSiet -13*/
        "Index document must not be empty.",
        "Invalid index document, must be end with.html.",
        "Invalid error document, must be end with .html.",
        /*iostream request body -16*/
        "Request body is null.",
        "Request body is in fail state. Logical error on i/o operation.",
        "Request body is in bad state. Read/writing error on i/o operation.",
        /*MultipartUpload -19*/
        "PartList is empty.",
        "PartSize should not be less than 100*1024 or greater than 5*1024*1024*1024.",
        "PartNumber should not be less than 1 or greater than 10000.",
        /*Lifecycle Rules -22*/
        "One bucket not allow exceed one thousand item of LifecycleRules.",
        "LifecycleRule should not be null or empty.",
        "Only one expiration property should be specified.",
        "You have a rule for a prefix, and therefore you cannot create the rule for the whole bucket.",
        "Configure at least one of file and fragment lifecycle.",
        /*GetObject -27*/
        "The range is invalid. The start should not be less than 0 or less then the end. The end could be -1 to get the rest of the data.",
    };

    int index = code - ARG_ERROR_START;
    int msg_size = sizeof(msg)/sizeof(msg[0]);
    if (code < ARG_ERROR_START || index > msg_size) {
        index = 0;
    }

    return msg[index];
}


const char * AlibabaCloud::OSS::GetModelErrorMsg(const int code)
{
    if (code >= ARG_ERROR_START && code <= ARG_ERROR_END) {
        return GetArgErrorMsg(code);
    }

    return "Model error, but undefined.";
}
