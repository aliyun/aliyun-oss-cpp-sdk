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

#include <gtest/gtest.h>
#include <alibabacloud/oss/OssClient.h>
#include <alibabacloud/oss/http/Url.h>
#include <src/utils/Utils.h>
#include "../Config.h"
#include "../Utils.h"
#include <fstream>
#include "src/utils/FileSystemUtils.h"
#include "src/utils/StreamBuf.h"
#include "src/auth/HmacSha1Signer.h"

namespace AlibabaCloud {
namespace OSS {

class UtilsFunctionTest : public ::testing::Test {
protected:
    UtilsFunctionTest()
    {
    }

    ~UtilsFunctionTest() override
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    static int64_t GetFileLength(const std::string& file);
};

int64_t UtilsFunctionTest::GetFileLength(const std::string& file)
{
    std::fstream f(file, std::ios::in | std::ios::binary);
    f.seekg(0, f.end);
    int64_t size = f.tellg();
    f.close();
    return size;
}

TEST_F(UtilsFunctionTest, Base64EncodeTest)
{
    std::vector<std::string> ori = { "abc" , "abcd" , "abcde", "" };
    std::vector<std::string> pat = { "YWJj" , "YWJjZA==" , "YWJjZGU=", ""};

    auto i = ori.size();
    for (i = 0; i < ori.size(); i++) {
        auto result = Base64Encode(ori[i]);
        //std::cout << "Base64EncodeTest: src:"<< ori[i] 
        //    << " ,result:" << result 
        //    << " ,pat:" << pat[i] << std::endl;
        EXPECT_STREQ(result.c_str(), pat[i].c_str());
    }
    EXPECT_TRUE((i == ori.size()));
}

TEST_F(UtilsFunctionTest, Base64DecodeTest)
{
    std::vector<std::string> ori = { "YWJj" , "YWJjZA==" , "YWJjZGU=", "", "YWJjZA" , "YWJjZGU" };
    std::vector<std::string> pat = { "abc" , "abcd" , "abcde", "", "abcd" , "abcde" };

    auto i = ori.size();
    for (i = 0; i < ori.size(); i++) {
        auto result = Base64Decode(ori[i]);
        EXPECT_EQ(result.size(), pat[i].size());
        EXPECT_TRUE(TestUtils::IsByteBufferEQ(reinterpret_cast<const char *>(result.data()), pat[i].data(), result.size()));
    }
    EXPECT_TRUE((i == ori.size()));
}

TEST_F(UtilsFunctionTest, Base64EncodeAndDecodeTest)
{
    for (int i = 1; i < 256; i++) {
        ByteBuffer data = TestUtils::GetRandomByteBuffer(i);
        EXPECT_EQ(data.size(), static_cast<size_t>(i));
        auto result = Base64Encode(data);
        auto data1 = Base64Decode(result.c_str(), result.size());
        EXPECT_EQ(data1.size(), static_cast<size_t>(i));
        EXPECT_TRUE(TestUtils::IsByteBufferEQ(data.data(), data1.data(), i));
    }
}

static std::vector<std::string> urlOri = 
{
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_~",
    "`!@#$%^&*()+={}[]:;'\\|<>,?/ \"",
    "hello world!"
};

static std::vector<std::string> urlPat =
{
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_~",
    "%60%21%40%23%24%25%5E%26%2A%28%29%2B%3D%7B%7D%5B%5D%3A%3B%27%5C%7C%3C%3E%2C%3F%2F%20%22",
    "hello%20world%21"
};

TEST_F(UtilsFunctionTest, UrlEncodeTest)
{
    auto i = urlOri.size();
    for (i = 0; i < urlOri.size(); i++) {
        auto result = UrlEncode(urlOri[i]);
        EXPECT_STREQ(result.c_str(), urlPat[i].c_str());
    }
    EXPECT_TRUE((i == urlOri.size()));
}

TEST_F(UtilsFunctionTest, UrlDecodeTest)
{
    auto i = urlOri.size();
    for (i = 0; i < urlOri.size(); i++) {
        auto result = UrlDecode(urlPat[i]);
        EXPECT_STREQ(result.c_str(), urlOri[i].c_str());
    }
    EXPECT_TRUE((i == urlPat.size()));
}

TEST_F(UtilsFunctionTest, ToStorageClassNameTest)
{
    EXPECT_STREQ(ToStorageClassName(StorageClass::Standard), "Standard");
    EXPECT_STREQ(ToStorageClassName(StorageClass::IA), "IA");
    EXPECT_STREQ(ToStorageClassName(StorageClass::Archive), "Archive");
    EXPECT_STREQ(ToStorageClassName(StorageClass::ColdArchive), "ColdArchive");
}

TEST_F(UtilsFunctionTest, ToStorageClassTypeTest)
{
    EXPECT_EQ(ToStorageClassType("Standard"), StorageClass::Standard);
    EXPECT_EQ(ToStorageClassType("standard"), StorageClass::Standard);
    EXPECT_EQ(ToStorageClassType("IA"), StorageClass::IA);
    EXPECT_EQ(ToStorageClassType("ia"), StorageClass::IA);
    EXPECT_EQ(ToStorageClassType("Archive"), StorageClass::Archive);
    EXPECT_EQ(ToStorageClassType("ArChive"), StorageClass::Archive);
    EXPECT_EQ(ToStorageClassType(nullptr), StorageClass::Standard);
    EXPECT_EQ(ToStorageClassType("unknown"), StorageClass::Standard);
    EXPECT_EQ(ToStorageClassType("ColdArchive"), StorageClass::ColdArchive);
    EXPECT_EQ(ToStorageClassType("coldArchive"), StorageClass::ColdArchive);
}

TEST_F(UtilsFunctionTest, ToAclNameTest)
{
    EXPECT_STREQ(ToAclName(CannedAccessControlList::Private), "private");
    EXPECT_STREQ(ToAclName(CannedAccessControlList::PublicRead), "public-read");
    EXPECT_STREQ(ToAclName(CannedAccessControlList::PublicReadWrite), "public-read-write");
    EXPECT_STREQ(ToAclName(CannedAccessControlList::Default), "default");
}

TEST_F(UtilsFunctionTest, ToAclTypeTest)
{
    EXPECT_EQ(ToAclType("private"), CannedAccessControlList::Private);
    EXPECT_EQ(ToAclType("public-read"), CannedAccessControlList::PublicRead);
    EXPECT_EQ(ToAclType("public-read-write"), CannedAccessControlList::PublicReadWrite);
    EXPECT_EQ(ToAclType("Private"), CannedAccessControlList::Private);
    EXPECT_EQ(ToAclType("Public-read"), CannedAccessControlList::PublicRead);
    EXPECT_EQ(ToAclType("Public-read-write"), CannedAccessControlList::PublicReadWrite);
    EXPECT_EQ(ToAclType(nullptr), CannedAccessControlList::Default);
    EXPECT_EQ(ToAclType("unknown"), CannedAccessControlList::Default);
}

TEST_F(UtilsFunctionTest, ToCopyActionNameTest)
{
    EXPECT_STREQ(ToCopyActionName(CopyActionList::Copy), "COPY");
    EXPECT_STREQ(ToCopyActionName(CopyActionList::Replace), "REPLACE");
}

TEST_F(UtilsFunctionTest, TrimSpaceChTest)
{
    std::string str = " ����  ";
    EXPECT_STREQ(Trim(str.c_str()).c_str(), "����");

    str = " ��  ��  ";
    EXPECT_STREQ(Trim(str.c_str()).c_str(), "��  ��");
}

TEST_F(UtilsFunctionTest, TrimSpaceTest)
{
    std::vector<std::string> testString = { " abc " , "   abc   " , "  abc" , "abc  ", "    abc     ", "\r    \nabc     \n\r"  };
    for (auto const &str : testString) {
        EXPECT_STREQ(Trim(str.c_str()).c_str(), "abc");
    }
}

TEST_F(UtilsFunctionTest, LeftTrimSpaceTest)
{
    std::vector<std::string> testString = { " abc " , "   abc   " , "  abc    " , "abc  " };
    for (auto const &str : testString) {
        auto result = LeftTrim(str.c_str());
        EXPECT_EQ(result.compare(0, 3, "abc", 3), 0);
        EXPECT_NE(result.compare("abc"), 0);
    }
}

TEST_F(UtilsFunctionTest, RightTrimSpaceTest)
{
    std::vector<std::string> testString = { " abc " , "   abc   " , "  abc    " , "abc  " };
    for (auto const &str : testString) {
        auto result = RightTrim(str.c_str());
        auto pos = result.find('a');
        EXPECT_EQ(strcmp(result.c_str() + pos, "abc"), 0);
    }
}

TEST_F(UtilsFunctionTest, TrimQuotesTest)
{
    std::vector<std::string> testString = { R"("abc")" , R"(""abc"")" , R"(""abc)" , R"(abc"""")" };
    for (auto const &str : testString) {
        EXPECT_STREQ(TrimQuotes(str.c_str()).c_str(), "abc");
    }
}

TEST_F(UtilsFunctionTest, LeftTrimQuotesTest)
{
    std::vector<std::string> testString = { R"(""abc"")" , R"("""abc "")" , R"("""abc  "")" , R"(abc  "")" };
    for (auto const &str : testString) {
        auto result = LeftTrimQuotes(str.c_str());
        EXPECT_EQ(result.compare(0, 3, "abc", 3), 0);
        EXPECT_NE(result.compare("abc"), 0);
    }
}

TEST_F(UtilsFunctionTest, RightTrimQuotesTest)
{
    std::vector<std::string> testString = { R"(""abc"")" , R"(""" abc"")" , R"("""abc")" , R"(abc""""")" };
    for (auto const &str : testString) {
        auto result = RightTrimQuotes(str.c_str());
        auto pos = result.find('a');
        EXPECT_EQ(strcmp(result.c_str() + pos, "abc"), 0);
    }
}

TEST_F(UtilsFunctionTest, ToLowerTest)
{
    std::vector<std::string> testString = { "ABC" , "Abc" , "AbC" , "abc" };
    for (auto const &str : testString) {
        auto result = ToLower(str.c_str());
        EXPECT_STREQ(result.c_str(), "abc");
    }
}

TEST_F(UtilsFunctionTest, ToUpperTest)
{
    std::vector<std::string> testString = { "ABC" , "Abc" , "AbC" , "abc" };
    for (auto const &str : testString) {
        auto result = ToUpper(str.c_str());
        EXPECT_STREQ(result.c_str(), "ABC");
    }
}

TEST_F(UtilsFunctionTest, IsIpTest)
{
    EXPECT_EQ(IsIp("192.168.1.1"), true);
    EXPECT_EQ(IsIp("10.1.1.1"), true);
    EXPECT_EQ(IsIp("127.0.0.0"), true);
    EXPECT_EQ(IsIp("266.168.1.1"), false);
    EXPECT_EQ(IsIp("1192.168.1.1"), false);
    EXPECT_EQ(IsIp("hostname"), false);
}

TEST_F(UtilsFunctionTest, ToGmtTimeTest)
{
    std::time_t t = 0;
    std::string timeStr = ToGmtTime(t);
    EXPECT_STREQ(timeStr.c_str(), "Thu, 01 Jan 1970 00:00:00 GMT");

    t = 1520411719;
    timeStr = ToGmtTime(t);
    EXPECT_STREQ(timeStr.c_str(), "Wed, 07 Mar 2018 08:35:19 GMT");

    t = 1554703347;
    timeStr = ToGmtTime(t);
    EXPECT_STREQ(timeStr.c_str(), "Mon, 08 Apr 2019 06:02:27 GMT");

    t = 1554739347;
    timeStr = ToGmtTime(t);
    EXPECT_STREQ(timeStr.c_str(), "Mon, 08 Apr 2019 16:02:27 GMT");
}

TEST_F(UtilsFunctionTest, ToGmtTimeWithSetlocaleTest)
{
    auto oldLoc = std::cout.getloc();
    std::locale::global(std::locale(""));

    std::time_t t = 0;
    std::string timeStr = ToGmtTime(t);
    EXPECT_STREQ(timeStr.c_str(), "Thu, 01 Jan 1970 00:00:00 GMT");

    t = 1520411719;
    timeStr = ToGmtTime(t);
    EXPECT_STREQ(timeStr.c_str(), "Wed, 07 Mar 2018 08:35:19 GMT");

    t = 1554703347;
    timeStr = ToGmtTime(t);
    EXPECT_STREQ(timeStr.c_str(), "Mon, 08 Apr 2019 06:02:27 GMT");

    t = 1554739347;
    timeStr = ToGmtTime(t);
    EXPECT_STREQ(timeStr.c_str(), "Mon, 08 Apr 2019 16:02:27 GMT");

    std::locale::global(oldLoc);
}

TEST_F(UtilsFunctionTest, ToUtcTimeTest)
{
    std::time_t t = 0;
    std::string timeStr = ToUtcTime(t);
    EXPECT_STREQ(timeStr.c_str(), "1970-01-01T00:00:00.000Z");

    t = 1520411719;
    timeStr = ToUtcTime(t);
    EXPECT_STREQ(timeStr.c_str(), "2018-03-07T08:35:19.000Z");
}

TEST_F(UtilsFunctionTest, ToUtcTimeWithSetlocaleTest)
{
    auto oldLoc = std::cout.getloc();
    std::locale::global(std::locale(""));

    std::time_t t = 0;
    std::string timeStr = ToUtcTime(t);
    EXPECT_STREQ(timeStr.c_str(), "1970-01-01T00:00:00.000Z");

    t = 1520411719;
    timeStr = ToUtcTime(t);
    EXPECT_STREQ(timeStr.c_str(), "2018-03-07T08:35:19.000Z");

    t = 1520433319;
    timeStr = ToUtcTime(t);
    EXPECT_STREQ(timeStr.c_str(), "2018-03-07T14:35:19.000Z");

    std::locale::global(oldLoc);
}

TEST_F(UtilsFunctionTest, UtcToUnixTimeTest)
{
    std::string date = "1970-01-01T00:00:00.000Z";
    std::time_t t = UtcToUnixTime(date);
    EXPECT_EQ(t, 0);

    date = "2018-03-07T08:35:19.123Z";
    t = UtcToUnixTime(date);
    EXPECT_EQ(t, 1520411719);

    //invalid case
    date = "2018-03-07T08:35:19Z";
    t = UtcToUnixTime(date);
    EXPECT_EQ(t, -1);

    date = "2018-03-07T08:35:19.abcZ";
    t = UtcToUnixTime(date);
    EXPECT_EQ(t, -1);

    date = "18-03-07T08:35:19.000Z";
    t = UtcToUnixTime(date);
    EXPECT_EQ(t, -1);

    date = "";
    t = UtcToUnixTime(date);
    EXPECT_EQ(t, -1);
}

TEST_F(UtilsFunctionTest, LookupMimeTypeTest)
{
    EXPECT_STREQ(LookupMimeType("name.html").c_str(), "text/html");
    EXPECT_STREQ(LookupMimeType("test.mp3").c_str(), "audio/mpeg");
    EXPECT_STREQ(LookupMimeType("test.mp3.unkonw").c_str(), "audio/mpeg");
    EXPECT_STREQ(LookupMimeType("test.mp3.unkonw.unkonw").c_str(), "application/octet-stream");
    EXPECT_STREQ(LookupMimeType("unkonw").c_str(), "application/octet-stream");
    EXPECT_STREQ(LookupMimeType("name.Html").c_str(), "text/html");
    EXPECT_STREQ(LookupMimeType("test.Mp3.unkonw").c_str(), "audio/mpeg");
}

struct Md5TestData {
    const char *msg;
    const unsigned char hash[16];
};
static Md5TestData tests[] = {
  { "",
    { 0xd4, 0x1d, 0x8c, 0xd9, 0x8f, 0x00, 0xb2, 0x04,
      0xe9, 0x80, 0x09, 0x98, 0xec, 0xf8, 0x42, 0x7e } },
  { "a",
    {0x0c, 0xc1, 0x75, 0xb9, 0xc0, 0xf1, 0xb6, 0xa8,
     0x31, 0xc3, 0x99, 0xe2, 0x69, 0x77, 0x26, 0x61 } },
  { "abc",
    { 0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0,
      0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72 } },
  { "message digest",
    { 0xf9, 0x6b, 0x69, 0x7d, 0x7c, 0xb7, 0x93, 0x8d,
      0x52, 0x5a, 0x2f, 0x31, 0xaa, 0xf1, 0x61, 0xd0 } },
  { "abcdefghijklmnopqrstuvwxyz",
    { 0xc3, 0xfc, 0xd3, 0xd7, 0x61, 0x92, 0xe4, 0x00,
      0x7d, 0xfb, 0x49, 0x6c, 0xca, 0x67, 0xe1, 0x3b } },
  { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
    { 0xd1, 0x74, 0xab, 0x98, 0xd2, 0x77, 0xd9, 0xf5,
      0xa5, 0x61, 0x1c, 0x2c, 0x9f, 0x41, 0x9d, 0x9f } },
  { "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
    { 0x57, 0xed, 0xf4, 0xa2, 0x2b, 0xe3, 0xc9, 0x55,
      0xac, 0x49, 0xda, 0x2e, 0x21, 0x07, 0xb6, 0x7a } }
};

TEST_F(UtilsFunctionTest, Md5CharArgTest)
{
    for (auto const &item : tests) {
        auto md5_base64_1 = ComputeContentMD5(item.msg, !item.msg? 0: strlen(item.msg));
        auto md5_base64_2 = Base64Encode((const char *)item.hash, 16);
        EXPECT_STREQ(md5_base64_1.c_str(), md5_base64_2.c_str());
    }
}

TEST_F(UtilsFunctionTest, Md5StringStreamArgTest)
{
    for (auto const &item : tests) {
        std::stringstream ss;
        ss << item.msg;
        auto md5_base64_1 = ComputeContentMD5(ss);
        auto md5_base64_2 = Base64Encode((const char *)item.hash, 16);
        EXPECT_STREQ(md5_base64_1.c_str(), md5_base64_2.c_str());
    }
}

TEST_F(UtilsFunctionTest, Md5NullptrTest)
{
    auto md5_base64_1 = ComputeContentMD5(nullptr, 0);
    EXPECT_EQ(md5_base64_1, "");
}

TEST_F(UtilsFunctionTest, Md5ResetContentPositionTest)
{
    std::string fileName = TestUtils::GetTargetFileName("Md5ResetContentPositionTest");
    TestUtils::WriteRandomDatatoFile(fileName, 1024);
    std::fstream of(fileName, std::ios::in | std::ios::binary);
    of.seekg(0, of.end);
    char buff[10];
    of.read(buff, 10);
    auto md5_base64_1 = ComputeContentMD5(of);
    of.close();
    auto md5_file = TestUtils::GetFileMd5(fileName);
    EXPECT_EQ(md5_base64_1, md5_file);
    RemoveFile(fileName);
}

struct ETagTestData {
    const char *msg;
    const char *hexHash;
};
static ETagTestData eTagTests[] = {
  { "",
    "D41D8CD98F00B204E9800998ECF8427E"}, 
  { "a",
    "0CC175B9C0F1B6A831C399E269772661"},
  { "abc",
    "900150983CD24FB0D6963F7D28E17F72"},
  { "message digest",
    "F96B697D7CB7938D525A2F31AAF161D0"},
  { "abcdefghijklmnopqrstuvwxyz",
    "C3FCD3D76192E4007DFB496CCA67E13B"},
  { "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
    "D174AB98D277D9F5A5611C2C9F419D9F"},
  { "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
    "57EDF4A22BE3C955AC49DA2E2107B67A"}
};

TEST_F(UtilsFunctionTest, ETagCharArgTest)
{
    for (auto const &item : eTagTests) {
        auto etag = ComputeContentETag(item.msg, !item.msg ? 0 : strlen(item.msg));
        EXPECT_STREQ(etag.c_str(), item.hexHash);
    }
}

TEST_F(UtilsFunctionTest, ETagStringStreamArgTest)
{
    for (auto const &item : eTagTests) {
        std::stringstream ss;
        ss << item.msg;
        auto etag = ComputeContentETag(ss);
        EXPECT_STREQ(etag.c_str(), item.hexHash);
    }
}

TEST_F(UtilsFunctionTest, ETagNullptrTest)
{
    auto etag = ComputeContentETag(nullptr, 0);
    EXPECT_EQ(etag, "");
}

TEST_F(UtilsFunctionTest, ETagResetContentPositionTest)
{
    std::string fileName = TestUtils::GetTargetFileName("ETagResetContentPositionTest");
    TestUtils::WriteRandomDatatoFile(fileName, 1024);
    std::fstream of(fileName, std::ios::in | std::ios::binary);
    of.seekg(0, of.end);
    char buff[10];
    of.read(buff, 10);
    auto etag = ComputeContentETag(of);
    of.close();
    EXPECT_FALSE(etag.empty());
    RemoveFile(fileName);
}


TEST_F(UtilsFunctionTest, GetIOStreamLengthResetContentPositionTest)
{
    std::string fileName = TestUtils::GetTargetFileName("GetIOStreamLengthResetContentPositionTest");
    TestUtils::WriteRandomDatatoFile(fileName, 1024);
    std::fstream of(fileName, std::ios::in | std::ios::binary);
    of.seekg(0, of.end);
    char buff[10];
    of.read(buff, 10);
    auto length = GetIOStreamLength(of);
    of.close();
    auto md5_file = TestUtils::GetFileMd5(fileName);
    EXPECT_EQ(length, 1024LL);
    RemoveFile(fileName);
}


TEST_F(UtilsFunctionTest, CombineHostStringTest)
{
    EXPECT_STREQ(CombineHostString("http://oss-cn-hangzhou.aliyuncs.com", "test-bucket", false).c_str(),
        "http://test-bucket.oss-cn-hangzhou.aliyuncs.com");
    EXPECT_STREQ(CombineHostString("oss-cn-hangzhou.aliyuncs.com", "test-bucket", false).c_str(),
        "http://test-bucket.oss-cn-hangzhou.aliyuncs.com");
    EXPECT_STREQ(CombineHostString("http://192.168.1.1", "test-bucket", false).c_str(),
        "http://192.168.1.1");

    EXPECT_STREQ(CombineHostString("http://cname.com", "test-bucket", true).c_str(),
        "http://cname.com");
    EXPECT_STREQ(CombineHostString("cname.com", "test-bucket", true).c_str(),
        "http://cname.com");
    EXPECT_STREQ(CombineHostString("http://192.168.1.1", "test-bucket", true).c_str(),
        "http://192.168.1.1");
}

TEST_F(UtilsFunctionTest, CombinePathStringTest)
{
    EXPECT_STREQ(CombinePathString("http://oss-cn-hangzhou.aliyuncs.com", "test-bucket", "test-key").c_str(),
        "/test-key");
    EXPECT_STREQ(CombinePathString("http://192.168.1.1", "test-bucket", "test-key").c_str(),
        "/test-bucket/test-key");
}

TEST_F(UtilsFunctionTest, CombineQueryStringTest)
{
    ParameterCollection parameters;
    parameters["empty"] = "";
    parameters["arg1"] = "a";
    EXPECT_STREQ(CombineQueryString(parameters).c_str(), "arg1=a&empty");

    parameters.clear();
    parameters["arg1"] = "a";
    EXPECT_STREQ(CombineQueryString(parameters).c_str(), "arg1=a");

    parameters.clear();
    parameters["arg1 arg1"] = "a a";
    EXPECT_STREQ(CombineQueryString(parameters).c_str(), "arg1%20arg1=a%20a");
}

TEST_F(UtilsFunctionTest, HostToIpTest)
{
    std::string ip = TestUtils::GetIpByEndpoint("oss-cn-hangzhou.aliyuncs.com");
    EXPECT_TRUE(TestUtils::IsValidIp(ip));
    //std::cout << "ip:" << ip << std::endl;
}

TEST_F(UtilsFunctionTest, Base64EncodeUrlSafeTest)
{
    const unsigned char buff[] = { 0x14, 0xFB, 0x9C, 0x03, 0xD9, 0x7E };
    size_t len = sizeof(buff) / sizeof(buff[0]);
    auto value = Base64EncodeUrlSafe((const char *)buff, static_cast<int>(len));
    EXPECT_EQ(value, "FPucA9l-");

    std::vector<std::string> ori = { "abc" , "abcd" , "abcde" };
    std::vector<std::string> pat = { "YWJj" , "YWJjZA" , "YWJjZGU" };

    auto i = ori.size();
    for (i = 0; i < ori.size(); i++) {
        auto result = Base64EncodeUrlSafe(ori[i]);
        EXPECT_STREQ(result.c_str(), pat[i].c_str());
    }
    EXPECT_TRUE((i == ori.size()));

}

TEST_F(UtilsFunctionTest, StringReplaceTest)
{
    std::string test = "1234abcdABCD1234";

    StringReplace(test, "abcd", "A");
    EXPECT_EQ(test, "1234AABCD1234");

    test = "12212";
    StringReplace(test, "12", "21");
    EXPECT_EQ(test, "21221");
}

TEST_F(UtilsFunctionTest, UploadAndDownloadObject)
{
    // create client and bucket
    auto BucketName = TestUtils::GetBucketName("utils-function-bucket-test");
    auto key = TestUtils::GetObjectKey("utils-function-object-test");
    std::shared_ptr<OssClient> Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    TestUtils::EnsureBucketExist(*Client, BucketName);

    // create local file
    std::string tmpFile = TestUtils::GetTargetFileName("UtilsFunctionObject").append(".tmp");
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024);
    ObjectMetaData meta;
    // upload obect
    auto uploadObjectOutcome = TestUtils::UploadObject(*Client, BucketName, key, tmpFile, meta);
    EXPECT_EQ(uploadObjectOutcome.isSuccess(), true);
    EXPECT_EQ(Client->DoesObjectExist(BucketName, key), true);

