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

#include <string>
#include <list>
#include <alibabacloud/oss/OssClient.h>

namespace AlibabaCloud {
namespace OSS {
class TestUtils
{
public:
    static const std::list<std::string>& InvalidBucketNamesList();
    static const std::list<std::string>& InvalidObjectKeyNamesList();
    static const std::list<std::string>& InvalidLoggingPrefixNamesList();
    static const std::list<std::string>& InvalidPageNamesList();

    static std::string GetBucketName(const std::string& prefix);
    static std::string GetObjectKey(const std::string& prefix);
    static std::string GetTargetFileName(const std::string& prefix);

    static bool BucketExists(const OssClient &client, const std::string &prefix);
    static void EnsureBucketExist(const OssClient &client, const std::string &bucketName);
    static bool ObjectExists(const OssClient& client, const std::string& bucketName, const std::string& keyName);
    static void CleanBucket(const OssClient &client, const std::string &bucketName);
    static void CleanBucketsByPrefix(const OssClient &client, const std::string &prefix);
    static void CleanBucketVersioning(const OssClient &client, const std::string &bucketName);

    static PutObjectOutcome UploadObject(const OssClient& client, const std::string &bucketName,
        const std::string& keyName, const std::string& filename, const ObjectMetaData& metadata);
    static PutObjectOutcome UploadObject(const OssClient& client, const std::string &bucketName,
        const std::string& keyName, const std::string& filename);
    static void DownloadObject(const OssClient& client, const std::string& bucketName, const std::string& keyName, const std::string& targetFile);

    static void WaitForCacheExpire(int sec);
    static char GetRandomChar();
    static std::string GetRandomString(int length);
    static std::shared_ptr<std::iostream> GetRandomStream(int length);
    static void WriteRandomDatatoFile(const std::string &file, int length);

    static bool IsValidIp(const std::string &host);
    static std::string GetIpByEndpoint(const std::string &endpoint);

    static std::string GetExecutableDirectory();
    static std::wstring GetExecutableDirectoryW();

    static std::string GetGMTString(int64_t delayS);

    static std::string GetUTCString(int32_t Days, bool noSec = false);


    static std::string GetHTTPSEndpoint(const std::string endpoint);

    static std::string GetFileMd5(const std::string file);

    static std::string GetFileETag(const std::string file);

    static uint64_t GetFileCRC64(const std::string file);

    static void LogPrintCallback(LogLevel level, const std::string &stream);

    static std::string Base64Decode(std::string const& encoded_string);

    static bool IsByteBufferEQ(const char *src, const char *pat, int len);
    static bool IsByteBufferEQ(const unsigned char *src, const unsigned char *pat, int len);
    static bool IsByteBufferEQ(const ByteBuffer& src, const ByteBuffer& pat);

    static ByteBuffer GetRandomByteBuffer(int length);
};

}
}
