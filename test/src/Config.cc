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
#include <string.h>
#include <string>
#include "Config.h"
#include <fstream>
#include <algorithm>

static std::string dataPath_;
#ifdef _WIN32
#define delimiter "\\"
#pragma warning( disable : 4996)
#else
#define delimiter "/"
#endif

std::string Config::AccessKeyId = "";
std::string Config::AccessKeySecret = "";
std::string Config::Endpoint = "";
std::string Config::SecondEndpoint = "";
std::string Config::CallbackServer = "";
std::string Config::CfgFilePath = "oss.ini";
std::string Config::PayerAccessKeyId = "";
std::string Config::PayerAccessKeySecret = "";
std::string Config::PayerUID = "";
std::string Config::RamRoleArn = "";
std::string Config::RamUID = "";

static std::string LeftTrim(const char* source)
{
    std::string copy(source);
    copy.erase(copy.begin(), std::find_if(copy.begin(), copy.end(), [](unsigned char ch) { return !::isspace(ch); }));
    return copy;
}

static std::string RightTrim(const char* source)
{
    std::string copy(source);
    copy.erase(std::find_if(copy.rbegin(), copy.rend(), [](unsigned char ch) { return !::isspace(ch); }).base(), copy.end());
    return copy;
}

static std::string Trim(const char* source)
{
    return LeftTrim(RightTrim(source).c_str());
}

static std::string LeftTrimQuotes(const char* source)
{
    std::string copy(source);
    copy.erase(copy.begin(), std::find_if(copy.begin(), copy.end(), [](int ch) { return !(ch == '"'); }));
    return copy;
}

static std::string RightTrimQuotes(const char* source)
{
    std::string copy(source);
    copy.erase(std::find_if(copy.rbegin(), copy.rend(), [](int ch) { return !(ch == '"'); }).base(), copy.end());
    return copy;
}

static std::string TrimQuotes(const char* source)
{
    return LeftTrimQuotes(RightTrimQuotes(source).c_str());
}

static bool hasCfgInfo()
{
    if (!Config::AccessKeyId.empty() &&
        !Config::AccessKeySecret.empty() &&
        !Config::Endpoint.empty()) {
        return true;
    }
    return false;
}

static void LoadCfgFromFile()
{
    if (hasCfgInfo())
        return;

    std::fstream in(Config::CfgFilePath, std::ios::in | std::ios::binary);
    if (!in.good())
        return;

    char buffer[256];
    char *ptr;

    while (in.getline(buffer, 256)) {
        ptr = strchr(buffer, '=');
        if (!ptr) {
            continue;
        }
        if (!strncmp(buffer, "AccessKeyId", 11)) {
            Config::AccessKeyId = TrimQuotes(Trim(ptr + 1).c_str());
        }
        else if (!strncmp(buffer, "AccessKeySecret", 15)) {
            Config::AccessKeySecret = TrimQuotes(Trim(ptr + 1).c_str());
        }
        else if (!strncmp(buffer, "Endpoint", 8)) {
            Config::Endpoint = TrimQuotes(Trim(ptr + 1).c_str());
        }
        else if (!strncmp(buffer, "SecondEndpoint", 14)) {
            Config::SecondEndpoint = TrimQuotes(Trim(ptr + 1).c_str());
        }
        else if (!strncmp(buffer, "CallbackServer", 14)) {
            Config::CallbackServer = TrimQuotes(Trim(ptr + 1).c_str());
        }
        else if (!strncmp(buffer, "PayerAccessKeyId", 16)) {
            Config::PayerAccessKeyId = TrimQuotes(Trim(ptr + 1).c_str());
        }
        else if (!strncmp(buffer, "PayerAccessKeySecret", 20)) {
            Config::PayerAccessKeySecret = TrimQuotes(Trim(ptr + 1).c_str());
        }
        else if (!strncmp(buffer, "PayerUID", 8)) {
            Config::PayerUID = TrimQuotes(Trim(ptr + 1).c_str());
        }
        else if (!strncmp(buffer, "RamRoleArn", 10)) {
            Config::RamRoleArn = TrimQuotes(Trim(ptr + 1).c_str());
        }
        else if (!strncmp(buffer, "RamUID", 6)) {
            Config::RamUID = TrimQuotes(Trim(ptr + 1).c_str());
        }
    }
    in.close();
}

static void LoadCfgFromEnv()
{
    if (hasCfgInfo())
        return;

    const char *value;
    value = std::getenv("TEST_ACCESS_KEY_ID");
    if (value) {
        Config::AccessKeyId = TrimQuotes(Trim(value).c_str());
    }

    value = std::getenv("TEST_ACCESS_KEY_SECRET");
    if (value) {
        Config::AccessKeySecret = TrimQuotes(Trim(value).c_str());
    }

    value = std::getenv("TEST_OSS_ENDPOINT");
    if (value) {
        Config::Endpoint = TrimQuotes(Trim(value).c_str());
    }

    value = std::getenv("TEST_OSS_CALLBACKSERVER");
    if (value) {
        Config::CallbackServer = TrimQuotes(Trim(value).c_str());
    }

    value = std::getenv("TEST_OSS_PAYER_ACCESS_KEY_ID");
    if (value) {
        Config::PayerAccessKeyId = TrimQuotes(Trim(value).c_str());
    }

    value = std::getenv("TEST_OSS_PAYER_ACCESS_KEY_SECRET");
    if (value) {
        Config::PayerAccessKeySecret = TrimQuotes(Trim(value).c_str());
    }

    value = std::getenv("TEST_OSS_PAYER_UID");
    if (value) {
        Config::PayerUID = TrimQuotes(Trim(value).c_str());
    }

    value = std::getenv("TEST_OSS_STS_ARN");
    if (value) {
        Config::RamRoleArn = TrimQuotes(Trim(value).c_str());
    }

    value = std::getenv("TEST_OSS_ACCOUNT_ID");
    if (value) {
        Config::RamUID = TrimQuotes(Trim(value).c_str());
    }
}

bool Config::InitTestEnv()
{
    /*Get from Env*/
    LoadCfgFromEnv();

    /*Get from file*/
    LoadCfgFromFile();

    /*Inti Data Path*/
    std::string path = __FILE__;
    auto last_pos = path.rfind("src");
    path = path.substr(0, last_pos);
    path.append("data").append(delimiter);
    dataPath_ = path;

    return hasCfgInfo();
}

std::string Config::GetDataPath()
{
    return dataPath_;
}

void Config::ParseArg(int argc, char **argv)
{
    int i = 1;
    while (i < argc)
    {
        if (argv[i][0] == '-') 
        {
            if (!strcmp("-oss_cfg", argv[i]) && (i+1) < argc) {
                Config::CfgFilePath = Trim(argv[i + 1]);
                i++;
            }
        }
        i++;
    }
}
