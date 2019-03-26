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
#include "Utils.h"
#include <time.h>
#include <ctime>
#include <sstream>
#include <memory>
#include <fstream>
#include <thread>
#ifdef _WIN32
#include <ws2tcpip.h>
#pragma comment (lib, "Ws2_32.lib")
#else
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#endif
#include <regex>
#include <iomanip>
#include <src/utils/Utils.h>
#include <src/http/Url.h>
#include <cstring>

#ifdef GetObject
#undef GetObject
#endif 
using namespace AlibabaCloud::OSS;
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

const std::list<std::string>& TestUtils::InvalidBucketNamesList()
{
    const static std::list<std::string> nameList =
    { "a", "1", "!", "aa", "12", "a1",
        "a!", "1!", "aAa", "1A1", "a!a", "FengChao@123", "-a123", "a_123", "a123-",
        "1234567890123456789012345678901234567890123456789012345678901234", ""
    };
    return nameList;
}

const std::list<std::string>& TestUtils::InvalidObjectKeyNamesList()
{
    const static std::list<std::string> nameList =
    {
        "/abc", "\\123", ""
    };
    return nameList;
}

const std::list<std::string>& TestUtils::InvalidLoggingPrefixNamesList()
{
    const static std::list<std::string> nameList =
    {
        "1", "-a", "@@", "a_", "abcdefghijklmnopqrstuvwxyz1234567"
    };
    return nameList;
}

const std::list<std::string>& TestUtils::InvalidPageNamesList()
{
    const static std::list<std::string> nameList =
    {
        "a", ".html", ""
    };
    return nameList;
}

std::string TestUtils::GetBucketName(const std::string &prefix)
{
    std::stringstream ss;
    auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    ss << prefix << "-bucket-" << tp.time_since_epoch().count();
    return ss.str();
}

std::string TestUtils::GetObjectKey(const std::string &prefix)
{
    std::stringstream ss;
    auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    ss << prefix << "-object-" << tp.time_since_epoch().count();
    return ss.str();
}

std::string TestUtils::GetTargetFileName(const std::string &prefix)
{
    std::stringstream ss;
    auto tp = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
    ss << prefix << "-file-" << tp.time_since_epoch().count();
    return ss.str();
}


bool TestUtils::BucketExists(const OssClient &client, const std::string &bucketName)
{
    return client.GetBucketAcl(GetBucketAclRequest(bucketName)).isSuccess();
}

void TestUtils::EnsureBucketExist(const OssClient &client, const std::string &bucketName)
{
    if (!BucketExists(client, bucketName)) {
        client.CreateBucket(CreateBucketRequest(bucketName));
    }
}

bool TestUtils::ObjectExists(const OssClient& client, const std::string& bucketName, const std::string& keyName)
{
    return client.GetObjectMeta(GetObjectMetaRequest(bucketName, keyName)).isSuccess();
}

void TestUtils::CleanBucket(const OssClient &client, const std::string &bucketName)
{
    if (!client.DoesBucketExist(bucketName))
        return;

    // Clean up multipart uploading object
    auto listOutcome = client.ListMultipartUploads(ListMultipartUploadsRequest(bucketName));
    if (listOutcome.isSuccess()) {
        for (auto const &upload : listOutcome.result().MultipartUploadList())
        {
            client.AbortMultipartUpload(AbortMultipartUploadRequest(bucketName, upload.Key, upload.UploadId));
        }
    }

    // Clean up objects
    ListObjectsRequest request(bucketName);
    bool IsTruncated = false;
    do {
        auto outcome = client.ListObjects(request);
        if (outcome.isSuccess()) {
            for (auto const &obj : outcome.result().ObjectSummarys()) {
                client.DeleteObject(DeleteObjectRequest(bucketName, obj.Key()));
            }
        }
        else {
            break;
        }
        request.setMarker(outcome.result().NextMarker());
        IsTruncated = outcome.result().IsTruncated();
    } while (IsTruncated);

    // Clean up LiveChannel
    ListLiveChannelRequest request2(bucketName);
    IsTruncated = false;
    do{
        auto listOutcome = client.ListLiveChannel(request2);
        if(listOutcome.isSuccess())
        {
            for(auto const &liveChannel : listOutcome.result().LiveChannelList())
            {
                client.DeleteLiveChannel(DeleteLiveChannelRequest(bucketName, liveChannel.name));
            }
            IsTruncated = listOutcome.result().IsTruncated();
            request2.setMarker(listOutcome.result().NextMarker());
        }else{
            break;
        }
    }while(IsTruncated);

    // Delete the bucket.
    client.DeleteBucket(DeleteBucketRequest(bucketName));
}

