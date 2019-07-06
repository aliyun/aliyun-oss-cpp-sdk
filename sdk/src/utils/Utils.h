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

#include <string>
#include <ctime>
#include <iostream>
#include <alibabacloud/oss/Types.h>

namespace AlibabaCloud
{
namespace OSS
{
    #define UNUSED_PARAM(x) ((void)(x))

    std::string ComputeContentMD5(const std::string& data);
    std::string ComputeContentMD5(const char *data, size_t size);
    std::string ComputeContentMD5(std::istream & stream); 

    std::string ComputeContentETag(const std::string& data);
    std::string ComputeContentETag(const char *data, size_t size);
    std::string ComputeContentETag(std::istream & stream);

    std::string GenerateUuid();
    std::string UrlEncode(const std::string &src);
    std::string UrlDecode(const std::string &src);

    std::string Base64Encode(const std::string &src);
    std::string Base64Encode(const char *src, int len);
    std::string Base64EncodeUrlSafe(const std::string &src);
    std::string Base64EncodeUrlSafe(const char *src, int len);


    void StringReplace(std::string &src, const std::string &s1, const std::string &s2);
    std::string LeftTrim(const char* source);
    std::string RightTrim(const char* source);
    std::string Trim(const char* source);
    std::string LeftTrimQuotes(const char* source);
    std::string RightTrimQuotes(const char* source);
    std::string TrimQuotes(const char* source);
    std::string ToLower(const char* source);
    std::string ToUpper(const char* source);
    std::string ToGmtTime(std::time_t &t);
    std::string ToUtcTime(std::time_t &t);
    std::time_t UtcToUnixTime(const std::string &t);

    bool IsIp(const std::string &host);
    bool IsValidBucketName(const std::string &bucketName);
    bool IsValidObjectKey(const std::string &key);
    bool IsValidLoggingPrefix(const std::string &prefix);
    bool IsValidChannelName(const std::string &channelName);
    bool IsValidPlayListName(const std::string &playListName);
    bool IsValidTagKey(const std::string &key);
    bool IsValidTagValue(const std::string &value);

    const std::string &LookupMimeType(const std::string& name);
    std::string CombineHostString(const std::string &endpoint, const std::string &bucket, bool isCname);
    std::string CombinePathString(const std::string &endpoint, const std::string &bucket, const std::string &key);
    std::string CombineQueryString(const ParameterCollection &parameters);
    std::string CombineRTMPString(const std::string &endpoint, const std::string &bucket, bool isCname);


    std::streampos GetIOStreamLength(std::iostream &stream);

    const char *ToStorageClassName(StorageClass storageClass);
    StorageClass ToStorageClassType(const char *name);

    const char *ToAclName(CannedAccessControlList acl);
    CannedAccessControlList ToAclType(const char *name);

    const char * ToCopyActionName(CopyActionList action);

    const char * ToRuleStatusName(RuleStatus status);
    RuleStatus ToRuleStatusType(const char *name);

    const char * ToLiveChannelStatusName(LiveChannelStatus status);
    LiveChannelStatus ToLiveChannelStatusType(const char *name);

	const char* ToRequestPayerName(RequestPayer payer);
	RequestPayer ToRequestPayer(const char* name);
}
}