    // download object
    std::string targetFile = TestUtils::GetTargetFileName("TargetFile").append(".tmp");
    TestUtils::DownloadObject(*Client, BucketName, key, targetFile);

    EXPECT_EQ(GetFileLength(targetFile), GetFileLength(tmpFile));

    // delete bucket
    TestUtils::CleanBucket(*Client, BucketName);
    Client = nullptr;
    EXPECT_EQ(RemoveFile(tmpFile), true);
    EXPECT_EQ(RemoveFile(targetFile), true);
}

TEST_F(UtilsFunctionTest, IsValidChannelNameTest)
{
    EXPECT_FALSE(IsValidChannelName(""));
    EXPECT_FALSE(IsValidChannelName("abc/abc"));
    std::string value;
    for (int i = 0; i < 1300; i++)
        value.append("a");
    EXPECT_FALSE(IsValidChannelName(value));

    EXPECT_TRUE(IsValidChannelName("channelName"));
}

TEST_F(UtilsFunctionTest, IsValidPlayListNameTest)
{
    std::string longName;
    for (int i = 0; i < 130; i++)
        longName.append("a");
    std::string shortName;
    shortName = "aaa";

    EXPECT_FALSE(IsValidPlayListName(""));
    EXPECT_FALSE(IsValidPlayListName(longName));
    EXPECT_FALSE(IsValidPlayListName(shortName));

    EXPECT_FALSE(IsValidPlayListName("aaaa/aaa"));
    EXPECT_FALSE(IsValidPlayListName(".aaaaaaa"));
    EXPECT_FALSE(IsValidPlayListName("aaaaaaaa."));
    EXPECT_FALSE(IsValidPlayListName("aaaaaaaa.m4u8"));

    EXPECT_TRUE(IsValidPlayListName("aaaaaaaa.m3u8"));
}

TEST_F(UtilsFunctionTest, ToLiveChannelStatusNameTest)
{
    std::string str = ToLiveChannelStatusName(LiveChannelStatus::EnabledStatus);
    EXPECT_EQ(str, "enabled");

    str.clear();
    str = ToLiveChannelStatusName(LiveChannelStatus::DisabledStatus);
    EXPECT_EQ(str, "disabled");

    str.clear();
    str = ToLiveChannelStatusName(LiveChannelStatus::IdleStatus);
    EXPECT_EQ(str, "idle");

    str.clear();
    str = ToLiveChannelStatusName(LiveChannelStatus::LiveStatus);
    EXPECT_EQ(str, "live");

    str.clear();
    str = ToLiveChannelStatusName(LiveChannelStatus::UnknownStatus);
    EXPECT_EQ(str, "");
}

TEST_F(UtilsFunctionTest, ToLiveChannelStatusTypeTest)
{
    EXPECT_EQ(ToLiveChannelStatusType("enabled"), LiveChannelStatus::EnabledStatus);
    EXPECT_EQ(ToLiveChannelStatusType("EnaBled"), LiveChannelStatus::EnabledStatus);
    EXPECT_EQ(ToLiveChannelStatusType("Enabled"), LiveChannelStatus::EnabledStatus);
    EXPECT_EQ(ToLiveChannelStatusType("Disabled"), LiveChannelStatus::DisabledStatus);
    EXPECT_EQ(ToLiveChannelStatusType("DISabled"), LiveChannelStatus::DisabledStatus);
    EXPECT_EQ(ToLiveChannelStatusType("DISABLED"), LiveChannelStatus::DisabledStatus);
    EXPECT_EQ(ToLiveChannelStatusType("IDLE"), LiveChannelStatus::IdleStatus);
    EXPECT_EQ(ToLiveChannelStatusType("IDLe"), LiveChannelStatus::IdleStatus);
    EXPECT_EQ(ToLiveChannelStatusType("Live"), LiveChannelStatus::LiveStatus);
    EXPECT_EQ(ToLiveChannelStatusType("LIVE"), LiveChannelStatus::LiveStatus);
    EXPECT_EQ(ToLiveChannelStatusType("aaa"), LiveChannelStatus::UnknownStatus);
    EXPECT_EQ(ToLiveChannelStatusType("lived"), LiveChannelStatus::UnknownStatus);
}

TEST_F(UtilsFunctionTest, TwoHeaderCollectionInsertTest)
{
    HeaderCollection headers1;
    HeaderCollection headers2;
    HeaderCollection headers3;

    headers1["key1"] = "value1";
    headers1["key2"] = "value2";

    headers2["key1"] = "value1-1";
    headers2["key3"] = "value3";


    headers1.insert(headers2.begin(), headers2.end());
    headers1.insert(headers3.begin(), headers3.end());

    EXPECT_EQ(headers1.size(), 3U);
    EXPECT_EQ(headers1["key1"], "value1");
    EXPECT_EQ(headers1["key2"], "value2");
    EXPECT_EQ(headers1["key3"], "value3");
}

class StreamBuftest : public StreamBufProxy
{
public:
    StreamBuftest(std::iostream& stream) :
        StreamBufProxy(stream)
    {
    }

