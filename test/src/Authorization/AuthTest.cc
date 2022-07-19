#include <gtest/gtest.h>
#include <alibabacloud/oss/OssClient.h>
#include "../Config.h"
#include <alibabacloud/oss/OssRequest.h>
#include <fstream>
#include "src/auth/AuthSignerV4.h"

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

        static std::string BucketName = "bucket";
        static std::string ObjectName = "object";

        TEST_F(AuthTest, SignHeaderV1Test)
        {
            // sign header test
            std::string FileNametoSave = Config::GetDataPath() + "sign.txt";

            ClientConfiguration conf;
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
            ClientConfiguration conf;
            OssClient client(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, conf);

            std::time_t t = std::time(nullptr) + 1200;
            auto genOutcome = client.GeneratePresignedUrl(BucketName, ObjectName, t, Http::Get);

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

        TEST_F(AuthTest, SignV4CloudBoxTest)
        {
            // cloudbox 
            std::string bucket = "bucket";
            std::string object = "object";
            std::string region = "cloudBoxId";
            std::string product = "oss-cloudbox";
            std::time_t requestTime = 1658201018; // 20220719T032338Z

            SimpleCredentialsProvider provider("ak", "sk");
            Credentials credentials = provider.getCredentials();
            AuthSignerV4 signer(region, product);
            auto httpRequest = std::make_shared<HttpRequest>(Http::Get);
            ClientConfiguration conf;
            httpRequest->addHeader(Http::USER_AGENT, conf.userAgent);
            AuthSignerParam param(bucket, object, credentials, requestTime);
            signer.signRequest(*httpRequest.get(), param);

            std::string rightAuth = "OSS4-HMAC-SHA256 Credential=ak/20220719/cloudBoxId/oss-cloudbox/aliyun_v4_request,Signature=88db5f82cdc3115dc07a9c022942bf10931b5d2fe7191e015961c387a531382a";
            EXPECT_STREQ(httpRequest->Header(Http::AUTHORIZATION).c_str(), rightAuth.c_str());
        }

        TEST_F(AuthTest, SignV4ParamsTest)
        {
            // cloudbox 
            std::string bucket = "bucket";
            std::string object = "object";
            std::string region = "cloudBoxId";
            std::string product = "oss-cloudbox";
            std::time_t requestTime = 1658212981; // 20220719T064301Z

            SimpleCredentialsProvider provider("ak", "sk");
            Credentials credentials = provider.getCredentials();
            AuthSignerV4 signer(region, product);
            auto httpRequest = std::make_shared<HttpRequest>(Http::Get);
            ClientConfiguration conf;
            httpRequest->addHeader(Http::USER_AGENT, conf.userAgent);
            httpRequest->addHeader("x-oss-head1", "value");
            httpRequest->addHeader("abc", "value");
            httpRequest->addHeader("ZAbc", "value");
            httpRequest->addHeader("XYZ", "value");

            AuthSignerParam authParam(bucket, object, credentials, requestTime);
            ParameterCollection param;
            param["param1"] = "value1";
            param["|param1"] = "value2";
            param["+param1"] = "value3";
            param["|param1"] = "value4";
            param["+param2"] = "";
            param["|param2"] = "";
            param["param2"] = "";
            HeaderSet additionalHeaders;
            additionalHeaders.insert("ZAbc");
            additionalHeaders.insert("x-oss-head1");
            additionalHeaders.insert("abc");
            authParam.setAdditionalHeaders(additionalHeaders);
            authParam.setParameters(param);
            
            signer.signRequest(*httpRequest.get(), authParam);
            std::string rightAuth = "OSS4-HMAC-SHA256 Credential=ak/20220719/cloudBoxId/oss-cloudbox/aliyun_v4_request,AdditionalHeaders=abc;zabc,Signature=6829265e66cb6fb4488269dacf047bbdb1e6ea301a8343b6525c6bf39cf04b04";
            EXPECT_STREQ(httpRequest->Header(Http::AUTHORIZATION).c_str(), rightAuth.c_str());
        }
   }
}