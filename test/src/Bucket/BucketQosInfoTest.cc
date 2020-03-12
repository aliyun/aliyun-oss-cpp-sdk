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
#include "../Config.h"
#include "../Utils.h"

namespace AlibabaCloud 
{
namespace OSS 
{
    class BucketQosInfoTest : public ::testing::Test {
    protected:
        BucketQosInfoTest()
        {
        }

        ~BucketQosInfoTest() override
        {
        }

        // Sets up the stuff shared by all tests in this test case.
        static void SetUpTestCase()
        {
            Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
            BucketName = TestUtils::GetBucketName("cpp-sdk-bucketqosinfo");
            Client->CreateBucket(CreateBucketRequest(BucketName));
        }

        // Tears down the stuff shared by all tests in this test case.
        static void TearDownTestCase()
        {
            Client->DeleteBucket(DeleteBucketRequest(BucketName));
            Client = nullptr;
        }

        // Sets up the test fixture.
        void SetUp() override
        {
        }

        // Tears down the test fixture.
        void TearDown() override
        {
        }
    public:
        static std::shared_ptr<OssClient> Client;
        static std::string BucketName;
    };

    std::shared_ptr<OssClient> BucketQosInfoTest::Client = nullptr;
    std::string BucketQosInfoTest::BucketName = "";

    TEST_F(BucketQosInfoTest, SetAndDeleteBucketQosInfoTest)
    {
        QosConfiguration qos;
        qos.setTotalUploadBandwidth(10);
        qos.setExtranetUploadBandwidth(-1);
        qos.setIntranetUploadBandwidth(-1);
        qos.setTotalDownloadBandwidth(10);
        qos.setExtranetDownloadBandwidth(-1);
        qos.setIntranetDownloadBandwidth(-1);
        qos.setTotalQps(1000);
        qos.setExtranetQps(-1);
        qos.setIntranetQps(-1);
        SetBucketQosInfoRequest setrequest(BucketName, qos);

        auto setoutcome = Client->SetBucketQosInfo(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), true);

        DeleteBucketQosInfoRequest delrequest(BucketName);
        auto deloutcome = Client->DeleteBucketQosInfo(delrequest);
        EXPECT_EQ(deloutcome.isSuccess(), true);
    }

    TEST_F(BucketQosInfoTest, GetBucketQosInfoTest)
    {
        QosConfiguration qos;
        qos.setTotalUploadBandwidth(10);
        qos.setExtranetUploadBandwidth(-1);
        qos.setIntranetUploadBandwidth(-1);
        qos.setTotalDownloadBandwidth(10);
        qos.setExtranetDownloadBandwidth(-1);
        qos.setIntranetDownloadBandwidth(-1);
        qos.setTotalQps(1000);
        qos.setExtranetQps(-1);
        qos.setIntranetQps(-1);
        SetBucketQosInfoRequest setrequest(BucketName, qos);

        auto setoutcome = Client->SetBucketQosInfo(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), true);

        GetBucketQosInfoRequest getrequest(BucketName);
        auto getoutcome = Client->GetBucketQosInfo(getrequest);
        EXPECT_EQ(getoutcome.isSuccess(), true);       
        EXPECT_EQ(getoutcome.result().QosInfo().TotalUploadBandwidth(), 10);
        EXPECT_EQ(getoutcome.result().QosInfo().ExtranetUploadBandwidth(), -1);
        EXPECT_EQ(getoutcome.result().QosInfo().IntranetUploadBandwidth(), -1);
        EXPECT_EQ(getoutcome.result().QosInfo().TotalDownloadBandwidth(), 10);
        EXPECT_EQ(getoutcome.result().QosInfo().ExtranetDownloadBandwidth(), -1);
        EXPECT_EQ(getoutcome.result().QosInfo().IntranetDownloadBandwidth(), -1);
        EXPECT_EQ(getoutcome.result().QosInfo().TotalQps(), 1000);
        EXPECT_EQ(getoutcome.result().QosInfo().IntranetQps(), -1);
        EXPECT_EQ(getoutcome.result().QosInfo().ExtranetQps(), -1);

        GetUserQosInfoRequest getrequest1;
        auto getoutcome1 = Client->GetUserQosInfo(getrequest1);
        EXPECT_EQ(getoutcome1.isSuccess(), true);