    void test()
    {
        overflow();
        pbackfail();
        showmanyc();
        underflow();
        uflow();
        char buf[5] = "test";
        int len = 0;
        xsgetn(buf, len);
        setbuf(buf, len);
        imbue(std::locale(""));
    }
};

TEST_F(UtilsFunctionTest, StreamBufFunctionTest)
{
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << "StreamBufFunctionTest";
    StreamBuftest buf(*content);
    buf.test();
}

TEST_F(UtilsFunctionTest, UrlFunctionTest)
{
    Url url1;
    Url url2;
    EXPECT_EQ(url1 == url2, true);
    url1.setHost("test.com");
    EXPECT_EQ(url1 != url2, true);
    url1.fragment();
    url1.authority();
    url1.setPort(1);
    url1.setUserName("test");
    url1.authority();
    url1.fromString("#test");
    std::string str;
    url1.fromString(str);

    url2.setFragment("test");
    EXPECT_EQ(url2.isEmpty(), false);

    Url url3;
    url3.isValid();
    url3.setUserName("test");
    url3.isValid();

    url1.port();
    url1.password();
    url1.path();
    url3.setAuthority(str);
    url3.setAuthority("@test:test");
    url3.setHost(str);
    url3.setPassword("test");
    url3.setUserInfo("test");
    url3.setUserInfo(":test");
    url3.setUserName("test");
    url3.userName();

    url2.toString();
    url2.userInfo();
    url2.setHost("test");
    url2.setUserName("test");
    url2.setPassword("test");
    url2.toString();
    url2.userInfo();
}

TEST_F(UtilsFunctionTest, SignerFunctionTest)
{
    HmacSha1Signer s;
    s.name();
    s.type();
    std::string str;
    s.generate(str, str);
}

TEST_F(UtilsFunctionTest, UtilBranchTest)
{
    Base64Encode(nullptr, 1);
    ToUpper(nullptr);

    ToSSEAlgorithm("AES256");
    ToDataRedundancyType("ZRS");

    ObjectMetaData meta;
    meta.addUserHeader("test1","test");
    meta.removeUserHeader("test");
}

TEST_F(UtilsFunctionTest, XmlEscapeTest)
{
    EXPECT_EQ(XmlEscape(""), "");
    EXPECT_EQ(XmlEscape("\"<"), "&quot;&lt;");
    EXPECT_EQ(XmlEscape("abdeef"), "abdeef");
    EXPECT_EQ(XmlEscape(">abc<efj\"hij\'lmn&"), "&gt;abc&lt;efj&quot;hij&apos;lmn&amp;");

    std::string str;
    str.push_back((char)0xE6);
    str.push_back((char)0xB5);
    str.push_back((char)0x8B);
    str.push_back((char)0xE8);
    str.push_back((char)0xAF);
    str.push_back((char)0x95);
    EXPECT_EQ(XmlEscape(str), str);
}

TEST_F(UtilsFunctionTest, ToInventoryFormatNameTest)
{
    EXPECT_STREQ(ToInventoryFormatName(InventoryFormat::NotSet), "NotSet");
    EXPECT_STREQ(ToInventoryFormatName(InventoryFormat::CSV), "CSV");
}

TEST_F(UtilsFunctionTest, ToInventoryFormatTypeTest)
{
    EXPECT_EQ(ToInventoryFormatType("NotSet"), InventoryFormat::NotSet);
    EXPECT_EQ(ToInventoryFormatType("notSet"), InventoryFormat::NotSet);
    EXPECT_EQ(ToInventoryFormatType("Invalid"), InventoryFormat::NotSet);
    EXPECT_EQ(ToInventoryFormatType("CSV"), InventoryFormat::CSV);
    EXPECT_EQ(ToInventoryFormatType("csv"), InventoryFormat::CSV);
}

TEST_F(UtilsFunctionTest, ToInventoryFrequencyNameTest)
{
    EXPECT_STREQ(ToInventoryFrequencyName(InventoryFrequency::NotSet), "NotSet");
    EXPECT_STREQ(ToInventoryFrequencyName(InventoryFrequency::Daily), "Daily");
    EXPECT_STREQ(ToInventoryFrequencyName(InventoryFrequency::Weekly), "Weekly");
}

TEST_F(UtilsFunctionTest, ToInventoryFrequencyTypeTest)
{
    EXPECT_EQ(ToInventoryFrequencyType("NotSet"), InventoryFrequency::NotSet);
    EXPECT_EQ(ToInventoryFrequencyType("notSet"), InventoryFrequency::NotSet);
    EXPECT_EQ(ToInventoryFrequencyType("Invalid"), InventoryFrequency::NotSet);
    EXPECT_EQ(ToInventoryFrequencyType("Daily"), InventoryFrequency::Daily);
    EXPECT_EQ(ToInventoryFrequencyType("daily"), InventoryFrequency::Daily);
    EXPECT_EQ(ToInventoryFrequencyType("Weekly"), InventoryFrequency::Weekly);
    EXPECT_EQ(ToInventoryFrequencyType("weekly"), InventoryFrequency::Weekly);
}

TEST_F(UtilsFunctionTest, ToInventoryOptionalFieldNameTest)
{
    EXPECT_STREQ(ToInventoryOptionalFieldName(InventoryOptionalField::NotSet), "NotSet");
    EXPECT_STREQ(ToInventoryOptionalFieldName(InventoryOptionalField::Size), "Size");
    EXPECT_STREQ(ToInventoryOptionalFieldName(InventoryOptionalField::LastModifiedDate), "LastModifiedDate");
    EXPECT_STREQ(ToInventoryOptionalFieldName(InventoryOptionalField::ETag), "ETag");
    EXPECT_STREQ(ToInventoryOptionalFieldName(InventoryOptionalField::StorageClass), "StorageClass");
    EXPECT_STREQ(ToInventoryOptionalFieldName(InventoryOptionalField::IsMultipartUploaded), "IsMultipartUploaded");
    EXPECT_STREQ(ToInventoryOptionalFieldName(InventoryOptionalField::EncryptionStatus), "EncryptionStatus");
}

TEST_F(UtilsFunctionTest, ToInventoryOptionalFieldTypeTest)
{
    EXPECT_EQ(ToInventoryOptionalFieldType("NotSet"), InventoryOptionalField::NotSet);
    EXPECT_EQ(ToInventoryOptionalFieldType("notSet"), InventoryOptionalField::NotSet);
    EXPECT_EQ(ToInventoryOptionalFieldType("Invalid"), InventoryOptionalField::NotSet);
    EXPECT_EQ(ToInventoryOptionalFieldType("Size"), InventoryOptionalField::Size);
    EXPECT_EQ(ToInventoryOptionalFieldType("size"), InventoryOptionalField::Size);
    EXPECT_EQ(ToInventoryOptionalFieldType("LastModifiedDate"), InventoryOptionalField::LastModifiedDate);
    EXPECT_EQ(ToInventoryOptionalFieldType("lastModifiedDate"), InventoryOptionalField::LastModifiedDate);
    EXPECT_EQ(ToInventoryOptionalFieldType("ETag"), InventoryOptionalField::ETag);
    EXPECT_EQ(ToInventoryOptionalFieldType("eTag"), InventoryOptionalField::ETag);
    EXPECT_EQ(ToInventoryOptionalFieldType("StorageClass"), InventoryOptionalField::StorageClass);
    EXPECT_EQ(ToInventoryOptionalFieldType("storageClass"), InventoryOptionalField::StorageClass);
    EXPECT_EQ(ToInventoryOptionalFieldType("IsMultipartUploaded"), InventoryOptionalField::IsMultipartUploaded);
    EXPECT_EQ(ToInventoryOptionalFieldType("isMultipartUploaded"), InventoryOptionalField::IsMultipartUploaded);
    EXPECT_EQ(ToInventoryOptionalFieldType("EncryptionStatus"), InventoryOptionalField::EncryptionStatus);
    EXPECT_EQ(ToInventoryOptionalFieldType("encryptionStatus"), InventoryOptionalField::EncryptionStatus);
}


TEST_F(UtilsFunctionTest, ToInventoryIncludedObjectVersionsNameTest)
{
    EXPECT_STREQ(ToInventoryIncludedObjectVersionsName(InventoryIncludedObjectVersions::NotSet), "NotSet");
    EXPECT_STREQ(ToInventoryIncludedObjectVersionsName(InventoryIncludedObjectVersions::All), "All");
    EXPECT_STREQ(ToInventoryIncludedObjectVersionsName(InventoryIncludedObjectVersions::Current), "Current");
}

TEST_F(UtilsFunctionTest, ToInventoryIncludedObjectVersionsTypeTest)
{
    EXPECT_EQ(ToInventoryIncludedObjectVersionsType("NotSet"), InventoryIncludedObjectVersions::NotSet);
    EXPECT_EQ(ToInventoryIncludedObjectVersionsType("notSet"), InventoryIncludedObjectVersions::NotSet);
    EXPECT_EQ(ToInventoryIncludedObjectVersionsType("Invalid"), InventoryIncludedObjectVersions::NotSet);
    EXPECT_EQ(ToInventoryIncludedObjectVersionsType("All"), InventoryIncludedObjectVersions::All);
    EXPECT_EQ(ToInventoryIncludedObjectVersionsType("all"), InventoryIncludedObjectVersions::All);
    EXPECT_EQ(ToInventoryIncludedObjectVersionsType("Current"), InventoryIncludedObjectVersions::Current);
    EXPECT_EQ(ToInventoryIncludedObjectVersionsType("current"), InventoryIncludedObjectVersions::Current);
}

TEST_F(UtilsFunctionTest, ToInventoryBucketFullNameTest)
{
    EXPECT_EQ(ToInventoryBucketFullName("bucket-name"), "acs:oss:::bucket-name");
    EXPECT_EQ(ToInventoryBucketFullName(""), "acs:oss:::");
    EXPECT_EQ(ToInventoryBucketFullName("test"), "acs:oss:::test");
}

TEST_F(UtilsFunctionTest, ToInventoryBucketShortNameTest)
{
    EXPECT_EQ(ToInventoryBucketShortName("acs:oss:::bucket-name"), "bucket-name");
    EXPECT_EQ(ToInventoryBucketShortName(nullptr), "");
    EXPECT_EQ(ToInventoryBucketShortName("acs:oss:::test"), "test");
    EXPECT_EQ(ToInventoryBucketShortName("ACS:OSS:::test"), "test");
}

TEST_F(UtilsFunctionTest, ToTierTypeNameTest)
{
    EXPECT_STREQ(ToTierTypeName(TierType::Standard), "Standard");
    EXPECT_STREQ(ToTierTypeName(TierType::Expedited), "Expedited");
    EXPECT_STREQ(ToTierTypeName(TierType::Bulk), "Bulk");
}

TEST_F(UtilsFunctionTest, ToTierTypeTest)
{
    EXPECT_EQ(ToTierType("Standard"), TierType::Standard);
    EXPECT_EQ(ToTierType("standard"), TierType::Standard);
    EXPECT_EQ(ToTierType("Expedited"), TierType::Expedited);
    EXPECT_EQ(ToTierType("expedited"), TierType::Expedited);
    EXPECT_EQ(ToTierType("Bulk"), TierType::Bulk);
    EXPECT_EQ(ToTierType("bulk"), TierType::Bulk);
}

TEST_F(UtilsFunctionTest, JsonStringToMapTest)
{
    std::string jsonStr1 = R"({"comment":"rsa test","provider":"aliclould"})";
    std::string jsonStr2 = R"({"provider":"aliclould","comment":"rsa test"})";
    std::string jsonStr3 = R"({"comment":"kms test","provider":"aliclould"})";

    auto map1 = JsonStringToMap(jsonStr1);
    auto map2 = JsonStringToMap(jsonStr2);
    auto map3 = JsonStringToMap(jsonStr3);

    EXPECT_EQ(map1, map2);
    EXPECT_NE(map1, map3);

    EXPECT_EQ(map1["comment"], "rsa test");
    EXPECT_EQ(map1["provider"], "aliclould");

    EXPECT_EQ(map3["comment"], "kms test");
    EXPECT_EQ(map3["provider"], "aliclould");
}

TEST_F(UtilsFunctionTest, MapToJsonStringTest)
{
    std::string jsonStr1 = R"({"comment":"rsa test","provider":"aliclould"})";
    std::string jsonStr3 = R"({"comment":"kms test","provider":"aliclould"})";

    std::map<std::string, std::string> map1;
    map1["comment"] = "rsa test";
    map1["provider"] = "aliclould";
    auto json1 = MapToJsonString(map1);

    std::map<std::string, std::string> map2;
    map2["provider"] = "aliclould";
    map2["comment"] = "rsa test";
    auto json2 = MapToJsonString(map2);

    std::map<std::string, std::string> map3;
    map3["provider"] = "aliclould";
    map3["comment"] = "kms test";
    auto json3 = MapToJsonString(map3);
       
    EXPECT_EQ(json1, jsonStr1);
    EXPECT_EQ(json3, jsonStr3);
    EXPECT_EQ(json1, json2);
}

TEST_F(UtilsFunctionTest, IsValidEndpointTest)
{
    EXPECT_EQ(IsValidEndpoint("192.168.1.1"), true);
    EXPECT_EQ(IsValidEndpoint("192.168.1.1:80"), true);
    EXPECT_EQ(IsValidEndpoint("www.test-inc.com"), true);
    EXPECT_EQ(IsValidEndpoint("WWW.test-inc_CN.com"), true);
    EXPECT_EQ(IsValidEndpoint("http://www.test-inc.com"), true);
    EXPECT_EQ(IsValidEndpoint("http://www.test-inc_test.com:80"), true);
    EXPECT_EQ(IsValidEndpoint("https://www.test-inc_test.com:80/test?123=x"), true);
    EXPECT_EQ(IsValidEndpoint("www.test-inc*test.com"), false);
    EXPECT_EQ(IsValidEndpoint("www.test-inc.com\\oss-cn-hangzhou.aliyuncs.com"), false);
    EXPECT_EQ(IsValidEndpoint(""), false);
}

}
}
