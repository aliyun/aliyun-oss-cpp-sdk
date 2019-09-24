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
#include <src/utils/Crc64.h>
#include "../Config.h"
#include "../Utils.h"
#include <alibabacloud/oss/OssClient.h>
#include "src/utils/LogUtils.h"

namespace AlibabaCloud {
namespace OSS {

class LogTest : public ::testing::Test {
protected:
    LogTest()
    {
    }

    ~LogTest() override
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
        Client = nullptr;
    }

public:
    static void LogCallbackFunc(LogLevel level, const std::string &stream);
    static std::shared_ptr<OssClient> Client;
    static std::string LogString;
};

std::shared_ptr<OssClient> LogTest::Client = nullptr;
std::string LogTest::LogString = "";

void LogTest::LogCallbackFunc(LogLevel level, const std::string &stream)
{
    LogString = stream;
    std::cout << stream;
    if (level == LogLevel::LogOff) {
        LogString.append("haha");
    }
}

TEST_F(LogTest, DisableLogLevelTest)
{
    SetLogLevel(LogLevel::LogOff);
    LogString = "";
    auto outcome = Client->ListBuckets();
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(LogString.empty(), true);
}

TEST_F(LogTest, DisableLogCallbackTest)
{
    SetLogLevel(LogLevel::LogAll);
    SetLogCallback(nullptr);
    LogString = "";
    auto outcome = Client->ListBuckets();
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(LogString.empty(), true);
    SetLogLevel(LogLevel::LogOff);
}

TEST_F(LogTest, EnableLogTest)
{
    SetLogLevel(LogLevel::LogAll);
    SetLogCallback(LogCallbackFunc);
    LogString = "";
    auto outcome = Client->ListBuckets();
    if (!outcome.isSuccess()) {
        TestUtils::WaitForCacheExpire(2);
        outcome = Client->ListBuckets();
    }
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_EQ(LogString.empty(), false);
    SetLogLevel(LogLevel::LogOff);

}

TEST_F(LogTest, LogMacroTest)
{
    SetLogLevel(LogLevel::LogAll);
    SetLogCallback(LogCallbackFunc);
    LogString = "";

    OSS_LOG(LogLevel::LogFatal, "LogTest", "LogMacroTest%s","Fatal");
    EXPECT_TRUE(strstr(LogString.c_str(), "[FATAL]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "[LogTest]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "LogMacroTestFatal") != nullptr);
    std::cout << LogString;

    LogString = "";
    OSS_LOG(LogLevel::LogError, "LogTest", "LogMacroTest%s", "Error");
    EXPECT_TRUE(strstr(LogString.c_str(), "[ERROR]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "[LogTest]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "LogMacroTestError") != nullptr);
    std::cout << LogString;

    LogString = "";
    OSS_LOG(LogLevel::LogWarn, "LogTest", "LogMacroTest%s", "Warn");
    EXPECT_TRUE(strstr(LogString.c_str(), "[WARN]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "[LogTest]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "LogMacroTestWarn") != nullptr);

    LogString = "";
    OSS_LOG(LogLevel::LogInfo, "LogTest", "LogMacroTest%s", "Info");
    EXPECT_TRUE(strstr(LogString.c_str(), "[INFO]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "[LogTest]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "LogMacroTestInfo") != nullptr);

    LogString = "";
    OSS_LOG(LogLevel::LogDebug, "LogTest", "LogMacroTest%s", "Debug");
    EXPECT_TRUE(strstr(LogString.c_str(), "[DEBUG]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "[LogTest]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "LogMacroTestDebug") != nullptr);

    LogString = "";
    OSS_LOG(LogLevel::LogTrace, "LogTest", "LogMacroTest%s", "Trace");
    EXPECT_TRUE(strstr(LogString.c_str(), "[TRACE]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "[LogTest]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "LogMacroTestTrace") != nullptr);

    LogString = "";
    OSS_LOG(LogLevel::LogOff, "LogTest", "LogMacroTest%s", "Off");
    EXPECT_TRUE(strstr(LogString.c_str(), "[OFF]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "[LogTest]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "LogMacroTestOff") != nullptr);

    LogString = "";
    OSS_LOG(LogLevel::LogAll, "LogTest", "LogMacroTest%s", "All");
    EXPECT_TRUE(strstr(LogString.c_str(), "[ALL]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "[LogTest]") != nullptr);
    EXPECT_TRUE(strstr(LogString.c_str(), "LogMacroTestAll") != nullptr);
    SetLogLevel(LogLevel::LogOff);
}

TEST_F(LogTest, EndofLineTest)
{
    SetLogLevel(LogLevel::LogAll);
    SetLogCallback(LogCallbackFunc);
    LogString = "";

    OSS_LOG(LogLevel::LogTrace, "LogTest", "LogMacroTest%s\n", "Trace");
    EXPECT_EQ(LogString.c_str()[LogString.size()-1], '\n');
    EXPECT_EQ(LogString.c_str()[LogString.size() - 2], 'e');
    std::cout << LogString;

    LogString = "";
    OSS_LOG(LogLevel::LogTrace, "LogTest", "LogMacroTest%s\n\n\n", "Trace");
    EXPECT_EQ(LogString.c_str()[LogString.size()-1], '\n');
    EXPECT_EQ(LogString.c_str()[LogString.size() - 2], 'e');
    std::cout << LogString;

    LogString = "";
    OSS_LOG(LogLevel::LogTrace, "LogTest", "LogMacroTest%s", "Trace");
    EXPECT_EQ(LogString.c_str()[LogString.size() - 1], '\n');
    EXPECT_EQ(LogString.c_str()[LogString.size() - 2], 'e');
    std::cout << LogString;

    LogString = "";
    OSS_LOG(LogLevel::LogTrace, "LogTest", "");
    EXPECT_EQ(LogString.c_str()[LogString.size() - 1], '\n');
    EXPECT_EQ(LogString.c_str()[LogString.size() - 2], ']');
    std::cout << LogString;
    SetLogLevel(LogLevel::LogOff);
}

}
}
