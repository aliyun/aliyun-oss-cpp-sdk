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

#include <stdlib.h>
#include <sstream>
#include <gtest/gtest.h>
#include <alibabacloud/oss/OssClient.h>
#include "../Config.h"
#include "../Utils.h"

namespace AlibabaCloud
{
namespace OSS
{
class SelectObjectTest : public ::testing::Test
{
protected:
    SelectObjectTest()
    {
    }
    ~SelectObjectTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
        ClientConfiguration conf;
        conf.enableCrc64 = false;
        Client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-selectobject");
        CreateBucketOutcome outCome = Client->CreateBucket(CreateBucketRequest(BucketName));
        EXPECT_EQ(outCome.isSuccess(), true);
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
        TestUtils::CleanBucket(*Client, BucketName);
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
    static std::string sqlMessage;
    static std::string jsonMessage;
};

std::shared_ptr<OssClient> SelectObjectTest::Client = nullptr;
std::string SelectObjectTest::BucketName = "";
std::string SelectObjectTest::sqlMessage = std::string("name,school,company,age\r\n")
    .append("Lora Francis,School A,Staples Inc,27\r\n")
    .append("Eleanor Little,School B,\"Conectiv, Inc\",43\r\n")
    .append("Rosie Hughes,School C,Western Gas Resources Inc,44\r\n")
    .append("Lawrence Ross,School D,MetLife Inc.,24");

std::string SelectObjectTest::jsonMessage = std::string("{\n")
    .append("\t\"name\": \"Lora Francis\",\n")
    .append("\t\"age\": 27,\n")
    .append("\t\"company\": \"Staples Inc\"\n")
    .append("}\n")
    .append("{\n")
    .append("\t\"name\": \"Eleanor Little\",\n")
    .append("\t\"age\": 43,\n")
    .append("\t\"company\": \"Conectiv, Inc\"\n")
    .append("}\n")
    .append("{\n")
    .append("\t\"name\": \"Rosie Hughes\",\n")
    .append("\t\"age\": 44,\n")
    .append("\t\"company\": \"Western Gas Resources Inc\"\n")
    .append("}\n")
    .append("{\n")
    .append("\t\"name\": \"Lawrence Ross\",\n")
    .append("\t\"age\": 24,\n")
    .append("\t\"company\": \"MetLife Inc.\"\n")
    .append("}");

TEST_F(SelectObjectTest, NormalSelectObjectWithCsvDataTest)
{
    std::string key = TestUtils::GetObjectKey("SqlObjectWithCsvData");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << sqlMessage;
    PutObjectRequest putRequest(BucketName, key, content);
    // put object
    auto putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // select object
    SelectObjectRequest selectRequest(BucketName, key);
    selectRequest.setExpression("select * from ossobject");

    CSVInputFormat csvInputFormat;
    csvInputFormat.setHeaderInfo(CSVHeader::Use);
    csvInputFormat.setRecordDelimiter("\r\n");
    csvInputFormat.setFieldDelimiter(",");
    csvInputFormat.setQuoteChar("\"");
    csvInputFormat.setCommentChar("#");
    selectRequest.setInputFormat(csvInputFormat);

    CSVOutputFormat csvOutputFormat;
    selectRequest.setOutputFormat(csvOutputFormat);

    auto outcome = Client->SelectObject(selectRequest);
    EXPECT_EQ(outcome.isSuccess(), true);

    // createSelectObjectMeta
    CreateSelectObjectMetaRequest metaRequest(BucketName, key);
    metaRequest.setInputFormat(csvInputFormat);
    auto metaOutcome = Client->CreateSelectObjectMeta(metaRequest);
    EXPECT_EQ(metaOutcome.isSuccess(), true);
}

TEST_F(SelectObjectTest, NormalSelectObjectWithOutputRawTest)
{
    // put object
    std::string key = TestUtils::GetObjectKey("SqlObjectWithoutOutputRaw");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << sqlMessage;
    PutObjectRequest putRequest(BucketName, key, content);
    auto putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // select object
    SelectObjectRequest selectRequest(BucketName, key);
    selectRequest.setExpression("select * from ossobject");
    
    CSVInputFormat inputCsv(CSVHeader::Use, "\r\n", ",", "\"", "#");
    selectRequest.setInputFormat(inputCsv);
    CSVOutputFormat outputCsv;
    outputCsv.setOutputRawData(true);
    outputCsv.setEnablePayloadCrc(false);
    selectRequest.setOutputFormat(outputCsv);
    
    auto outcome = Client->SelectObject(selectRequest);
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(SelectObjectTest, NormalSelectObjectWithKeepColumnsTest)
{
    // put object
    std::string key = TestUtils::GetObjectKey("SqlObjectWithKeepColums");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << sqlMessage;
    PutObjectRequest putRequest(BucketName, key, content);
    auto putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // select object
    SelectObjectRequest selectRequest(BucketName, key);
    selectRequest.setExpression("select * from ossobject");

    CSVInputFormat inputCsv(CSVHeader::Use, "\r\n", ",", "\"", "#");
    selectRequest.setInputFormat(inputCsv);

    CSVOutputFormat outputCsv;
    outputCsv.setKeepAllColumns(true);
    selectRequest.setOutputFormat(outputCsv);

    auto outcome = Client->SelectObject(selectRequest);
    EXPECT_EQ(outcome.isSuccess(), true);
}


TEST_F(SelectObjectTest, NormalSelectObjectWithOutputHeaderTest)
{
    // put object
    std::string key = TestUtils::GetObjectKey("SqlObjectWithOutputHeader");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << sqlMessage;
    PutObjectRequest putRequest(BucketName, key, content);
    auto putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // select object
    SelectObjectRequest selectRequest(BucketName, key);
    selectRequest.setExpression("select * from ossobject");

    CSVInputFormat inputCsv(CSVHeader::Use, "\r\n", ",", "\"", "#");
    selectRequest.setInputFormat(inputCsv);

    CSVOutputFormat outputCsv;
    outputCsv.setOutputHeader(true);
    selectRequest.setOutputFormat(outputCsv);

    auto outcome = Client->SelectObject(selectRequest);
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(SelectObjectTest, NormalSelectObjectWithSkipPartialDataTrueTest)
{
    // append message
    std::string errorSqlMessage(sqlMessage);
    errorSqlMessage.append("\r\n").append("1,,3\r\n").append("4,,6\r\n").append("7,8,9\r\n");

    // put object
    std::string key = TestUtils::GetObjectKey("SqlObjectWithSkipPartialData");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << errorSqlMessage;
    PutObjectRequest putRequest(BucketName, key, content);
    auto putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // select object
    SelectObjectRequest selectRequest(BucketName, key);
    selectRequest.setExpression("select _1,_2,_3,_4 from ossobject");

    CSVInputFormat inputCsv(CSVHeader::Use, "\r\n", ",", "\"", "#");
    selectRequest.setInputFormat(inputCsv);

    CSVOutputFormat outputCsv;
    selectRequest.setOutputFormat(outputCsv);

    selectRequest.setSkippedRecords(true, 1);
    auto outcome = Client->SelectObject(selectRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidCsvLine");

    selectRequest.setSkippedRecords(true, 5);
    auto retryOutcome = Client->SelectObject(selectRequest);
    EXPECT_EQ(retryOutcome.isSuccess(), true);
}

TEST_F(SelectObjectTest, NormalSelectObjectWithCrcCheckTest)
{
    // put object
    std::string key = TestUtils::GetObjectKey("SqlObjectWithCrcCheck");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << sqlMessage;
    PutObjectRequest putRequest(BucketName, key, content);
    auto putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // select object
    SelectObjectRequest selectRequest(BucketName, key);
    selectRequest.setExpression("select * from ossobject");

    CSVInputFormat inputCsv(CSVHeader::Use, "\r\n", ",", "\"", "#");
    selectRequest.setInputFormat(inputCsv);

    CSVOutputFormat outputCsv;
    outputCsv.setEnablePayloadCrc(true);
    selectRequest.setOutputFormat(outputCsv);

    auto outcome = Client->SelectObject(selectRequest);
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(SelectObjectTest, NormalSelectObjectWithGzipDataTest)
{
    //put object
    std::string key("sample_data.csv.gz");
    auto filePath = Config::GetDataPath();
    filePath.append("sample_data.csv.gz");
    auto putOutcome = Client->PutObject(BucketName, key, filePath);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // select object
    SelectObjectRequest selectRequest(BucketName, key);
    selectRequest.setExpression("select * from ossobject");

    CSVInputFormat inputCsv;
    inputCsv.setRecordDelimiter("\n");
    inputCsv.setHeaderInfo(CSVHeader::Use);
    inputCsv.setCompressionType(CompressionType::GZIP);
    selectRequest.setInputFormat(inputCsv);

    CSVOutputFormat outputCsv;
    selectRequest.setOutputFormat(outputCsv);

    auto outcome = Client->SelectObject(selectRequest);
    EXPECT_EQ(outcome.isSuccess(), true);
}

TEST_F(SelectObjectTest, NormalSelectObjectCreateMetaWithDelimitersTest)
{
    // put object
    std::string key = TestUtils::GetObjectKey("SelectObjectCreateMetaWithDelimiters");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << "abc,def123,456|7891334\n777,888|999,222012345\n\n";
    auto putOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // create meta
    CreateSelectObjectMetaRequest metaRequest(BucketName, key);
    CSVInputFormat csvInput;
    csvInput.setFieldDelimiter(",");
    csvInput.setRecordDelimiter("\n");
    metaRequest.setInputFormat(csvInput);
    auto metaOutcome = Client->CreateSelectObjectMeta(metaRequest);
    EXPECT_EQ(metaOutcome.isSuccess(), true);
    EXPECT_EQ(metaOutcome.result().SplitsCount(), 1U);
    EXPECT_EQ(metaOutcome.result().RowsCount(), 3U);
    EXPECT_EQ(metaOutcome.result().ColsCount(), 3U);

    // create meta without overwrite
    csvInput.setFieldDelimiter("|");
    csvInput.setRecordDelimiter("\n\n");
    metaRequest.setInputFormat(csvInput);
    auto withoutOutcome = Client->CreateSelectObjectMeta(metaRequest);
    EXPECT_EQ(withoutOutcome.isSuccess(), true);
    EXPECT_EQ(withoutOutcome.result().SplitsCount(), 1U);
    EXPECT_EQ(withoutOutcome.result().RowsCount(), 3U);
    EXPECT_EQ(withoutOutcome.result().ColsCount(), 3U);

    // create meta with overwrite
    metaRequest.setOverWriteIfExists(true);
    metaRequest.setInputFormat(csvInput);
    auto withOutcome = Client->CreateSelectObjectMeta(metaRequest);
    EXPECT_EQ(withOutcome.isSuccess(), true);
    EXPECT_EQ(withOutcome.result().SplitsCount(), 1U);
    EXPECT_EQ(withOutcome.result().RowsCount(), 1U);
    EXPECT_EQ(withOutcome.result().ColsCount(), 3U);
}


TEST_F(SelectObjectTest, NormalSelectObjectCreateMetaWithQuotecharacterTest)
{
    // put object
    std::string key = TestUtils::GetObjectKey("SelectObjectCreateMetaWithQuotecharacter");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << "'abc','def\n123','456'\n";
    auto putOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // create meta
    CreateSelectObjectMetaRequest metaRequest(BucketName, key);
    CSVInputFormat inputCsv;
    inputCsv.setQuoteChar("'");
    metaRequest.setInputFormat(inputCsv);
    auto metaOutcome = Client->CreateSelectObjectMeta(metaRequest);
    EXPECT_EQ(metaOutcome.isSuccess(), true);
    EXPECT_EQ(metaOutcome.result().SplitsCount(), 1U);
    EXPECT_EQ(metaOutcome.result().RowsCount(), 1U);
    EXPECT_EQ(metaOutcome.result().ColsCount(), 3U);

    // create new object
    auto anotherObjectOutcome = Client->PutObject(BucketName, key, content);
    EXPECT_EQ(anotherObjectOutcome.isSuccess(), true);
    inputCsv.setQuoteChar("\"");
    metaRequest.setInputFormat(inputCsv);
    auto anotherOutcome = Client->CreateSelectObjectMeta(metaRequest);
    EXPECT_EQ(anotherOutcome.isSuccess(), true);
    EXPECT_EQ(anotherOutcome.result().SplitsCount(), 1U);
    EXPECT_EQ(anotherOutcome.result().RowsCount(), 2U);
    EXPECT_EQ(anotherOutcome.result().ColsCount(), 2U);
}

TEST_F(SelectObjectTest, NormalSelectObjectInvalidTest)
{
    // put object
    std::string key = TestUtils::GetObjectKey("SelectObjectInvalid");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << "abc,def|123,456|7891334\n\n777,888|999,222|012345\n\n";

    // select object
    CSVInputFormat inputCSV;
    CSVOutputFormat outputCSV;
    SelectObjectRequest request(BucketName, key);
    request.setInputFormat(inputCSV);
    request.setOutputFormat(outputCSV);
    auto outcome = Client->SelectObject(request);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "InvalidSqlParameter");
}

TEST_F(SelectObjectTest, NormalValidateErrorTest)
{
    std::string key = TestUtils::GetObjectKey("ValidateErrorObject");
    {
        SelectObjectRequest request(BucketName, key);
        auto outcome = Client->SelectObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }
    {
        CreateSelectObjectMetaRequest request(BucketName, key);
        auto outcome = Client->CreateSelectObjectMeta(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }
    {
        SelectObjectRequest request(BucketName, key);
        CSVInputFormat inputCSV;
        JSONOutputFormat outputJSON;
        request.setInputFormat(inputCSV);
        request.setOutputFormat(outputJSON);
        auto outcome = Client->SelectObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }
    {
        SelectObjectRequest request(BucketName, key);
        CSVInputFormat inputCSV;
        inputCSV.setLineRange(3, 1);
        request.setInputFormat(inputCSV);
        CSVOutputFormat outputCSV;
        request.setOutputFormat(outputCSV);
        auto outcome = Client->SelectObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }
    {
        SelectObjectRequest request(BucketName, key);
        CSVInputFormat inputCSV;
        inputCSV.setSplitRange(3, 1);
        request.setInputFormat(inputCSV);
        CSVOutputFormat outputCSV;
        request.setOutputFormat(outputCSV);
        auto outcome = Client->SelectObject(request);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_EQ(outcome.error().Code(), "ValidateError");
    }
}

TEST_F(SelectObjectTest, NormalSelectObjectWithRangeSet)
{
    auto key = TestUtils::GetObjectKey("SqlObjectWithRangeSet");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << sqlMessage;
    PutObjectRequest putRequest(BucketName, key, content);
    auto putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    CSVInputFormat inputCSV;
    inputCSV.setHeaderInfo(CSVHeader::Use);
    inputCSV.setRecordDelimiter("\r\n");
    CSVOutputFormat outputCSV;

    // create select object meta
    CreateSelectObjectMetaRequest metaRequest(BucketName, key);
    metaRequest.setInputFormat(inputCSV);
    auto metaOutcome = Client->CreateSelectObjectMeta(metaRequest);
    EXPECT_EQ(metaOutcome.isSuccess(), true);

    // set line range
    SelectObjectRequest lineReauest(BucketName, key);
    lineReauest.setExpression("select * from ossobject");
    inputCSV.setLineRange(0, 3);
    lineReauest.setInputFormat(inputCSV);
    lineReauest.setOutputFormat(outputCSV);

    auto lineOutcome = Client->SelectObject(lineReauest);
    EXPECT_EQ(lineOutcome.isSuccess(), true);

    // set split range
    SelectObjectRequest splitRequest(BucketName, key);
    splitRequest.setExpression("select * from ossobject");
    inputCSV.setSplitRange(0, 3);
    splitRequest.setInputFormat(inputCSV);
    splitRequest.setOutputFormat(outputCSV);

    auto splitOutcome = Client->SelectObject(splitRequest);
    EXPECT_EQ(splitOutcome.isSuccess(), true);
}

TEST_F(SelectObjectTest, NormalSelectObjectWithJsonType)
{
    auto key = TestUtils::GetObjectKey("SqlObjectWithJsonType");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << jsonMessage;
    PutObjectRequest putRequest(BucketName, key, content);
    auto putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // select object
    SelectObjectRequest selectRequest(BucketName, key);
    selectRequest.setExpression("select * from ossobject");
    
    JSONInputFormat inputJson;
    inputJson.setJsonType(JsonType::LINES);
    inputJson.setCompressionType(CompressionType::NONE);
    JSONOutputFormat outputJson;
    outputJson.setEnablePayloadCrc(true);

    selectRequest.setInputFormat(inputJson);
    selectRequest.setOutputFormat(outputJson);
    auto outcome = Client->SelectObject(selectRequest);
    EXPECT_EQ(outcome.isSuccess(), true);

    // create select object meta
    CreateSelectObjectMetaRequest metaRequest(BucketName, key);
    metaRequest.setInputFormat(inputJson);
    auto metaOutcome = Client->CreateSelectObjectMeta(metaRequest);
    EXPECT_EQ(metaOutcome.isSuccess(), true);
    EXPECT_EQ(metaOutcome.result().SplitsCount(), 1U);
    EXPECT_EQ(metaOutcome.result().RowsCount(), 4U);
}

TEST_F(SelectObjectTest, SelectObjectWithPayloadChecksumFailTest) 
{
    std::string key = TestUtils::GetObjectKey("SqlObjectWithCsvData");
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    *content << sqlMessage;
    PutObjectRequest putRequest(BucketName, key, content);
    // put object
    auto putOutcome = Client->PutObject(putRequest);
    EXPECT_EQ(putOutcome.isSuccess(), true);

    // select object
    SelectObjectRequest selectRequest(BucketName, key);
    selectRequest.setExpression("select * from ossobject");

    CSVInputFormat inputCSV;
    inputCSV.setHeaderInfo(CSVHeader::Use);
    inputCSV.setRecordDelimiter("\r\n");
    inputCSV.setFieldDelimiter(",");
    inputCSV.setQuoteChar("\"");
    inputCSV.setCommentChar("#");
    selectRequest.setInputFormat(inputCSV);

    CSVOutputFormat outputCSV;
    selectRequest.setOutputFormat(outputCSV);

    selectRequest.setFlags(selectRequest.Flags() | (1U << 29));
    auto outcome = Client->SelectObject(selectRequest);
    EXPECT_EQ(outcome.isSuccess(), false);
    EXPECT_EQ(outcome.error().Code(), "SelectObjectError");
}

TEST_F(SelectObjectTest, CreateSelectObjectMetaWithPayloadChecksumFailTest) 
{
    unsigned char data[] = { 
        0x01, 0x80, 0x00, 0x06, 0x00, 0x00, 0x00, 0x25, 
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xc5, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xc5, 0x00, 0x00, 0x00, 0xc8,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x04,
        0x2e, 0x78, 0x95, 0x1f, 0x00};

    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    for (int i = 0; i < 53; i++) {
        *content << data[i];
    }
    CreateSelectObjectMetaResult result("BucketName", "ObjectName", "RequestId",content);
}

TEST_F(SelectObjectTest, CreateSelectObjectMetaWithParseIOStreamFailTest) 
{
    unsigned char data[] = {
        0x01, 0x80, 0x00, 0x06, 0x00, 0x00, 0x00, 0x25,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xc5, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xc5, 0x00, 0x00, 0x00, 0xc8,
        0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00};
    std::shared_ptr<std::iostream> content = std::make_shared<std::stringstream>();
    for (int i = 0; i < 40; i++) {
        *content << data[i];
    }
    CreateSelectObjectMetaResult result("BucketName", "ObjectName", "RequestId", content);
}

TEST_F(SelectObjectTest, InvalidObjectKeyTest)
{
    auto content = TestUtils::GetRandomStream(100);
    for (auto const& invalidKeyName : TestUtils::InvalidObjectKeyNamesList()) {
        // select object
        SelectObjectRequest selectRequest(BucketName, invalidKeyName);
        selectRequest.setExpression("select * from ossobject");

        CSVInputFormat csvInputFormat;
        csvInputFormat.setHeaderInfo(CSVHeader::Use);
        csvInputFormat.setRecordDelimiter("\r\n");
        csvInputFormat.setFieldDelimiter(",");
        csvInputFormat.setQuoteChar("\"");
        csvInputFormat.setCommentChar("#");
        selectRequest.setInputFormat(csvInputFormat);

        CSVOutputFormat csvOutputFormat;
        selectRequest.setOutputFormat(csvOutputFormat);

        auto outcome = Client->SelectObject(selectRequest);
        EXPECT_EQ(outcome.isSuccess(), false);
        EXPECT_STREQ(outcome.error().Code().c_str(), "ValidateError");
        break;
    }
}


}
}
