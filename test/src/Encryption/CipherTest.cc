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
#include <src/utils/Utils.h>
#include "../Config.h"
#include "../Utils.h"
#include <alibabacloud/oss/encryption/Cipher.h>
#include <alibabacloud/oss/encryption/EncryptionMaterials.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

namespace AlibabaCloud {
namespace OSS {

class CipherTest : public ::testing::Test {
protected:
    CipherTest()
    {
    }

    ~CipherTest() override
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

TEST_F(CipherTest, ByteBufferTest)
{
    ByteBuffer buff = ByteBuffer(1024);
    for (auto i = 0U; i < buff.size(); i++) {
        buff[i] = static_cast<char>(i + 10U);
    }
    buff.resize(25U);
    EXPECT_EQ(buff.size(), 25U);

    for (auto i = 0U; i < buff.size(); i++) {
        EXPECT_EQ(buff[i], static_cast<char>(i + 10U));
    }
}

TEST_F(CipherTest, GenerateKeyTest)
{
    ByteBuffer key1 = SymmetricCipher::GenerateKey(32);
    ByteBuffer key2 = SymmetricCipher::GenerateKey(32);
    EXPECT_EQ(key1.size(), 32U);
    EXPECT_EQ(key2.size(), 32U);
    EXPECT_NE(key1, key2);

    key1 = key2;
    EXPECT_EQ(key1, key2);

    key1 = SymmetricCipher::GenerateKey(16);
    EXPECT_EQ(key1.size(), 16U);
    EXPECT_NE(key1, key2);
}

TEST_F(CipherTest, GenerateIVTest)
{
    ByteBuffer iv1 = SymmetricCipher::GenerateKey(32);
    ByteBuffer iv2 = SymmetricCipher::GenerateKey(32);
    EXPECT_EQ(iv1.size(), 32U);
    EXPECT_EQ(iv2.size(), 32U);
    EXPECT_NE(iv1, iv2);

    iv1 = iv2;
    EXPECT_EQ(iv1, iv2);

    iv1 = SymmetricCipher::GenerateKey(16);
    EXPECT_EQ(iv1.size(), 16U);
    EXPECT_NE(iv1, iv2);
}

TEST_F(CipherTest, AES128_CBCTest)
{
    auto cipher = SymmetricCipher::CreateAES128_CBCImpl();
    auto key = ByteBuffer(16);
    auto iv = ByteBuffer(16);
    auto data = ByteBuffer(16);
    memcpy((void *)key.data(), (void *)("1234567890123456"), 16);
    memcpy((void *)iv.data(), (void *)("1234567890123456"), 16);
    memcpy((void *)data.data(), (void *)("1234567890123456"), 16);

    cipher->EncryptInit(key, iv);
    auto out = cipher->Encrypt(data);
    auto res = cipher->EncryptFinish();
    out.insert(out.end(), res.begin(), res.end());
    auto encryptedOut = Base64Encode(out);
    EXPECT_EQ(encryptedOut, "2LWYSMdnDJSym1TSN54uesXryeud7lOPCtlpWV16dAw=");

    data = ByteBuffer(32);
    memcpy((void *)data.data(), (void *)("12345678901234561234567890123456"), 32);
    cipher->EncryptInit(key, iv);
    out = cipher->Encrypt(data);
    res = cipher->EncryptFinish();
    out.insert(out.end(), res.begin(), res.end());
    encryptedOut = Base64Encode(out);
    EXPECT_EQ(encryptedOut, "2LWYSMdnDJSym1TSN54uetDw/KrpkJ7D+YYFVxZjfqGV1OFBxxlBNPpKFFwBegWk");

    data = ByteBuffer(33);
    memcpy((void *)data.data(), (void *)("123456789012345612345678901234561"), 33);
    cipher->EncryptInit(key, iv);
    out = cipher->Encrypt(data);
    res = cipher->EncryptFinish();
    out.insert(out.end(), res.begin(), res.end());
    encryptedOut = Base64Encode(out);
    EXPECT_EQ(encryptedOut, "2LWYSMdnDJSym1TSN54uetDw/KrpkJ7D+YYFVxZjfqGzz+k3f0nDhB4D6GcOJxcy");
    
    memcpy((void *)key.data(), (void *)("abcdefghijklmnop"), 16);
    memcpy((void *)iv.data(), (void *)("ABCDEFGHIJKLMNOP"), 16);
    data = ByteBuffer(34);
    memcpy((void *)data.data(), (void *)("1234567890123456123456789012345612"), 34);
    cipher->EncryptInit(key, iv);
    out = cipher->Encrypt(data);
    res = cipher->EncryptFinish();
    out.insert(out.end(), res.begin(), res.end());
    encryptedOut = Base64Encode(out);
    EXPECT_EQ(encryptedOut, "Ck22xnBoI63Di96xD7wA4wKNM3JtJE9QLmRSYne6XaAABklNJi5Tv7U6zZW0eKWC");

    EXPECT_EQ(cipher->Name(), "AES/CBC/PKCS5Padding");
    EXPECT_EQ(cipher->Algorithm(), CipherAlgorithm::AES);
    EXPECT_EQ(cipher->Mode(), CipherMode::CBC);
    EXPECT_EQ(cipher->Padding(), CipherPadding::PKCS5Padding);
}


TEST_F(CipherTest, AES256_CTRTest)
{
    auto cipher = SymmetricCipher::CreateAES256_CTRImpl();
    auto key = ByteBuffer(32);
    auto iv = ByteBuffer(16);
    auto data = ByteBuffer(16);
    memcpy((void *)key.data(), (void *)("12345678901234561234567890123456"), 32);
    memcpy((void *)iv.data(), (void *)("1234567890123456"), 16);
    memcpy((void *)data.data(), (void *)("1234567890123456"), 16);

    //Encrypt
    cipher->EncryptInit(key, iv);
    auto out = cipher->Encrypt(data);
    auto encryptedOut = Base64Encode(out);
    EXPECT_EQ(encryptedOut, "wrnuWSenVochXx3m0QzCBQ==");

    cipher->EncryptInit(key, iv);
    out.resize(16);
    memset(out.data(), 0, out.size());
    auto ret = cipher->Encrypt(out.data(), data.size(), data.data(), data.size());
    EXPECT_EQ(ret, 16);
    encryptedOut = Base64Encode(out);
    EXPECT_EQ(encryptedOut, "wrnuWSenVochXx3m0QzCBQ==");

    //Decrypt
    cipher->DecryptInit(key, iv);
    auto reout = cipher->Decrypt(out);
    EXPECT_EQ(TestUtils::IsByteBufferEQ(reout, data), true);

    cipher->DecryptInit(key, iv);
    reout.resize(16);
    memset(reout.data(), 0, reout.size());
    ret = cipher->Decrypt(reout.data(), reout.size(), out.data(), out.size());
    EXPECT_EQ(ret, 16);
    EXPECT_EQ(TestUtils::IsByteBufferEQ(reout, data), true);


    //data is empty
    cipher->EncryptInit(key, iv);
    out = cipher->Encrypt(ByteBuffer());
    EXPECT_EQ(out.empty(), true);

    ret = cipher->Encrypt(nullptr, 0, out.data(), out.size());
    EXPECT_EQ(ret, -1);

    ret = cipher->Encrypt(out.data(), out.size(), nullptr, out.size());
    EXPECT_EQ(ret, -1);

    cipher->DecryptInit(key, iv);
    reout = cipher->Decrypt(ByteBuffer());
    EXPECT_EQ(reout.empty(), true);

    ret = cipher->Decrypt(nullptr, 0, out.data(), out.size());
    EXPECT_EQ(ret, -1);

    ret = cipher->Decrypt(reout.data(), reout.size(), nullptr, 0);
    EXPECT_EQ(ret, -1);
}

TEST_F(CipherTest, AES256_CTRCounterTest)
{
    auto cipher = SymmetricCipher::CreateAES256_CTRImpl();
    auto key = ByteBuffer(32);
    auto iv = ByteBuffer(16);
    auto data = ByteBuffer(32);
    auto data1 = ByteBuffer(16);
    memcpy((void *)key.data(), (void *)("12345678901234561234567890123456"), 32);
    memcpy((void *)iv.data(), (void *)("1234567890123456"), 16);
    memcpy((void *)data.data(), (void *)("12345678901234561234567890123456"), 32);
    memcpy((void *)data1.data(), (void *)("1234567890123456"), 16);

    cipher->EncryptInit(key, iv);
    auto out = cipher->Encrypt(data);

    auto iv1 = SymmetricCipher::IncCTRCounter(iv, 1);
    cipher->EncryptInit(key, iv1);
    auto out1 = cipher->Encrypt(data1);

    EXPECT_TRUE(TestUtils::IsByteBufferEQ(out.data() + 16, out1.data(), 16));
}


TEST_F(CipherTest, RSA_RSAPEMTest)
{
    auto cipher = AsymmetricCipher::CreateRSA_NONEImpl();

    const std::string publicKey =
        "-----BEGIN RSA PUBLIC KEY-----\n"
        "MIGJAoGBALpUiB+w+r3v2Fgw0SgMbWl8bnzUVc3t3YbA89H13lrw7v6RUbL8+HGl\n"
        "s5YGoqD4lObG/sCQyaWd0B/XzOhjlSc1b53nyZhms84MGJ6nF2NQP+1gjY1ByDMK\n"
        "zeyVFFFvl9prlr6XpuJQlY0F/W4pbjLsk8Px4Qix5AoJbShElUu1AgMBAAE=\n"
        "-----END RSA PUBLIC KEY-----";

    const std::string privateKey = 
        "-----BEGIN RSA PRIVATE KEY-----\n"
        "MIICXgIBAAKBgQC6VIgfsPq979hYMNEoDG1pfG581FXN7d2GwPPR9d5a8O7+kVGy\n"
        "/PhxpbOWBqKg+JTmxv7AkMmlndAf18zoY5UnNW+d58mYZrPODBiepxdjUD/tYI2N\n"
        "QcgzCs3slRRRb5faa5a+l6biUJWNBf1uKW4y7JPD8eEIseQKCW0oRJVLtQIDAQAB\n"
        "AoGBAJrzWRAhuSLipeMRFZ5cV1B1rdwZKBHMUYCSTTC5amPuIJGKf4p9XI4F4kZM\n"
        "1klO72TK72dsAIS9rCoO59QJnCpG4CvLYlJ37wA2UbhQ1rBH5dpBD/tv3CUyfdtI\n"
        "9CLUsZR3DGBWXYwGG0KGMYPExe5Hq3PUH9+QmuO+lXqJO4IBAkEA6iLee6oBzu6v\n"
        "90zrr4YA9NNr+JvtplpISOiL/XzsU6WmdXjzsFLSsZCeaJKsfdzijYEceXY7zUNa\n"
        "0/qQh2BKoQJBAMu61rQ5wKtql2oR4ePTSm00/iHoIfdFnBNU+b8uuPXlfwU80OwJ\n"
        "Gbs0xBHe+dt4uT53QLci4KgnNkHS5lu4XJUCQQCisCvrvcuX4B6BNf+mbPSJKcci\n"
        "biaJqr4DeyKatoz36mhpw+uAH2yrWRPZEeGtayg4rvf8Jf2TuTOJi9eVWYFBAkEA\n"
        "uIPzyS81TQsxL6QajpjjI52HPXZcrPOis++Wco0Cf9LnA/tczSpA38iefAETEq94\n"
        "NxcSycsQ5br97QfyEsgbMQJANTZ/HyMowmDPIC+n9ExdLSrf4JydARSfntFbPsy1\n"
        "4oC6ciKpRdtAtAtiU8s9eAUSWi7xoaPJzjAHWbmGSHHckg==\n"
        "-----END RSA PRIVATE KEY-----";

    cipher->setPublicKey(publicKey);
    cipher->setPrivateKey(privateKey);

    auto data = ByteBuffer(32);
    memcpy((void *)data.data(), (void *)("12345678901234561234567890123456"), 32);

    auto encoded = cipher->Encrypt(data);
    EXPECT_EQ(encoded.size(), 128U);

    auto decoded = cipher->Decrypt(encoded);
    EXPECT_EQ(decoded.size(), data.size());
    EXPECT_TRUE(TestUtils::IsByteBufferEQ(data.data(), decoded.data(), data.size()));
    EXPECT_EQ(cipher->Name(), "RSA/NONE/PKCS1Padding");
}

TEST_F(CipherTest, RSA_PEMTest)
{
    auto cipher = AsymmetricCipher::CreateRSA_NONEImpl();

    const std::string publicKey =
        "-----BEGIN PUBLIC KEY-----\n"
        "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC6VIgfsPq979hYMNEoDG1pfG58\n"
        "1FXN7d2GwPPR9d5a8O7+kVGy/PhxpbOWBqKg+JTmxv7AkMmlndAf18zoY5UnNW+d\n"
        "58mYZrPODBiepxdjUD/tYI2NQcgzCs3slRRRb5faa5a+l6biUJWNBf1uKW4y7JPD\n"
        "8eEIseQKCW0oRJVLtQIDAQAB\n"
        "-----END PUBLIC KEY-----";

    const std::string privateKey =
        "-----BEGIN PRIVATE KEY-----\n"
        "MIICeAIBADANBgkqhkiG9w0BAQEFAASCAmIwggJeAgEAAoGBALpUiB+w+r3v2Fgw\n"
        "0SgMbWl8bnzUVc3t3YbA89H13lrw7v6RUbL8+HGls5YGoqD4lObG/sCQyaWd0B/X\n"
        "zOhjlSc1b53nyZhms84MGJ6nF2NQP+1gjY1ByDMKzeyVFFFvl9prlr6XpuJQlY0F\n"
        "/W4pbjLsk8Px4Qix5AoJbShElUu1AgMBAAECgYEAmvNZECG5IuKl4xEVnlxXUHWt\n"
        "3BkoEcxRgJJNMLlqY+4gkYp/in1cjgXiRkzWSU7vZMrvZ2wAhL2sKg7n1AmcKkbg\n"
        "K8tiUnfvADZRuFDWsEfl2kEP+2/cJTJ920j0ItSxlHcMYFZdjAYbQoYxg8TF7ker\n"
        "c9Qf35Ca476Veok7ggECQQDqIt57qgHO7q/3TOuvhgD002v4m+2mWkhI6Iv9fOxT\n"
        "paZ1ePOwUtKxkJ5okqx93OKNgRx5djvNQ1rT+pCHYEqhAkEAy7rWtDnAq2qXahHh\n"
        "49NKbTT+Iegh90WcE1T5vy649eV/BTzQ7AkZuzTEEd7523i5PndAtyLgqCc2QdLm\n"
        "W7hclQJBAKKwK+u9y5fgHoE1/6Zs9IkpxyJuJomqvgN7Ipq2jPfqaGnD64AfbKtZ\n"
        "E9kR4a1rKDiu9/wl/ZO5M4mL15VZgUECQQC4g/PJLzVNCzEvpBqOmOMjnYc9dlys\n"
        "86Kz75ZyjQJ/0ucD+1zNKkDfyJ58ARMSr3g3FxLJyxDluv3tB/ISyBsxAkA1Nn8f\n"
        "IyjCYM8gL6f0TF0tKt/gnJ0BFJ+e0Vs+zLXigLpyIqlF20C0C2JTyz14BRJaLvGh\n"
        "o8nOMAdZuYZIcdyS\n"
        "-----END PRIVATE KEY-----";

    cipher->setPublicKey(publicKey);
    cipher->setPrivateKey(privateKey);

    auto data = ByteBuffer(32);
    memcpy((void *)data.data(), (void *)("12345678901234561234567890123456"), 32);

    auto encoded = cipher->Encrypt(data);
    EXPECT_EQ(encoded.size(), 128U);

    auto decoded = cipher->Decrypt(encoded);
    EXPECT_EQ(decoded.size(), data.size());
    EXPECT_TRUE(TestUtils::IsByteBufferEQ(data.data(), decoded.data(), data.size()));
    EXPECT_EQ(cipher->Name(), "RSA/NONE/PKCS1Padding");
}


TEST_F(CipherTest, RSA_DifferentPEMTest)
{
    const std::string publicKey1 =
        "-----BEGIN PUBLIC KEY-----\n"
        "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC6VIgfsPq979hYMNEoDG1pfG58\n"
        "1FXN7d2GwPPR9d5a8O7+kVGy/PhxpbOWBqKg+JTmxv7AkMmlndAf18zoY5UnNW+d\n"
        "58mYZrPODBiepxdjUD/tYI2NQcgzCs3slRRRb5faa5a+l6biUJWNBf1uKW4y7JPD\n"
        "8eEIseQKCW0oRJVLtQIDAQAB\n"
        "-----END PUBLIC KEY-----";

    const std::string privateKey1 =
        "-----BEGIN PRIVATE KEY-----\n"
        "MIICeAIBADANBgkqhkiG9w0BAQEFAASCAmIwggJeAgEAAoGBALpUiB+w+r3v2Fgw\n"
        "0SgMbWl8bnzUVc3t3YbA89H13lrw7v6RUbL8+HGls5YGoqD4lObG/sCQyaWd0B/X\n"
        "zOhjlSc1b53nyZhms84MGJ6nF2NQP+1gjY1ByDMKzeyVFFFvl9prlr6XpuJQlY0F\n"
        "/W4pbjLsk8Px4Qix5AoJbShElUu1AgMBAAECgYEAmvNZECG5IuKl4xEVnlxXUHWt\n"
        "3BkoEcxRgJJNMLlqY+4gkYp/in1cjgXiRkzWSU7vZMrvZ2wAhL2sKg7n1AmcKkbg\n"
        "K8tiUnfvADZRuFDWsEfl2kEP+2/cJTJ920j0ItSxlHcMYFZdjAYbQoYxg8TF7ker\n"
        "c9Qf35Ca476Veok7ggECQQDqIt57qgHO7q/3TOuvhgD002v4m+2mWkhI6Iv9fOxT\n"
        "paZ1ePOwUtKxkJ5okqx93OKNgRx5djvNQ1rT+pCHYEqhAkEAy7rWtDnAq2qXahHh\n"
        "49NKbTT+Iegh90WcE1T5vy649eV/BTzQ7AkZuzTEEd7523i5PndAtyLgqCc2QdLm\n"
        "W7hclQJBAKKwK+u9y5fgHoE1/6Zs9IkpxyJuJomqvgN7Ipq2jPfqaGnD64AfbKtZ\n"
        "E9kR4a1rKDiu9/wl/ZO5M4mL15VZgUECQQC4g/PJLzVNCzEvpBqOmOMjnYc9dlys\n"
        "86Kz75ZyjQJ/0ucD+1zNKkDfyJ58ARMSr3g3FxLJyxDluv3tB/ISyBsxAkA1Nn8f\n"
        "IyjCYM8gL6f0TF0tKt/gnJ0BFJ+e0Vs+zLXigLpyIqlF20C0C2JTyz14BRJaLvGh\n"
        "o8nOMAdZuYZIcdyS\n"
        "-----END PRIVATE KEY-----";

    const std::string publicKey2 =
        "-----BEGIN PUBLIC KEY-----\n"
        "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCokfiAVXXf5ImFzKDw+XO/UByW\n"
        "6mse2QsIgz3ZwBtMNu59fR5zttSx+8fB7vR4CN3bTztrP9A6bjoN0FFnhlQ3vNJC\n"
        "5MFO1PByrE/MNd5AAfSVba93I6sx8NSk5MzUCA4NJzAUqYOEWGtGBcom6kEF6MmR\n"
        "1EKib1Id8hpooY5xaQIDAQAB\n"
        "-----END PUBLIC KEY-----";

    const std::string privateKey2 =
        "-----BEGIN RSA PRIVATE KEY-----\n"
        "MIICWwIBAAKBgQCokfiAVXXf5ImFzKDw+XO/UByW6mse2QsIgz3ZwBtMNu59fR5z\n"
        "ttSx+8fB7vR4CN3bTztrP9A6bjoN0FFnhlQ3vNJC5MFO1PByrE/MNd5AAfSVba93\n"
        "I6sx8NSk5MzUCA4NJzAUqYOEWGtGBcom6kEF6MmR1EKib1Id8hpooY5xaQIDAQAB\n"
        "AoGAOPUZgkNeEMinrw31U3b2JS5sepG6oDG2CKpPu8OtdZMaAkzEfVTJiVoJpP2Y\n"
        "nPZiADhFW3e0ZAnak9BPsSsySRaSNmR465cG9tbqpXFKh9Rp/sCPo4Jq2n65yood\n"
        "JBrnGr6/xhYvNa14sQ6xjjfSgRNBSXD1XXNF4kALwgZyCAECQQDV7t4bTx9FbEs5\n"
        "36nAxPsPM6aACXaOkv6d9LXI7A0J8Zf42FeBV6RK0q7QG5iNNd1WJHSXIITUizVF\n"
        "6aX5NnvFAkEAybeXNOwUvYtkgxF4s28s6gn11c5HZw4/a8vZm2tXXK/QfTQrJVXp\n"
        "VwxmSr0FAajWAlcYN/fGkX1pWA041CKFVQJAG08ozzekeEpAuByTIOaEXgZr5MBQ\n"
        "gBbHpgZNBl8Lsw9CJSQI15wGfv6yDiLXsH8FyC9TKs+d5Tv4Cvquk0efOQJAd9OC\n"
        "lCKFs48hdyaiz9yEDsc57PdrvRFepVdj/gpGzD14mVerJbOiOF6aSV19ot27u4on\n"
        "Td/3aifYs0CveHzFPQJAWb4LCDwqLctfzziG7/S7Z74gyq5qZF4FUElOAZkz718E\n"
        "yZvADwuz/4aK0od0lX9c4Jp7Mo5vQ4TvdoBnPuGoyw==\n"
        "-----END RSA PRIVATE KEY-----";

    auto cipher1 = AsymmetricCipher::CreateRSA_NONEImpl();
    cipher1->setPublicKey(publicKey1);
    cipher1->setPrivateKey(privateKey1);

    auto cipher2 = AsymmetricCipher::CreateRSA_NONEImpl();
    cipher2->setPublicKey(publicKey2);
    cipher2->setPrivateKey(privateKey2);

    auto data = ByteBuffer(32);
    memcpy((void *)data.data(), (void *)("12345678901234561234567890123456"), 32);

    //key1
    auto encoded1 = cipher1->Encrypt(data);
    EXPECT_EQ(encoded1.size(), 128U);

    auto decoded1 = cipher1->Decrypt(encoded1);
    EXPECT_EQ(decoded1.size(), data.size());
    EXPECT_TRUE(TestUtils::IsByteBufferEQ(data, decoded1));

    //key2
    auto encoded2 = cipher2->Encrypt(data);
    EXPECT_EQ(encoded1.size(), 128U);

    auto decoded2 = cipher2->Decrypt(encoded2);
    EXPECT_EQ(decoded2.size(), data.size());
    EXPECT_TRUE(TestUtils::IsByteBufferEQ(data, decoded2));

    //decrypt encode1 by key2
    auto decoded1_2 = cipher2->Decrypt(encoded1);
    EXPECT_EQ(decoded1_2.size(), 0U);

    //decrypt encode2 by key1
    auto decoded2_1 = cipher1->Decrypt(encoded2);
    EXPECT_EQ(decoded2_1.size(), 0U);
}

TEST_F(CipherTest, SimpleRSAEncryptionMaterialsTest)
{
    const std::string publicKey1 =
        "-----BEGIN PUBLIC KEY-----\n"
        "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC6VIgfsPq979hYMNEoDG1pfG58\n"
        "1FXN7d2GwPPR9d5a8O7+kVGy/PhxpbOWBqKg+JTmxv7AkMmlndAf18zoY5UnNW+d\n"
        "58mYZrPODBiepxdjUD/tYI2NQcgzCs3slRRRb5faa5a+l6biUJWNBf1uKW4y7JPD\n"
        "8eEIseQKCW0oRJVLtQIDAQAB\n"
        "-----END PUBLIC KEY-----";

    const std::string privateKey1 =
        "-----BEGIN PRIVATE KEY-----\n"
        "MIICeAIBADANBgkqhkiG9w0BAQEFAASCAmIwggJeAgEAAoGBALpUiB+w+r3v2Fgw\n"
        "0SgMbWl8bnzUVc3t3YbA89H13lrw7v6RUbL8+HGls5YGoqD4lObG/sCQyaWd0B/X\n"
        "zOhjlSc1b53nyZhms84MGJ6nF2NQP+1gjY1ByDMKzeyVFFFvl9prlr6XpuJQlY0F\n"
        "/W4pbjLsk8Px4Qix5AoJbShElUu1AgMBAAECgYEAmvNZECG5IuKl4xEVnlxXUHWt\n"
        "3BkoEcxRgJJNMLlqY+4gkYp/in1cjgXiRkzWSU7vZMrvZ2wAhL2sKg7n1AmcKkbg\n"
        "K8tiUnfvADZRuFDWsEfl2kEP+2/cJTJ920j0ItSxlHcMYFZdjAYbQoYxg8TF7ker\n"
        "c9Qf35Ca476Veok7ggECQQDqIt57qgHO7q/3TOuvhgD002v4m+2mWkhI6Iv9fOxT\n"
        "paZ1ePOwUtKxkJ5okqx93OKNgRx5djvNQ1rT+pCHYEqhAkEAy7rWtDnAq2qXahHh\n"
        "49NKbTT+Iegh90WcE1T5vy649eV/BTzQ7AkZuzTEEd7523i5PndAtyLgqCc2QdLm\n"
        "W7hclQJBAKKwK+u9y5fgHoE1/6Zs9IkpxyJuJomqvgN7Ipq2jPfqaGnD64AfbKtZ\n"
        "E9kR4a1rKDiu9/wl/ZO5M4mL15VZgUECQQC4g/PJLzVNCzEvpBqOmOMjnYc9dlys\n"
        "86Kz75ZyjQJ/0ucD+1zNKkDfyJ58ARMSr3g3FxLJyxDluv3tB/ISyBsxAkA1Nn8f\n"
        "IyjCYM8gL6f0TF0tKt/gnJ0BFJ+e0Vs+zLXigLpyIqlF20C0C2JTyz14BRJaLvGh\n"
        "o8nOMAdZuYZIcdyS\n"
        "-----END PRIVATE KEY-----";

    const std::string publicKey2 =
        "-----BEGIN PUBLIC KEY-----\n"
        "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCokfiAVXXf5ImFzKDw+XO/UByW\n"
        "6mse2QsIgz3ZwBtMNu59fR5zttSx+8fB7vR4CN3bTztrP9A6bjoN0FFnhlQ3vNJC\n"
        "5MFO1PByrE/MNd5AAfSVba93I6sx8NSk5MzUCA4NJzAUqYOEWGtGBcom6kEF6MmR\n"
        "1EKib1Id8hpooY5xaQIDAQAB\n"
        "-----END PUBLIC KEY-----";

    const std::string privateKey2 =
        "-----BEGIN RSA PRIVATE KEY-----\n"
        "MIICWwIBAAKBgQCokfiAVXXf5ImFzKDw+XO/UByW6mse2QsIgz3ZwBtMNu59fR5z\n"
        "ttSx+8fB7vR4CN3bTztrP9A6bjoN0FFnhlQ3vNJC5MFO1PByrE/MNd5AAfSVba93\n"
        "I6sx8NSk5MzUCA4NJzAUqYOEWGtGBcom6kEF6MmR1EKib1Id8hpooY5xaQIDAQAB\n"
        "AoGAOPUZgkNeEMinrw31U3b2JS5sepG6oDG2CKpPu8OtdZMaAkzEfVTJiVoJpP2Y\n"
        "nPZiADhFW3e0ZAnak9BPsSsySRaSNmR465cG9tbqpXFKh9Rp/sCPo4Jq2n65yood\n"
        "JBrnGr6/xhYvNa14sQ6xjjfSgRNBSXD1XXNF4kALwgZyCAECQQDV7t4bTx9FbEs5\n"
        "36nAxPsPM6aACXaOkv6d9LXI7A0J8Zf42FeBV6RK0q7QG5iNNd1WJHSXIITUizVF\n"
        "6aX5NnvFAkEAybeXNOwUvYtkgxF4s28s6gn11c5HZw4/a8vZm2tXXK/QfTQrJVXp\n"
        "VwxmSr0FAajWAlcYN/fGkX1pWA041CKFVQJAG08ozzekeEpAuByTIOaEXgZr5MBQ\n"
        "gBbHpgZNBl8Lsw9CJSQI15wGfv6yDiLXsH8FyC9TKs+d5Tv4Cvquk0efOQJAd9OC\n"
        "lCKFs48hdyaiz9yEDsc57PdrvRFepVdj/gpGzD14mVerJbOiOF6aSV19ot27u4on\n"
        "Td/3aifYs0CveHzFPQJAWb4LCDwqLctfzziG7/S7Z74gyq5qZF4FUElOAZkz718E\n"
        "yZvADwuz/4aK0od0lX9c4Jp7Mo5vQ4TvdoBnPuGoyw==\n"
        "-----END RSA PRIVATE KEY-----";

    auto cipher = AsymmetricCipher::CreateRSA_NONEImpl();

    const ByteBuffer key = SymmetricCipher::GenerateKey(32);
    const ByteBuffer iv = SymmetricCipher::GenerateKey(16);
    std::map<std::string, std::string> description1;
    std::map<std::string, std::string> description2;
    std::map<std::string, std::string> description3;
    description1["desc"] = "key1";
    description2["desc"] = "key2";

    cipher->setPublicKey(publicKey1);
    const ByteBuffer encryptedKey1 = cipher->Encrypt(key);
    const ByteBuffer encryptedIV1  = cipher->Encrypt(iv);

    cipher->setPublicKey(publicKey2);
    const ByteBuffer encryptedKey2 = cipher->Encrypt(key);
    const ByteBuffer encryptedIV2 = cipher->Encrypt(iv);

    EXPECT_TRUE(!encryptedKey1.empty());
    EXPECT_TRUE(!encryptedIV1.empty());

    EXPECT_EQ(encryptedKey1.size(), encryptedKey2.size());
    EXPECT_EQ(encryptedIV1.size(), encryptedIV2.size());

    SimpleRSAEncryptionMaterials materials(publicKey1, privateKey1, description1);
    materials.addEncryptionMaterial(publicKey2, privateKey2, description2);

    int ret;

    //EncryptCEK
    ContentCryptoMaterial content;
    content.setCipherName("AES/CTR/NoPadding");
    content.setContentKey(key);
    content.setContentIV(iv);
    ret = materials.EncryptCEK(content);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(description1, content.Description());

    //DecryptCEK
    content.setKeyWrapAlgorithm("RSA/NONE/PKCS1Padding");
    content.setContentKey(ByteBuffer());
    content.setContentIV(ByteBuffer());
    content.setEncryptedContentKey(encryptedKey2);
    content.setEncryptedContentIV(encryptedIV2);
    content.setDescription(description2);
    ret = materials.DecryptCEK(content);
    EXPECT_EQ(0, ret);
    EXPECT_TRUE(TestUtils::IsByteBufferEQ(key.data(), content.ContentKey().data(), key.size()));
    EXPECT_TRUE(TestUtils::IsByteBufferEQ(iv.data(), content.ContentIV().data(), iv.size()));

    content.setContentKey(ByteBuffer());
    content.setContentIV(ByteBuffer());
    content.setEncryptedContentKey(encryptedKey1);
    content.setEncryptedContentIV(encryptedIV1);
    content.setDescription(description1);
    ret = materials.DecryptCEK(content);
    EXPECT_EQ(0, ret);
    EXPECT_TRUE(TestUtils::IsByteBufferEQ(key.data(), content.ContentKey().data(), key.size()));
    EXPECT_TRUE(TestUtils::IsByteBufferEQ(iv.data(), content.ContentIV().data(), iv.size()));

    //EncryptCEK fail with no key or no iv
    content.setCipherName("AES/CTR/NoPadding");
    content.setContentKey(key);
    content.setContentIV(ByteBuffer());
    ret = materials.EncryptCEK(content);
    EXPECT_TRUE(ret != 0);

    content.setContentKey(ByteBuffer());
    content.setContentIV(iv);
    ret = materials.EncryptCEK(content);
    EXPECT_TRUE(ret != 0);

    content.setContentKey(ByteBuffer());
    content.setContentIV(ByteBuffer());
    ret = materials.EncryptCEK(content);
    EXPECT_TRUE(ret != 0);

    //DecryptCEK fail with no key or no iv
    content.setKeyWrapAlgorithm("RSA/NONE/PKCS1Padding");
    content.setEncryptedContentKey(ByteBuffer());
    content.setEncryptedContentIV(ByteBuffer());
    content.setDescription(description1);
    ret = materials.DecryptCEK(content);
    EXPECT_TRUE(ret != 0);

    content.setEncryptedContentKey(encryptedKey1);
    content.setEncryptedContentIV(ByteBuffer());
    content.setDescription(description1);
    ret = materials.DecryptCEK(content);
    EXPECT_TRUE(ret != 0);

    content.setEncryptedContentKey(ByteBuffer());
    content.setEncryptedContentIV(encryptedIV1);
    content.setDescription(description1);
    ret = materials.DecryptCEK(content);
    EXPECT_TRUE(ret != 0);

    //DecryptCEK fail with no desc
    content.setEncryptedContentKey(encryptedKey1);
    content.setEncryptedContentIV(encryptedIV1);
    content.setDescription(description3);
    ret = materials.DecryptCEK(content);
    EXPECT_TRUE(ret != 0);

    //DecryptCEK fail with non-match algo
    content.setKeyWrapAlgorithm("None");
    content.setEncryptedContentKey(encryptedKey1);
    content.setEncryptedContentIV(encryptedIV1);
    content.setDescription(description1);
    ret = materials.DecryptCEK(content);
    EXPECT_TRUE(ret != 0);
    
    //key invalid test
    SimpleRSAEncryptionMaterials invalidMaterials("invalid", "invalid");
    content.setCipherName("AES/CTR/NoPadding");
    content.setContentKey(key);
    content.setContentIV(iv);
    ret = invalidMaterials.EncryptCEK(content);
    EXPECT_TRUE(ret != 0);

    content.setKeyWrapAlgorithm("RSA/NONE/PKCS1Padding");
    content.setEncryptedContentKey(encryptedKey2);
    content.setEncryptedContentIV(encryptedIV2);
    content.setDescription(description2);
    ret = invalidMaterials.DecryptCEK(content);
    EXPECT_TRUE(ret != 0);

}


}
}