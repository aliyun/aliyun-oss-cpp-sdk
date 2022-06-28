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
#include "../Config.h"
#include "../Utils.h"
#include <chrono>
#include <future>
#include <fstream>


namespace AlibabaCloud {
namespace OSS {

class RateLimiterTest : public ::testing::Test {
protected:
    RateLimiterTest()
    {
    }

    ~RateLimiterTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-ratelimitertest");
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

std::shared_ptr<OssClient> RateLimiterTest::Client = nullptr;
std::string RateLimiterTest::BucketName = "";

class  DefaultRateLimiter: public RateLimiter
{
public:
    DefaultRateLimiter():rate_(0) {};
    ~DefaultRateLimiter() {};
    virtual void setRate(int rate) { rate_ = rate; };
    virtual int Rate() const { return rate_; };
private:
    int rate_;
};

TEST_F(RateLimiterTest, NoRateLimiterTest)
{
    ClientConfiguration conf;
    auto client = OssClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    auto content = TestUtils::GetRandomStream(1 * 1024 * 1024);
    auto key = TestUtils::GetObjectKey("NoRateLimiterTest");

    client.PutObject(BucketName, key, content);
    EXPECT_TRUE(client.DoesObjectExist(BucketName, key));

    auto outcome = client.GetObject(BucketName, key);
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(RateLimiterTest, DefaultRateLimiterTest)
{
    ClientConfiguration conf;
    auto rateLimiter = std::make_shared<DefaultRateLimiter>();
    conf.sendRateLimiter = rateLimiter;
    conf.recvRateLimiter = rateLimiter;
    auto client = OssClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    auto content = TestUtils::GetRandomStream(2 * 1024 * 1024);
    auto key = TestUtils::GetObjectKey("DefaultRateLimiterTest");
    Timer timer;

    //256k
    rateLimiter->setRate(256);

    timer.reset();
    client.PutObject(BucketName, key, content);
    auto diff_put = timer.elapsed();
    EXPECT_TRUE(client.DoesObjectExist(BucketName, key));

    timer.reset();
    auto outcome = client.GetObject(BucketName, key);
    auto diff_get = timer.elapsed();
    EXPECT_EQ(outcome.isSuccess(), true);

    EXPECT_NEAR(diff_put, diff_get, 1000LL);
}

TEST_F(RateLimiterTest, PutObjectRateLimiterTest)
{
    ClientConfiguration conf;
    auto sendRateLimiter = std::make_shared<DefaultRateLimiter>();
    conf.sendRateLimiter = sendRateLimiter;
    auto client = OssClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    auto content = TestUtils::GetRandomStream(5 * 1024 * 1024);
    auto key = TestUtils::GetObjectKey("PutObjectRateLimiterTest");
    Timer timer;

    timer.reset();
    auto outcome = client.PutObject(BucketName, key, content);
    auto diff_no_limit = timer.elapsed();
    EXPECT_EQ(outcome.isSuccess(),  true);

    sendRateLimiter->setRate(500);
    timer.reset();
    outcome = client.PutObject(BucketName, key, content);
    EXPECT_EQ(outcome.isSuccess(), true);
    auto diff_500k = timer.elapsed();
    EXPECT_NEAR(diff_500k, 10000LL, 2000LL);

    std::cout << "diff_no_limit:" << diff_no_limit << " ms, diff_500k:" << diff_500k << " ms" << std::endl;
}

TEST_F(RateLimiterTest, GetObjectRateLimiterTest)
{
    ClientConfiguration conf;
    auto recvRateLimiter = std::make_shared<DefaultRateLimiter>();
    conf.recvRateLimiter = recvRateLimiter;
    auto client = OssClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    auto content = TestUtils::GetRandomStream(10 * 1024 * 1024);
    auto key = TestUtils::GetObjectKey("GetObjectRateLimiterTest");
    Timer timer;

    client.PutObject(BucketName, key, content);
    EXPECT_TRUE(client.DoesObjectExist(BucketName, key));

    timer.reset();
    auto outcome = client.GetObject(BucketName, key);
    auto diff_no_limit = timer.elapsed();
    EXPECT_EQ(outcome.isSuccess(), true);

    timer.reset();
    recvRateLimiter->setRate(1024);
    outcome = client.GetObject(BucketName, key);
    auto diff_1024k = timer.elapsed();
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_NEAR(diff_1024k, 10000LL, 2000LL);
    std::cout << "diff_no_limit:" << diff_no_limit << " ms, diff_1024k:" << diff_1024k << " ms" << std::endl;
}

TEST_F(RateLimiterTest, SetRateWhenUploadingTest)
{
    ClientConfiguration conf;
    auto sendRateLimiter = std::make_shared<DefaultRateLimiter>();
    conf.sendRateLimiter = sendRateLimiter;
    auto client = OssClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    auto content = TestUtils::GetRandomStream(10 * 1024 * 1024);
    auto key = TestUtils::GetObjectKey("SetRateWhenUploadingTest");
    Timer timer;

    timer.reset();
    auto outcome = client.PutObject(BucketName, key, content);
    auto diff_no_limit = timer.elapsed();
    EXPECT_EQ(outcome.isSuccess(), true);

    sendRateLimiter->setRate(512);
    timer.reset();
    PutObjectRequest request(BucketName, key, content);
    auto callableOutcome = client.PutObjectCallable(request);
    TestUtils::WaitForCacheExpire(10);

    sendRateLimiter->setRate(1024);
    outcome = callableOutcome.get();
    EXPECT_EQ(outcome.isSuccess(), true);
    auto diff_limit = timer.elapsed();
    EXPECT_NEAR(diff_limit, 15000LL, 2500LL);

    sendRateLimiter->setRate(0);
    timer.reset();
    outcome = client.PutObject(BucketName, key, content);
    auto diff_no_limit1 = timer.elapsed();
    EXPECT_EQ(outcome.isSuccess(), true);
    EXPECT_NEAR(diff_no_limit, diff_no_limit1, 2000LL);

    std::cout << "diff_no_limit:"  << diff_no_limit << 
                " ms, diff_limit:" << diff_limit   << 
                " ms, diff_no_limit1:" << diff_no_limit1 << " ms" << std::endl;
}


TEST_F(RateLimiterTest, SetRateWhenDownloadingTest)
{
    ClientConfiguration conf;
    auto recvRateLimiter = std::make_shared<DefaultRateLimiter>();
    conf.recvRateLimiter = recvRateLimiter;
    auto client = OssClient(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
    auto content = TestUtils::GetRandomStream(10 * 1024 * 1024);
    auto key = TestUtils::GetObjectKey("SetRateWhenUploadingTest");
    Timer timer;

    client.PutObject(BucketName, key, content);
    EXPECT_TRUE(client.DoesObjectExist(BucketName, key));

    timer.reset();
    auto outcome = client.GetObject(BucketName, key);
    auto diff_no_limit = timer.elapsed();
    EXPECT_EQ(outcome.isSuccess(), true);

    recvRateLimiter->setRate(512);
    timer.reset();
    GetObjectRequest request(BucketName, key);
    auto callableOutcome = client.GetObjectCallable(request);
    TestUtils::WaitForCacheExpire(10);

    recvRateLimiter->setRate(256);
    outcome = callableOutcome.get();
    EXPECT_EQ(outcome.isSuccess(), true);
    auto diff_limit = timer.elapsed();
    EXPECT_NEAR(diff_limit, 30000LL, 2000LL);

    recvRateLimiter->setRate(0);
    timer.reset();
    outcome = client.GetObject(BucketName, key);
    auto diff_no_limit1 = timer.elapsed();
    EXPECT_EQ(outcome.isSuccess(), true);
    if (diff_no_limit1 < diff_no_limit) {
        EXPECT_TRUE(true);
    }
    else {
        EXPECT_NEAR(diff_no_limit, diff_no_limit1, 2000LL);
    }

    std::cout << "diff_no_limit:" << diff_no_limit <<
                 " ms, diff_limit:" << diff_limit <<
                 " ms, diff_no_limit1 " << diff_no_limit1 << " ms" << std::endl;
}

}
}