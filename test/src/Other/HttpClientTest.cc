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
#include <alibabacloud/oss/client/RateLimiter.h>
#include <src/http/CurlHttpClient.h>
#include "../Config.h"
#include "../Utils.h"
#include <chrono>
#include <future>
#include <fstream>
#include "src/utils/FileSystemUtils.h"
#include "src/utils/Utils.h"
#ifdef GetObject
#undef GetObject
#endif // GetObject


namespace AlibabaCloud {
namespace OSS {

class HttpClientTest : public ::testing::Test {
protected:
    HttpClientTest()
    {
    }

    ~HttpClientTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-httpclienttest");
        Client->CreateBucket(CreateBucketRequest(BucketName));
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
        TestUtils::CleanBucket(*Client, BucketName);
        Client = nullptr;
    }


    void SetUp() override
    {
    }

    void TearDown() override
    {
    }

public:
    static std::shared_ptr<OssClient> Client;
    static std::string BucketName;

    class Timer
    {
    public:
        Timer() : begin_(std::chrono::high_resolution_clock::now()) {}
        void reset() { begin_ = std::chrono::high_resolution_clock::now(); }
        int64_t elapsed() const
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - begin_).count();
        }
        int64_t elapsed_micro() const
        {
            return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - begin_).count();
        }
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> begin_;
    };
};

std::shared_ptr<OssClient> HttpClientTest::Client = nullptr;
std::string HttpClientTest::BucketName = "";

TEST_F(HttpClientTest, WaitForRetryTimeoutTest)
{
    ClientConfiguration conf;
    CurlHttpClient httpClient(conf);
    Timer timer;
    long timeouts[] = { 0, 1000, 2000, 3000, 5500, 10000 };
    for (auto t : timeouts) {
        timer.reset();
        httpClient.waitForRetry(t);
        long elapsed = static_cast<long>(timer.elapsed());
        EXPECT_NEAR(elapsed, t, 100);
        EXPECT_TRUE(httpClient.isEnable());
    }
}

TEST_F(HttpClientTest, WaitForRetryTimeoutAsyncTest)
{
    ClientConfiguration conf;
    CurlHttpClient httpClient(conf);
    CurlHttpClient *client = &httpClient;
    Timer timer;
    long timeouts[] = { 1000, 2000, 3000, 5500, 10000 };
    for (auto t : timeouts) {
        timer.reset();
        auto f = std::async(std::launch::async, [client, t]()->void { client->waitForRetry(t); });
        f.get();
        long elapsed = static_cast<long>(timer.elapsed());
        EXPECT_NEAR(elapsed, t, 100);
    }
}

TEST_F(HttpClientTest, WaitForRetryTimeoutDisableTest)
{
    ClientConfiguration conf;
    CurlHttpClient httpClient(conf);
    httpClient.disable();
    Timer timer;
    long timeouts[] = { 1000, 2000, 3000, 5500, 10000 };
    for (auto t : timeouts) {
        timer.reset();
        httpClient.waitForRetry(t);
        long elapsed = static_cast<long>(timer.elapsed());
        EXPECT_NEAR(elapsed, 0, 100);
    }
}

TEST_F(HttpClientTest, WaitForRetryTimeoutInterruptTest)
{
    ClientConfiguration conf;
    CurlHttpClient httpClient(conf);
    CurlHttpClient *client = &httpClient;
    Timer timer;
    long timeouts[] = { 1000, 2000, 3000, 5500, 10000 };
    for (auto t : timeouts) {
        timer.reset();
        client->enable();
        auto f = std::async(std::launch::async, [client, t]()->void { client->waitForRetry(t); });
        f.wait_for(std::chrono::milliseconds(t / 2));
        client->disable();
        f.get();
        long elapsed = static_cast<long>(timer.elapsed());
        EXPECT_NEAR(elapsed, t/2, 100);
        EXPECT_FALSE(client->isEnable());
    }
}