void TestUtils::CleanBucketsByPrefix(const OssClient &client, const std::string &prefix)
{
    ListBucketsRequest request;
    request.setMaxKeys(1);
    request.setPrefix(prefix);
    bool IsTruncated = false;
    do {
        auto outcome = client.ListBuckets(request);
        if (outcome.isSuccess()) {
            CleanBucket(client, outcome.result().Buckets()[0].Name());
        }
        request.setMarker(outcome.result().NextMarker());
        IsTruncated = outcome.result().IsTruncated();
    } while (IsTruncated);
}

PutObjectOutcome TestUtils::UploadObject(const OssClient& client, const std::string &bucketName,
    const std::string& keyName, const std::string &filename, const ObjectMetaData &metadata)
{
    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(filename, std::ios::in);
    return client.PutObject(PutObjectRequest(bucketName, keyName, content, metadata));
}

PutObjectOutcome TestUtils::UploadObject(const OssClient& client, const std::string &bucketName,
    const std::string& keyName, const std::string &filename)
{
    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(filename, std::ios::in|std::ios::binary);
    return client.PutObject(PutObjectRequest(bucketName, keyName, content));
}

void TestUtils::DownloadObject(const OssClient& client, const std::string &bucketName,
    const std::string& keyName, const std::string& targetFile)
{
    GetObjectRequest request(bucketName, keyName);
    request.setResponseStreamFactory([=]() {return std::make_shared<std::fstream>(targetFile, std::ios::binary|std::ios_base::out|std::ios_base::trunc); });
    client.GetObject(request);
}

void TestUtils::WaitForCacheExpire(int sec)
{
    std::this_thread::sleep_for(std::chrono::seconds(sec));
}

char TestUtils::GetRandomChar()
{
    return 'a' + rand() % 32;
}

std::string TestUtils::GetRandomString(int length)
{
    std::stringstream ss;
    for (int i = 0; i < length; i++) {
        ss << static_cast<char>('!' + rand() % 90);
    }
    return ss.str();
}

std::shared_ptr<std::iostream> TestUtils::GetRandomStream(int length)
{
    std::shared_ptr<std::stringstream> stream = std::make_shared<std::stringstream>();
    std::stringstream ss;
    for (int i = 0; i < length; i++) {
        *stream << static_cast<char>('!' + rand() % 90);
    }
    stream->seekg(0);
    stream->seekp(0, std::ios_base::end);
    return stream;
}

void TestUtils::WriteRandomDatatoFile(const std::string &file, int length)
{
    std::fstream of(file, std::ios::out | std::ios::binary | std::ios::trunc);
    of << GetRandomString(length);
    of.close();
}


bool TestUtils::IsValidIp(const std::string &host)
{
    return AlibabaCloud::OSS::IsIp(host);
}

std::string TestUtils::GetIpByEndpoint(const std::string &endpoint)
{
    if (IsValidIp(endpoint)) {
        return endpoint;
    }

    std::string hostname(endpoint);

    if (!endpoint.compare(0, 7, "http://", 7)) {
        hostname = hostname.substr(7);
    }
    else if (!endpoint.compare(0, 8, "https://", 8)) {
        hostname = hostname.substr(8);
    }

    auto it = hostname.find("/");
    if (it != std::string::npos) {
        hostname = hostname.substr(0, it);
    }

    struct addrinfo *ailist, *aip;
    struct addrinfo hint;
    //struct sockaddr_in *sinp;
    char m_ipaddr[16] = {0};
    int ret;
    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_INET;
    //hint.ai_socktype = SOCK_DGRAM;
    hint.ai_flags = AI_PASSIVE; 
    hint.ai_protocol = 0;
    ret = getaddrinfo(hostname.c_str(), "http", &hint, &ailist);
    if (ret == -1) {
        return "";
    }

    for (aip = ailist; aip != NULL; aip = aip->ai_next) {
        struct sockaddr_in *sinp = (struct sockaddr_in *)aip->ai_addr;
#ifdef _WIN32
        sprintf_s(m_ipaddr, "%d.%d.%d.%d",
            (*sinp).sin_addr.S_un.S_un_b.s_b1,
            (*sinp).sin_addr.S_un.S_un_b.s_b2,
            (*sinp).sin_addr.S_un.S_un_b.s_b3,
            (*sinp).sin_addr.S_un.S_un_b.s_b4);
#else
        snprintf(m_ipaddr, sizeof(m_ipaddr), "%d.%d.%d.%d",
            ((*sinp).sin_addr.s_addr >> 0) & 0xFF,
            ((*sinp).sin_addr.s_addr >> 8) & 0xFF,
            ((*sinp).sin_addr.s_addr >> 16)& 0xFF,
            ((*sinp).sin_addr.s_addr >> 24));
#endif
    }
    freeaddrinfo(ailist);
    return std::string(m_ipaddr);
}