        DeleteBucketQosInfoRequest delrequest(BucketName);
        auto deloutcome = Client->DeleteBucketQosInfo(delrequest);
        EXPECT_EQ(deloutcome.isSuccess(), true);
    }

    TEST_F(BucketQosInfoTest, GetUserQosInfoTest)
    {
        QosConfiguration qos;
        qos.setTotalUploadBandwidth(10);
        qos.setExtranetUploadBandwidth(-1);
        qos.setIntranetUploadBandwidth(-1);
        qos.setTotalDownloadBandwidth(10);
        qos.setExtranetDownloadBandwidth(-1);
        qos.setIntranetDownloadBandwidth(-1);
        qos.setTotalQps(1000);
        qos.setExtranetQps(-1);
        qos.setIntranetQps(-1);
        SetBucketQosInfoRequest setrequest(BucketName, qos);

        auto setoutcome = Client->SetBucketQosInfo(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), true);

        GetUserQosInfoRequest getrequest;
        auto getoutcome = Client->GetUserQosInfo(getrequest);
        EXPECT_EQ(getoutcome.isSuccess(), true);
        EXPECT_EQ(getoutcome.result().QosInfo().TotalUploadBandwidth(), 10);
        EXPECT_EQ(getoutcome.result().QosInfo().ExtranetUploadBandwidth(), -1);
        EXPECT_EQ(getoutcome.result().QosInfo().IntranetUploadBandwidth(), -1);
        EXPECT_EQ(getoutcome.result().QosInfo().TotalDownloadBandwidth(), 10);
        EXPECT_EQ(getoutcome.result().QosInfo().ExtranetDownloadBandwidth(), -1);
        EXPECT_EQ(getoutcome.result().QosInfo().IntranetDownloadBandwidth(), -1);
        //EXPECT_EQ(getoutcome.result().QosInfo().TotalQps(), 1000);
        EXPECT_EQ(getoutcome.result().QosInfo().IntranetQps(), -1);
        EXPECT_EQ(getoutcome.result().QosInfo().ExtranetQps(), -1);

        DeleteBucketQosInfoRequest delrequest(BucketName);
        auto deloutcome = Client->DeleteBucketQosInfo(delrequest);
        EXPECT_EQ(deloutcome.isSuccess(), true);
    }

    TEST_F(BucketQosInfoTest, GetUserQosInfoWithInvalidResponseBodyTest)
    {
        QosConfiguration qos;
        qos.setTotalUploadBandwidth(10);
        qos.setExtranetUploadBandwidth(-1);
        qos.setIntranetUploadBandwidth(-1);
        qos.setTotalDownloadBandwidth(10);
        qos.setExtranetDownloadBandwidth(-1);
        qos.setIntranetDownloadBandwidth(-1);
        qos.setTotalQps(1000);
        qos.setExtranetQps(-1);
        qos.setIntranetQps(-1);
        SetBucketQosInfoRequest setrequest(BucketName, qos);
        Client->SetBucketQosInfo(setrequest);

        auto gbqiRequest = GetBucketQosInfoRequest(BucketName);
        gbqiRequest.setResponseStreamFactory([=]() {
            auto content = std::make_shared<std::stringstream>();
            content->write("invlid data", 11);
            return content;
        });
        auto gbqiOutcome = Client->GetBucketQosInfo(gbqiRequest);
        EXPECT_EQ(gbqiOutcome.isSuccess(), false);
        EXPECT_EQ(gbqiOutcome.error().Code(), "ParseXMLError");

        auto guqiRequest = GetUserQosInfoRequest();
        guqiRequest.setResponseStreamFactory([=]() {
            auto content = std::make_shared<std::stringstream>();
            content->write("invlid data", 11);
            return content;
        });
        auto guqiOutcome = Client->GetUserQosInfo(guqiRequest);
        EXPECT_EQ(guqiOutcome.isSuccess(), false);
        EXPECT_EQ(guqiOutcome.error().Code(), "ParseXMLError");
    }

    TEST_F(BucketQosInfoTest, GetBucketQosInfoResultTest)
    {
        std::string xml = R"(<?xml version="1.0" ?>
                        <QoSConfiguration>
                                <TotalUploadBandwidth>10</TotalUploadBandwidth>
                                <IntranetUploadBandwidth>-1</IntranetUploadBandwidth>
                                <ExtranetUploadBandwidth>-1</ExtranetUploadBandwidth>
                                <TotalDownloadBandwidth>10</TotalDownloadBandwidth>
                                <IntranetDownloadBandwidth>-1</IntranetDownloadBandwidth>
                                <ExtranetDownloadBandwidth>-1</ExtranetDownloadBandwidth>
                                <TotalQps>1000</TotalQps>
                                <IntranetQps>-1</IntranetQps>
                                <ExtranetQps>-1</ExtranetQps>
                        </QoSConfiguration>)";
        GetBucketQosInfoResult result(xml);
        EXPECT_EQ(result.QosInfo().TotalUploadBandwidth(), 10);
        EXPECT_EQ(result.QosInfo().ExtranetUploadBandwidth(), -1);
        EXPECT_EQ(result.QosInfo().IntranetUploadBandwidth(), -1);
        EXPECT_EQ(result.QosInfo().TotalDownloadBandwidth(), 10);
        EXPECT_EQ(result.QosInfo().ExtranetDownloadBandwidth(), -1);
        EXPECT_EQ(result.QosInfo().IntranetDownloadBandwidth(), -1);
        EXPECT_EQ(result.QosInfo().TotalQps(), 1000);
        EXPECT_EQ(result.QosInfo().IntranetQps(), -1);
        EXPECT_EQ(result.QosInfo().ExtranetQps(), -1);
    }

    TEST_F(BucketQosInfoTest, GetUserQosInfoResultTest)
    {
        std::string xml = R"(<?xml version="1.0" ?>
                        <QoSConfiguration>
                                <Region>oss-cn-hangzhou</Region>
                                <TotalUploadBandwidth>10</TotalUploadBandwidth>
                                <IntranetUploadBandwidth>-1</IntranetUploadBandwidth>
                                <ExtranetUploadBandwidth>-1</ExtranetUploadBandwidth>
                                <TotalDownloadBandwidth>10</TotalDownloadBandwidth>
                                <IntranetDownloadBandwidth>-1</IntranetDownloadBandwidth>
                                <ExtranetDownloadBandwidth>-1</ExtranetDownloadBandwidth>
                                <TotalQps>1000</TotalQps>
                                <IntranetQps>-1</IntranetQps>
                                <ExtranetQps>-1</ExtranetQps>
                        </QoSConfiguration>)";
        GetUserQosInfoResult result(xml);
        EXPECT_EQ(result.Region(), "oss-cn-hangzhou");
        EXPECT_EQ(result.QosInfo().TotalUploadBandwidth(), 10);
        EXPECT_EQ(result.QosInfo().ExtranetUploadBandwidth(), -1);
        EXPECT_EQ(result.QosInfo().IntranetUploadBandwidth(), -1);
        EXPECT_EQ(result.QosInfo().TotalDownloadBandwidth(), 10);
        EXPECT_EQ(result.QosInfo().ExtranetDownloadBandwidth(), -1);
        EXPECT_EQ(result.QosInfo().IntranetDownloadBandwidth(), -1);
        EXPECT_EQ(result.QosInfo().TotalQps(), 1000);
        EXPECT_EQ(result.QosInfo().IntranetQps(), -1);
        EXPECT_EQ(result.QosInfo().ExtranetQps(), -1);
    }

    TEST_F(BucketQosInfoTest, GetUserQosInfoResultBranchTest)
    {
        GetUserQosInfoResult result("test");

        std::string xml = R"(<?xml version="1.0" ?>
                        <QoS>
                                <Region>oss-cn-hangzhou</Region>
                                <TotalUploadBandwidth>10</TotalUploadBandwidth>
                                <IntranetUploadBandwidth>-1</IntranetUploadBandwidth>
                                <ExtranetUploadBandwidth>-1</ExtranetUploadBandwidth>
                                <TotalDownloadBandwidth>10</TotalDownloadBandwidth>
                                <IntranetDownloadBandwidth>-1</IntranetDownloadBandwidth>
                                <ExtranetDownloadBandwidth>-1</ExtranetDownloadBandwidth>
                                <TotalQps>1000</TotalQps>
                                <IntranetQps>-1</IntranetQps>
                                <ExtranetQps>-1</ExtranetQps>
                        </QoS>)";
        GetUserQosInfoResult result1(xml);

       xml = R"(<?xml version="1.0" ?>
                        <QoSConfiguration>
                        </QoSConfiguration>)";
        GetUserQosInfoResult result2(xml);

        xml = R"(<?xml version="1.0" ?>
                        <QoSConfiguration>
                                <Region></Region>
                                <TotalUploadBandwidth></TotalUploadBandwidth>
                                <IntranetUploadBandwidth></IntranetUploadBandwidth>
                                <ExtranetUploadBandwidth></ExtranetUploadBandwidth>
                                <TotalDownloadBandwidth></TotalDownloadBandwidth>
                                <IntranetDownloadBandwidth></IntranetDownloadBandwidth>
                                <ExtranetDownloadBandwidth></ExtranetDownloadBandwidth>
                                <TotalQps></TotalQps>
                                <IntranetQps></IntranetQps>
                                <ExtranetQps></ExtranetQps>
                        </QoSConfiguration>)";
        GetUserQosInfoResult result3(xml);

        xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
        GetUserQosInfoResult result10(xml);

        xml = R"(<?xml version="1.0" ?>
                        <QoSConfiguration>
                                <Region>oss-cn-hangzhou</Region>
                                <TotalUploadBandwidth>10</TotalUploadBandwidth>
                                <IntranetUploadBandwidth>-1</IntranetUploadBandwidth>
                                <ExtranetUploadBandwidth>-1</ExtranetUploadBandwidth>
                                <TotalDownloadBandwidth>10</TotalDownloadBandwidth>
                                <IntranetDownloadBandwidth>-1</IntranetDownloadBandwidth>
                                <ExtranetDownloadBandwidth>-1</ExtranetDownloadBandwidth>
                                <TotalQps>1000</TotalQps>
                                <IntranetQps>-1</IntranetQps>
                                <ExtranetQps>-1</ExtranetQps>
                        </QoSConfiguration>)";
        GetBucketQosInfoResult result4(xml);

        xml = R"(<?xml version="1.0" encoding="UTF-8"?>)";
        GetBucketQosInfoResult result5(xml);

        GetBucketQosInfoResult result6("test");

        xml = R"(<?xml version="1.0" ?>
                        <QoSConfiguration>
                        </QoSConfiguration>)";
        GetBucketQosInfoResult result7(xml);

        xml = R"(<?xml version="1.0" ?>
                        <QoS>
                        </QoS>)";
        GetBucketQosInfoResult result8(xml);

        xml = R"(<?xml version="1.0" ?>
                        <QoSConfiguration>
                                <Region></Region>
                                <TotalUploadBandwidth></TotalUploadBandwidth>
                                <IntranetUploadBandwidth></IntranetUploadBandwidth>
                                <ExtranetUploadBandwidth></ExtranetUploadBandwidth>
                                <TotalDownloadBandwidth></TotalDownloadBandwidth>
                                <IntranetDownloadBandwidth></IntranetDownloadBandwidth>
                                <ExtranetDownloadBandwidth></ExtranetDownloadBandwidth>
                                <TotalQps></TotalQps>
                                <IntranetQps></IntranetQps>
                                <ExtranetQps></ExtranetQps>
                        </QoSConfiguration>)";
        GetBucketQosInfoResult result9(xml);

    }

    TEST_F(BucketQosInfoTest, SetBucketQosInfoFailTest)
    {
        QosConfiguration qos;
        SetBucketQosInfoRequest setrequest("INVALIDNAME", qos);

        auto setoutcome = Client->SetBucketQosInfo(setrequest);
        EXPECT_EQ(setoutcome.isSuccess(), false);

        DeleteBucketQosInfoRequest delrequest("INVALIDNAME");
        auto deloutcome = Client->DeleteBucketQosInfo(delrequest);
        EXPECT_EQ(deloutcome.isSuccess(), false);

        GetBucketQosInfoRequest getrequest("INVALIDNAME");
        auto getoutcome = Client->GetBucketQosInfo(getrequest);
        EXPECT_EQ(getoutcome.isSuccess(), false);
    }

    TEST_F(BucketQosInfoTest, UserQosInfoFailTest)
    {
        auto invalidClient = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, "Invalid", ClientConfiguration());
        GetUserQosInfoRequest getrequest1;
        auto getoutcome1 = invalidClient->GetUserQosInfo(getrequest1);
        EXPECT_EQ(getoutcome1.isSuccess(), false);
    }
}
}