TEST_F(HttpClientTest, ResourceManagerWithOneConnnectionTest)
{
    ClientConfiguration conf;
    conf.maxConnections = 1;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    auto keyPrefix  = TestUtils::GetObjectKey("ResourceManagerWithOneConnnectionTest");

    std::vector<PutObjectOutcomeCallable> Callables;

    //SetLogLevel(LogLevel::LogAll);
    //SetLogCallback(TestUtils::LogPrintCallback);

    for (int i = 0; i < 5; i++) {
        std::string key = keyPrefix;
        key.append("-").append(std::to_string(i));
        auto content = TestUtils::GetRandomStream(1024);
        PutObjectRequest request(BucketName, key, content);
        auto outcomeCallable = client.PutObjectCallable(request);
        Callables.emplace_back(std::move(outcomeCallable));
    }

    for (size_t i = 0; i < Callables.size(); i++) {
        auto outcome = Callables[i].get();
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_FALSE(outcome.result().ETag().empty());
    }

    //SetLogLevel(LogLevel::LogOff);
    //SetLogCallback(nullptr);
}

TEST_F(HttpClientTest, ResourceManagerWithTwoConnnectionTest)
{
    ClientConfiguration conf;
    conf.maxConnections = 2;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    auto keyPrefix = TestUtils::GetObjectKey("ResourceManagerWithTwoConnnectionTest");

    std::vector<PutObjectOutcomeCallable> Callables;

    //SetLogLevel(LogLevel::LogAll);
    //SetLogCallback(TestUtils::LogPrintCallback);

    for (int i = 0; i < 5; i++) {
        std::string key = keyPrefix;
        key.append("-").append(std::to_string(i));
        auto content = TestUtils::GetRandomStream(1024);
        PutObjectRequest request(BucketName, key, content);
        auto outcomeCallable = client.PutObjectCallable(request);
        Callables.emplace_back(std::move(outcomeCallable));
    }

    for (size_t i = 0; i < Callables.size(); i++) {
        auto outcome = Callables[i].get();
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_FALSE(outcome.result().ETag().empty());
    }

    //SetLogLevel(LogLevel::LogOff);
    //SetLogCallback(nullptr);
}

TEST_F(HttpClientTest, HttpRequestTest)
{
    HttpRequest request(Http::Get);
    EXPECT_EQ(request.method(), Http::Get);

    request.setMethod(Http::Put);
    EXPECT_EQ(request.method(), Http::Put);
}

TEST_F(HttpClientTest, HttpResponseTest)
{
    auto request = std::make_shared<HttpRequest>(Http::Get);
    request->setResponseStreamFactory([=]() { return nullptr; });
    HttpResponse response(request);
    const char * msg = "just for test";
    response.setStatusMsg(msg);
    EXPECT_STREQ(response.statusMsg().c_str(), msg);
    std::string msgStr("just for test");
    response.setStatusMsg(msgStr);
    EXPECT_EQ(response.statusMsg(), msgStr);
}

