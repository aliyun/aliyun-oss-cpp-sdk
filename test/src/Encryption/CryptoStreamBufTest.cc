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
#include <src/encryption/CryptoStreamBuf.h>
#include <fstream>

namespace AlibabaCloud {
namespace OSS {

class CryptoStreamBufTest : public ::testing::Test {
protected:
    CryptoStreamBufTest()
    {
    }

    ~CryptoStreamBufTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase()
    {
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase()
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
public:

};

TEST_F(CryptoStreamBufTest, EncryptBufferTest)
{
    auto key = ByteBuffer(32);
    auto iv = ByteBuffer(16);
    memcpy((void *)key.data(), (void *)("12345678901234561234567890123456"), 32);
    memcpy((void *)iv.data(), (void *)("1234567890123456"), 16);

    auto cipher = SymmetricCipher::CreateAES256_CTRImpl();
    cipher->EncryptInit(key, iv);

    auto content = std::make_shared<std::stringstream>();
    *content << "11223344556677889900aabbccddeeffb";
    CryptoStreamBuf cryptoStream(*content, cipher, key, iv);

    unsigned char encoded[] = { 0xc2, 0xba, 0xef, 0x5f, 0x21, 0xa2, 0x55, 0x8b, 0x2d, 0x5a, 0x1a, 0xe2, 0xd5, 0x0f, 0xcf, 0x0b,
                       0xa3, 0x00, 0x36, 0xa8, 0xf5, 0x79, 0x03, 0xaa, 0x4c, 0xc5, 0x65, 0xbb, 0x67, 0x0a, 0x07, 0x14, 0x9d };

    const int test_size = sizeof(encoded) / sizeof(encoded[0]);
    char buff[256];
    //content->read(buff, 128);
    //EXPECT_EQ(33, content->gcount());

    //read n byte by step
    //int step = 1;
    for (auto step = 1; step < test_size; step++) {
        int size = 0;
        memset(buff, 0, sizeof(buff) / sizeof(buff[0]));
        for (int offset = 0; offset < test_size; offset += step) {
            content->read(buff + offset, step);
            size += static_cast<int>(content->gcount());
        }
        EXPECT_EQ(size, test_size);
        EXPECT_TRUE(TestUtils::IsByteBufferEQ((char *)encoded, buff, test_size));
        content->clear();
        content->seekg(0, content->beg);
    }

    //seek to x offset and read n byte
    for (int n = 1; n < test_size; n++) {
        for (int offset = 0; offset < test_size; offset++) {
            memset(buff, 1, sizeof(buff) / sizeof(buff[0]));
            content->clear();
            content->seekg(offset, content->beg);
            content->read(buff + offset, n);
            int size = static_cast<int>(content->gcount());
            EXPECT_TRUE(TestUtils::IsByteBufferEQ((char *)encoded + offset, buff + offset, size));
        }
    }
}

TEST_F(CryptoStreamBufTest, DecryptBufferTest)
{
    auto key = ByteBuffer(32);
    auto iv = ByteBuffer(16);
    memcpy((void *)key.data(), (void *)("12345678901234561234567890123456"), 32);
    memcpy((void *)iv.data(), (void *)("1234567890123456"), 16);

    auto cipher = SymmetricCipher::CreateAES256_CTRImpl();


    unsigned char encoded[] = { 0xc2, 0xba, 0xef, 0x5f, 0x21, 0xa2, 0x55, 0x8b, 0x2d, 0x5a, 0x1a, 0xe2, 0xd5, 0x0f, 0xcf, 0x0b,
                       0xa3, 0x00, 0x36, 0xa8, 0xf5, 0x79, 0x03, 0xaa, 0x4c, 0xc5, 0x65, 0xbb, 0x67, 0x0a, 0x07, 0x14, 0x9d };

    const int test_size = sizeof(encoded) / sizeof(encoded[0]);

    for (auto step = 1; step <= test_size; step++)
    {
        std::string out;
        auto content = std::make_shared<std::stringstream>();
        auto cryptoStreamBuff = std::make_shared< CryptoStreamBuf>(*content, cipher, key, iv);
        for (auto i = 0; i < test_size;) {
            auto writeCnt = std::min(step, test_size - i);
            content->write((const char*)encoded + i, writeCnt);
            i += writeCnt;
        }
        cryptoStreamBuff = nullptr;
        *content >> out;
        EXPECT_EQ(out, "11223344556677889900aabbccddeeffb");
    }

}

TEST_F(CryptoStreamBufTest, DecryptBufferWithSkipCntTest)
{
    auto key = ByteBuffer(32);
    auto iv = ByteBuffer(16);
    memcpy((void *)key.data(), (void *)("12345678901234561234567890123456"), 32);
    memcpy((void *)iv.data(), (void *)("1234567890123456"), 16);

    auto cipher = SymmetricCipher::CreateAES256_CTRImpl();

    unsigned char encoded[] = { 0xc2, 0xba, 0xef, 0x5f, 0x21, 0xa2, 0x55, 0x8b, 0x2d, 0x5a, 0x1a, 0xe2, 0xd5, 0x0f, 0xcf, 0x0b,
                       0xa3, 0x00, 0x36, 0xa8, 0xf5, 0x79, 0x03, 0xaa, 0x4c, 0xc5, 0x65, 0xbb, 0x67, 0x0a, 0x07, 0x14, 0x9d };

    const int test_size = sizeof(encoded) / sizeof(encoded[0]);

    std::string pattern = "11223344556677889900aabbccddeeffb";

    for (auto skip = 0; skip <= CryptoStreamBuf::BLK_SIZE; skip++)
    {
        for (auto step = 1; step <= test_size; step++)
        {
            std::string out;
            auto content = std::make_shared<std::stringstream>();
            auto cryptoStreamBuff = std::make_shared< CryptoStreamBuf>(*content, cipher, key, iv, skip);
            for (auto i = 0; i < test_size;) {
                auto writeCnt = std::min(step, test_size - i);
                content->write((const char*)(encoded) + i, writeCnt);
                i += writeCnt;
            }
            cryptoStreamBuff = nullptr;
            *content >> out;
            EXPECT_EQ(out, pattern.substr(skip));
        }
    }
}


TEST_F(CryptoStreamBufTest, CryptoStreamTest)
{
    auto cipher = SymmetricCipher::CreateAES256_CTRImpl();
    auto content = std::make_shared<std::fstream>("", std::ios_base::out | std::ios_base::in | std::ios_base::trunc | std::ios_base::binary);
    auto cryptoStream = std::make_shared<CryptoStreamBuf>(*content, cipher, ByteBuffer(32), ByteBuffer(16));
    cryptoStream = nullptr;
}

}
}