#ifdef _WIN32

static std::string FromWString(const wchar_t* source)
{
    const auto len = static_cast<int>(std::wcslen(source));
    std::string output;
    if (int requiredSizeInBytes = WideCharToMultiByte(CP_UTF8, 0 ,  source,
        len,  nullptr,  0,  nullptr, nullptr)) {
        output.resize(requiredSizeInBytes);
    }
    const auto result = WideCharToMultiByte(CP_UTF8, 0, source,
        len,  &output[0], static_cast<int>(output.length()),
        nullptr, nullptr);

    if (result) {
        output.resize(result);
        return output;
    }

    return "";
}

std::string TestUtils::GetExecutableDirectory()
{
    WCHAR buffer[PATH_MAX];
    memset(buffer, 0, sizeof(buffer));

    if (GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(sizeof(buffer))))
    {
        std::string bufferStr(FromWString(buffer));
        auto fileNameStart = bufferStr.find_last_of('\\');
        if (fileNameStart != std::string::npos)
        {
            bufferStr = bufferStr.substr(0, fileNameStart);
        }

        return bufferStr;
    }

    return "";
}

#else

std::string TestUtils::GetExecutableDirectory()
{
    char dest[PATH_MAX];
    size_t destSize = sizeof(dest);
    memset(dest, 0, destSize);

    if (readlink("/proc/self/exe", dest, destSize))
    {
        std::string executablePath(dest);
        auto lastSlash = executablePath.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            return executablePath.substr(0, lastSlash);
        }
    }

    return "./";
}
#endif

std::string TestUtils::GetGMTString(int64_t delayS)
{
    std::time_t t = std::time(nullptr);
    t += delayS;
    return ToGmtTime(t);
}

std::string TestUtils::GetUTCString(int32_t Days, bool noSec)
{
    const int64_t secPerDay = 24LL * 60LL * 60LL;
    std::time_t t = std::time(nullptr);
    if (noSec) {
        t = t - t % secPerDay;
    }
    t += (int64_t)Days * secPerDay;
    return ToUtcTime(t);
}

std::string TestUtils::GetHTTPSEndpoint(const std::string endpoint)
{
    Url url(endpoint);
    std::string result;
    result.append("https://").append(url.authority());
    return result;
}

std::string TestUtils::GetFileMd5(const std::string file)
{
    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(file, std::ios::in | std::ios::binary);
    return ComputeContentMD5(*content);
}


std::string TestUtils::GetFileETag(const std::string file)
{
    std::shared_ptr<std::iostream> content = std::make_shared<std::fstream>(file, std::ios::in | std::ios::binary);
    return ComputeContentETag(*content);
}

void TestUtils::LogPrintCallback(LogLevel level, const std::string &stream)
{
    UNUSED_PARAM(level);
    std::cout << stream;
}

std::string TestUtils::Base64Decode(std::string const& data)
{
    int in_len = static_cast<int>(data.size());
    int i = 0;
    int in_ = 0;
    unsigned char part4[4];
    std::string ret;

    while (in_len-- && (data[in_] != '=')) {
        unsigned char ch = data[in_++];
        if ('A' <= ch && ch <= 'Z')  ch = ch - 'A';           // A - Z
        else if ('a' <= ch && ch <= 'z') ch = ch - 'a' + 26;  // a - z
        else if ('0' <= ch && ch <= '9') ch = ch - '0' + 52;  // 0 - 9
        else if ('+' == ch) ch = 62;                          // +
        else if ('/' == ch) ch = 63;                          // /
        else if ('=' == ch) ch = 64;                          // =
        else ch = 0xff;                                       // something wrong
        part4[i++] = ch;
        if (i == 4) {
            ret += (part4[0] << 2) + ((part4[1] & 0x30) >> 4);
            ret += ((part4[1] & 0xf) << 4) + ((part4[2] & 0x3c) >> 2);
            ret += ((part4[2] & 0x3) << 6) + part4[3];
            i = 0;
        }
    }

    if (i) {
        for (int j = i; j < 4; j++)
            part4[j] = 0xFF;
        ret += (part4[0] << 2) + ((part4[1] & 0x30) >> 4);
        ret += ((part4[1] & 0xf) << 4) + ((part4[2] & 0x3c) >> 2);
        ret += ((part4[2] & 0x3) << 6) + part4[3];
    }

    return ret;
}
