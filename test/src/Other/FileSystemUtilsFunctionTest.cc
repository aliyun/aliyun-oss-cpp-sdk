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
#include <alibabacloud/oss/Const.h>
#include <src/utils/Utils.h>
#include <src/utils/FileSystemUtils.h>
#include "../Config.h"
#include "../Utils.h"
#include <fstream>

namespace AlibabaCloud {
namespace OSS {

class FileSystemUtilsFunctionTest : public ::testing::Test {
protected:
    FileSystemUtilsFunctionTest()
    {
    }

    ~FileSystemUtilsFunctionTest() override
    {
    }

    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};


TEST_F(FileSystemUtilsFunctionTest, CreateRemoveDirectoryWithoutPathSeparatorTest)
{
    std::string testPath = TestUtils::GetExecutableDirectory();
    EXPECT_FALSE(testPath.empty());

    time_t t;
    testPath.push_back(PATH_DELIMITER);
    testPath.append(TestUtils::GetTargetFileName("CreateDirectoryTest"));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));

    EXPECT_TRUE(CreateDirectory(testPath));
    EXPECT_TRUE(GetPathLastModifyTime(testPath, t));

    EXPECT_TRUE(RemoveDirectory(testPath));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));
}

TEST_F(FileSystemUtilsFunctionTest, CreateRemoveDirectoryWithPathSeparatorTest)
{
    std::string testPath = TestUtils::GetExecutableDirectory();
    EXPECT_FALSE(testPath.empty());

    time_t t;
    testPath.push_back(PATH_DELIMITER);
    testPath.append(TestUtils::GetTargetFileName("CreateDirectoryTest"));
    testPath.push_back(PATH_DELIMITER);
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));

    EXPECT_TRUE(CreateDirectory(testPath));
    EXPECT_TRUE(GetPathLastModifyTime(testPath, t));

    EXPECT_TRUE(RemoveDirectory(testPath));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));
}

TEST_F(FileSystemUtilsFunctionTest, CreateRemoveDirectoryWithMultiSubTest)
{
    std::string testPath = TestUtils::GetExecutableDirectory();
    EXPECT_FALSE(testPath.empty());

    std::string upperFolder;
    time_t t;
    testPath.push_back(PATH_DELIMITER);
    testPath.append(TestUtils::GetTargetFileName("CreateDirectoryTest"));
    upperFolder = testPath;
    testPath.push_back(PATH_DELIMITER);
    testPath.append(TestUtils::GetTargetFileName("CreateDirectoryTest"));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));

    EXPECT_TRUE(CreateDirectory(testPath));
    EXPECT_TRUE(GetPathLastModifyTime(testPath, t));

    EXPECT_FALSE(RemoveDirectory(upperFolder));
    EXPECT_TRUE(GetPathLastModifyTime(upperFolder, t));

    EXPECT_TRUE(RemoveDirectory(testPath));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));

    EXPECT_TRUE(RemoveDirectory(upperFolder));
    EXPECT_FALSE(GetPathLastModifyTime(upperFolder, t));

}

#ifdef _WIN32
TEST_F(FileSystemUtilsFunctionTest, CreateDirectoryNegativeTest)
{
    std::string testPath = TestUtils::GetExecutableDirectory();
    EXPECT_FALSE(testPath.empty());

    std::string upperFolder;
    time_t t;
    testPath.push_back(PATH_DELIMITER);
    testPath.append(TestUtils::GetTargetFileName(":"));
    upperFolder = testPath;
    testPath.push_back(PATH_DELIMITER);
    testPath.append(TestUtils::GetTargetFileName("CreateDirectoryNegativeTest"));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));

    EXPECT_FALSE(CreateDirectory(testPath));
}
#endif

TEST_F(FileSystemUtilsFunctionTest, GetFolderLastModifyTimeTest)
{
    std::string testPath = TestUtils::GetExecutableDirectory();
    EXPECT_FALSE(testPath.empty());

    time_t t;
    testPath.push_back(PATH_DELIMITER);
    testPath.append(TestUtils::GetTargetFileName("CreateDirectoryTest"));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));

    EXPECT_TRUE(CreateDirectory(testPath));
    EXPECT_TRUE(GetPathLastModifyTime(testPath, t));

    time_t t1;
    testPath.push_back(PATH_DELIMITER);
    EXPECT_TRUE(GetPathLastModifyTime(testPath, t1));

    EXPECT_EQ(t, t1);

    EXPECT_TRUE(RemoveDirectory(testPath));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));
}

TEST_F(FileSystemUtilsFunctionTest, GetFileLastModifyTimeTest)
{
    std::string testPath = TestUtils::GetExecutableDirectory();
    EXPECT_FALSE(testPath.empty());

    time_t t;
    testPath.push_back(PATH_DELIMITER);
    testPath.append(TestUtils::GetTargetFileName("CreateDirectoryTest"));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));

    std::fstream fs(testPath, std::ios::out | std::ios::binary);
    fs << "just for test for" << __FUNCTION__ << std::endl;
    fs.close();

    EXPECT_TRUE(GetPathLastModifyTime(testPath, t));

    EXPECT_TRUE(RemoveFile(testPath));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));
}

TEST_F(FileSystemUtilsFunctionTest, RemoveFileTest)
{
    std::string testPath = TestUtils::GetExecutableDirectory();
    EXPECT_FALSE(testPath.empty());

    time_t t;
    testPath.push_back(PATH_DELIMITER);
    testPath.append(TestUtils::GetTargetFileName("CreateDirectoryTest"));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));

    std::fstream fs(testPath, std::ios::out | std::ios::binary);
    fs << "just for test for" << __FUNCTION__ <<std::endl;
    fs.close();

    EXPECT_TRUE(GetPathLastModifyTime(testPath, t));

    EXPECT_TRUE(RemoveFile(testPath));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));
}

TEST_F(FileSystemUtilsFunctionTest, RenameFileTest)
{
    std::string testPath = TestUtils::GetExecutableDirectory();
    EXPECT_FALSE(testPath.empty());

    time_t t;
    testPath.push_back(PATH_DELIMITER);
    testPath.append(TestUtils::GetTargetFileName("CreateDirectoryTest"));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));

    std::fstream fs(testPath, std::ios::out | std::ios::binary);
    fs << "just for test for" << __FUNCTION__ << std::endl;
    fs.close();

    EXPECT_TRUE(GetPathLastModifyTime(testPath, t));

    std::string newTestPath = testPath;
    newTestPath.append(".new");

    EXPECT_TRUE(RenameFile(testPath, newTestPath));
    EXPECT_TRUE(GetPathLastModifyTime(newTestPath, t));
    EXPECT_FALSE(GetPathLastModifyTime(testPath, t));


    EXPECT_TRUE(RemoveFile(newTestPath));
    EXPECT_FALSE(GetPathLastModifyTime(newTestPath, t));
}

TEST_F(FileSystemUtilsFunctionTest, GetFileLastModifyTimeEmptyPathTest)
{
	time_t t;
	std::string testPath = "";
	EXPECT_FALSE(GetPathLastModifyTime(testPath, t));
#if defined(_WIN32) && _MSC_VER < 1900
	testPath.clear();
	testPath.push_back(PATH_DELIMITER);
	EXPECT_FALSE(GetPathLastModifyTime(testPath, t));
#endif
}

}
}