TEST_F(HttpClientTest, DisableEnableRequestTest)
{
    ClientConfiguration conf;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    client.DisableRequest();
    auto outcome = client.ListBuckets();
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ClientError:100002");

    client.EnableRequest();
    outcome = client.ListBuckets();
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(HttpClientTest, DisableRequestWhenDoingTest)
{
    ClientConfiguration conf;
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    auto keyPrefix = TestUtils::GetObjectKey("DisableRequestAfterMakeRequestTest");
    auto tmpFile = TestUtils::GetTargetFileName("DisableRequestAfterMakeRequestTest");
    TestUtils::WriteRandomDatatoFile(tmpFile, 50 * 1024 * 1024);

    std::vector<PutObjectOutcomeCallable> Callables;

    SetLogLevel(LogLevel::LogAll);
    SetLogCallback(TestUtils::LogPrintCallback);

    for (int i = 0; i < 4; i++) {
        std::string key = keyPrefix;
        key.append("-").append(std::to_string(i));
        auto content = std::make_shared<std::fstream>(tmpFile, std::ios::in | std::ios::binary);
        PutObjectRequest request(BucketName, key, content);
        auto outcomeCallable = client.PutObjectCallable(request);
        Callables.emplace_back(std::move(outcomeCallable));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    client.DisableRequest();

    for (size_t i = 0; i < Callables.size(); i++) {
        auto outcome = Callables[i].get();
        EXPECT_EQ(outcome.isSuccess(), false);
        if (outcome.error().Code() == "ClientError:100002" ||
            outcome.error().Code() == "ClientError:200042") {
            EXPECT_TRUE(true);
        }
        else {
            EXPECT_TRUE(false);
        }
    }

    SetLogLevel(LogLevel::LogOff);
    SetLogCallback(nullptr);
    RemoveFile(tmpFile);
}

TEST_F(HttpClientTest, PutObjectWithSameContentTest)
{
    //seek to the first position when request done (fail or sucess).
    std::string key = TestUtils::GetObjectKey("PutObjectWithSameContentTest");
    auto content = TestUtils::GetRandomStream(1024);

    PutObjectRequest request(BucketName, key, content);
    auto pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    pOutcome = Client->PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    auto outome = Client->GetObject(BucketName, key);
    EXPECT_EQ(outome.isSuccess(), true);

    std::string oriMd5 = ComputeContentMD5(*content);
    std::string memMd5 = ComputeContentMD5(*outome.result().Content());
    EXPECT_EQ(oriMd5, memMd5);
}

class  DefaultRateLimiter : public RateLimiter
{
public:
    DefaultRateLimiter() :rate_(0) {};
    ~DefaultRateLimiter() {};
    virtual void setRate(int rate) { rate_ = rate; };
    virtual int Rate() const { return rate_; };
private:
    int rate_;
};


TEST_F(HttpClientTest, GetObjectToSameContentTest)
{
    ClientConfiguration conf;
    auto rateLimiter = std::make_shared<DefaultRateLimiter>();
    conf.recvRateLimiter = rateLimiter;
    auto client = OssClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);

    //seek to the first position when get object fail.
    std::string key = TestUtils::GetObjectKey("GetObjectToSameContentTest");
    auto content = TestUtils::GetRandomStream(2*1024*1024);

    PutObjectRequest request(BucketName, key, content);
    auto pOutcome = client.PutObject(request);
    EXPECT_EQ(pOutcome.isSuccess(), true);

    //SetLogLevel(LogLevel::LogAll);
    //SetLogCallback(TestUtils::LogPrintCallback);

    rateLimiter->setRate(256);
    auto tmpFile = TestUtils::GetTargetFileName("GetObjectToSameContentTest");
    auto fcontent = std::make_shared<std::fstream>(tmpFile, std::ios::out| std::ios::binary);
    GetObjectRequest gRequest(BucketName, key);
    gRequest.setResponseStreamFactory([=]() {return fcontent; });
    auto callable = client.GetObjectCallable(gRequest);

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    client.DisableRequest();
    auto outome = callable.get();
    EXPECT_EQ(outome.isSuccess(), false);
    if (!outome.isSuccess()) {
        rateLimiter->setRate(0);
        client.EnableRequest();
        callable = client.GetObjectCallable(gRequest);
        outome = callable.get();
        EXPECT_EQ(outome.isSuccess(), true);

        std::string oriMd5 = ComputeContentMD5(*content);
        std::string fileMd5 = TestUtils::GetFileMd5(tmpFile);
        EXPECT_EQ(oriMd5, fileMd5);
    }
    client.EnableRequest();
    //SetLogLevel(LogLevel::LogOff);
    //SetLogCallback(nullptr);
    fcontent->close();
    RemoveFile(tmpFile);
}

TEST_F(HttpClientTest, ResponseBodyToUserContentPasitiveTest)
{
    //don't write error response to user content
    std::string key = TestUtils::GetObjectKey("ResponseBodyToUserContentPasitiveTest");
    auto content = std::make_shared<std::stringstream>();
    GetObjectRequest gRequest(BucketName, key);
    gRequest.setResponseStreamFactory([=]() {return content; });
    auto outcome = Client->GetObject(gRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(content->str().empty(), true);
}

TEST_F(HttpClientTest, SetNetworkInterfaceTest)
{
    ClientConfiguration conf;
    conf.networkInterface = "eth100";
    OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);

    std::string key = TestUtils::GetObjectKey("UseInvalidNetworkInterfaceTest");
    auto content = std::make_shared<std::stringstream>();
    auto outcome = client.PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "ClientError:200045");

#ifdef _WIN32
    conf.networkInterface = "";
#else
    conf.networkInterface = "eth0";
#endif
    OssClient client1(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    auto content1 = std::make_shared<std::stringstream>();
    outcome = client1.PutObject(BucketName, key, content1);
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_TRUE(outcome.result().RequestId().size() > 0);
}

}
}