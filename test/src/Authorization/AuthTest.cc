#include <gtest/gtest.h>
#include <alibabacloud/oss/OssClient.h>
#include "../Config.h"
#include "../../../sdk/src/auth/SignGeneratorV1.h"
#include <alibabacloud/oss/OssRequest.h>
#include <fstream>

namespace AlibabaCloud
{
    namespace OSS
    {

        class AuthTest : public ::testing::Test
        {
        protected:
            AuthTest()
            {
            }

            ~AuthTest() override
            {
            }

            void SetUp() override
            {
            }

            void TearDown() override
            {
            }
        };

        TEST_F(AuthTest, SignHeaderV1Test)
        {
            // sign header test
            std::string BucketName = "signv4";
            std::string ObjectName = "sign.txt";
            std::string FileNametoSave = Config::GetDataPath() + "sign.txt";

            ClientConfiguration conf;
            // conf.authVersion = "4.0";
            // conf.authAlgorithm = "HMAC-SHA256";
            OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
            GetObjectRequest request(BucketName, ObjectName);

            auto outcome = client.GetObject(request);

            if (outcome.isSuccess())
            {
                std::cout << "GetObjectToFile success" << outcome.result().Metadata().ContentLength() << std::endl;
            }
            else
            {
                std::cout << "GetObjectToFile fail"
                          << ",code:" << outcome.error().Code() << ",message:" << outcome.error().Message() << ",requestId:" << outcome.error().RequestId() << std::endl;
            }
            EXPECT_EQ(outcome.isSuccess(), true);
        }

        TEST_F(AuthTest, PresignV1Test)
        {
            std::string PutobjectUrlName = "sign.txt";
            std::string BucketName = "signv4";

            ClientConfiguration conf;
            // conf.authVersion = "1.0";
            // conf.authAlgorithm = "HMAC-SHA1";
            OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);

            std::time_t t = std::time(nullptr) + 1200;
            auto genOutcome = client.GeneratePresignedUrl(BucketName, PutobjectUrlName, t, Http::Get);

            EXPECT_EQ(genOutcome.isSuccess(), true);
            std::cout << "GeneratePresignedUrl success, Gen url:" << genOutcome.result().c_str() << std::endl;

            auto outcome = client.GetObjectByUrl(genOutcome.result());

            if (!outcome.isSuccess())
            {
                std::cerr << "GetObjectByUrl fail"
                          << ",code:" << outcome.error().Code() << ",message:" << outcome.error().Message() << ",requestId:" << outcome.error().RequestId() << std::endl;
            }

            EXPECT_EQ(outcome.isSuccess(), true);
        }

        TEST_F(AuthTest, SignV4OSSTest)
        {
            // no clound-box and addtional header
            std::string ObjectName = "sign.txt";
            std::string BucketName = "signv4";

            ClientConfiguration conf;
            conf.authVersion = "4.0";
            OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
            client.setRegion("cn-hangzhou");
            // client.setCloudBoxId("cloudboxtest");
            // client.setAdditionalHeaders(conf.additionalHeaders);

            GetObjectRequest request(BucketName, ObjectName);

            auto outcome = client.GetObject(request);

            if (outcome.isSuccess())
            {
                std::cout << "GetObjectToFile success" << outcome.result().Metadata().ContentLength() << std::endl;
            }
            else
            {
                std::cout << "GetObjectToFile fail"
                          << ",code:" << outcome.error().Code() << ",message:" << outcome.error().Message() << ",requestId:" << outcome.error().RequestId() << std::endl;
            }
            EXPECT_EQ(outcome.isSuccess(), false);
        }

        TEST_F(AuthTest, SignV4CloudBoxTest)
        {
            // no addtional header
            std::string ObjectName = "sign.txt";
            std::string BucketName = "signv4";

            ClientConfiguration conf;
            conf.authVersion = "4.0";
            OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
            client.setRegion("cn-hangzhou");
            client.setCloudBoxId("cloudboxtest");
            // client.setAdditionalHeaders(conf.additionalHeaders);

            GetObjectRequest request(BucketName, ObjectName);

            auto outcome = client.GetObject(request);

            if (outcome.isSuccess())
            {
                std::cout << "GetObjectToFile success" << outcome.result().Metadata().ContentLength() << std::endl;
            }
            else
            {
                std::cout << "GetObjectToFile fail"
                          << ",code:" << outcome.error().Code() << ",message:" << outcome.error().Message() << ",requestId:" << outcome.error().RequestId() << std::endl;
            }
            EXPECT_EQ(outcome.isSuccess(), false);
        }

        TEST_F(AuthTest, SignV4AdditionalTest)
        {
            std::string ObjectName = "sign.txt";
            std::string BucketName = "signv4";

            ClientConfiguration conf;
            conf.authVersion = "4.0";
            conf.additionalHeaders.emplace_back("host");
            OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);
            client.setRegion("cn-hangzhou");
            client.setCloudBoxId("cloudboxtest");
            client.setAdditionalHeaders(conf.additionalHeaders);

            GetObjectRequest request(BucketName, ObjectName);

            auto outcome = client.GetObject(request);

            if (outcome.isSuccess())
            {
                std::cout << "GetObjectToFile success" << outcome.result().Metadata().ContentLength() << std::endl;
            }
            else
            {
                std::cout << "GetObjectToFile fail"
                          << ",code:" << outcome.error().Code() << ",message:" << outcome.error().Message() << ",requestId:" << outcome.error().RequestId() << std::endl;
            }
            EXPECT_EQ(outcome.isSuccess(), false);
        }
    }
}