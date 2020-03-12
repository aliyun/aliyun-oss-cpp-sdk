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
#include "src/utils/FileSystemUtils.h"
#include <fstream>

namespace AlibabaCloud {
namespace OSS {

class ObjectVersioningTest : public ::testing::Test {
protected:
    ObjectVersioningTest()
    {
    }

    ~ObjectVersioningTest() override
    {
    }

    // Sets up the stuff shared by all tests in this test case.
    static void SetUpTestCase() 
    {
        std::string endpoint = "http://oss-ap-south-1.aliyuncs.com";
        Client = std::make_shared<OssClient>(endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
        BucketName = TestUtils::GetBucketName("cpp-sdk-objectversioning");
        Client->CreateBucket(CreateBucketRequest(BucketName));
    }

    // Tears down the stuff shared by all tests in this test case.
    static void TearDownTestCase() 
    {
        TestUtils::CleanBucketVersioning(*Client, BucketName);
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

std::shared_ptr<OssClient> ObjectVersioningTest::Client = nullptr;
std::string ObjectVersioningTest::BucketName = "";

TEST_F(ObjectVersioningTest, ObjectBasicWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //test put, get, head and getmeta 
    auto content1 = std::make_shared<std::stringstream>("versioning test 1.");
    auto content2 = std::make_shared<std::stringstream>("versioning test 2.");
    auto etag1 = ComputeContentETag(*content1);
    auto etag2 = ComputeContentETag(*content2);
    auto key = TestUtils::GetObjectKey("ObjectBasicWithVersioningEnableTest");

    auto pOutcome = Client->PutObject(BucketName, key, content1);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId1 = pOutcome.result().VersionId();

    pOutcome = Client->PutObject(BucketName, key, content2);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId2 = pOutcome.result().VersionId();

    EXPECT_NE(versionId1, versionId2);
    EXPECT_NE(etag1, etag2);

    //head 
    HeadObjectRequest request(BucketName, key);
    auto hOutcome = Client->HeadObject(request);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().VersionId(), versionId2);
    EXPECT_EQ(hOutcome.result().ETag(), etag2);

    request.setVersionId(versionId1);
    hOutcome = Client->HeadObject(request);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().VersionId(), versionId1);
    EXPECT_EQ(hOutcome.result().ETag(), etag1);

    request.setVersionId(versionId2);
    hOutcome = Client->HeadObject(request);
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().VersionId(), versionId2);
    EXPECT_EQ(hOutcome.result().ETag(), etag2);

    //Get
    GetObjectRequest gRequest(BucketName, key);
    auto gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), versionId2);
    EXPECT_EQ(gOutcome.result().Metadata().ETag(), etag2);

    gRequest.setVersionId(versionId1);
    gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), versionId1);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag1);

    gRequest.setVersionId(versionId2);
    gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), versionId2);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag2);

    auto lOutcome = Client->ListObjects(BucketName, key);
    EXPECT_EQ(lOutcome.isSuccess(), true);
    EXPECT_EQ(lOutcome.result().ObjectSummarys().size(), 1UL);

    ListObjectVersionsRequest lvRequest(BucketName);
    lvRequest.setPrefix(key);
    auto lvOutcome = Client->ListObjectVersions(lvRequest);
    EXPECT_EQ(lvOutcome.isSuccess(), true);
    EXPECT_EQ(lvOutcome.result().ObjectVersionSummarys().size(), 2UL);

    //Delete
    DeleteObjectRequest dRequest(BucketName, key);
    auto dOutcome = Client->DeleteObject(dRequest);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(dOutcome.result().VersionId().empty(), false);
    EXPECT_EQ(dOutcome.result().DeleteMarker(), true);
    auto dversionId = dOutcome.result().VersionId();

    //Get simple meta
    GetObjectMetaRequest gmRequest(BucketName, key);
    auto gmOutcome = Client->GetObjectMeta(gmRequest);
    EXPECT_EQ(gmOutcome.isSuccess(), false);
    EXPECT_EQ(gmOutcome.error().Code(), "ServerError:404");

    gmRequest.setVersionId(versionId1);
    gmOutcome = Client->GetObjectMeta(gmRequest);
    EXPECT_EQ(gmOutcome.isSuccess(), true);
    EXPECT_EQ(gmOutcome.result().VersionId(), versionId1);
    EXPECT_EQ(gmOutcome.result().ETag(), etag1);

    gmRequest.setVersionId(versionId2);
    gmOutcome = Client->GetObjectMeta(gmRequest);
    EXPECT_EQ(gmOutcome.isSuccess(), true);
    EXPECT_EQ(gmOutcome.result().VersionId(), versionId2);
    EXPECT_EQ(gmOutcome.result().ETag(), etag2);

    //list agian
    lOutcome = Client->ListObjects(BucketName, key);
    EXPECT_EQ(lOutcome.isSuccess(), true);
    EXPECT_EQ(lOutcome.result().ObjectSummarys().size(), 0UL);

    lvRequest.setBucket(BucketName);
    lvRequest.setPrefix(key);
    lvOutcome = Client->ListObjectVersions(lvRequest);
    EXPECT_EQ(lvOutcome.isSuccess(), true);
    EXPECT_EQ(lvOutcome.result().ObjectVersionSummarys().size(), 2UL);
    EXPECT_EQ(lvOutcome.result().DeleteMarkerSummarys().size(), 1UL);

    //Get agian
    gOutcome = Client->GetObject(GetObjectRequest(BucketName, key));
    EXPECT_EQ(gOutcome.isSuccess(), false);

    gRequest.setVersionId(versionId1);
    gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), versionId1);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag1);

    gRequest.setVersionId(versionId2);
    gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), versionId2);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag2);

    //delete by version id
    dRequest.setVersionId(dversionId);
    dOutcome = Client->DeleteObject(dRequest);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(dOutcome.result().VersionId(), dversionId);
    EXPECT_EQ(dOutcome.result().DeleteMarker(), true);

    gmOutcome = Client->GetObjectMeta(BucketName, key);
    EXPECT_EQ(gmOutcome.isSuccess(), true);
    EXPECT_EQ(gmOutcome.result().VersionId(), versionId2);

    dRequest.setVersionId(versionId1);
    EXPECT_EQ(dRequest.VersionId(), versionId1);
    dOutcome = Client->DeleteObject(dRequest);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().VersionId(), versionId1);
    EXPECT_EQ(dOutcome.result().DeleteMarker(), false);

    dRequest.setVersionId(versionId2);
    dOutcome = Client->DeleteObject(dRequest);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().VersionId(), versionId2);
    EXPECT_EQ(dOutcome.result().DeleteMarker(), false);

    //list again
    //lvRequest.setBucket(BucketName);
    //lvRequest.setPrefix(key);
    lvOutcome = Client->ListObjectVersions(BucketName, key);
    EXPECT_EQ(lvOutcome.isSuccess(), true);
    EXPECT_EQ(lvOutcome.result().ObjectVersionSummarys().size(), 0UL);
    EXPECT_EQ(lvOutcome.result().DeleteMarkerSummarys().size(), 0UL);
}

TEST_F(ObjectVersioningTest, ObjectAclWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //test date
    auto content1 = std::make_shared<std::stringstream>("versioning test 1.");
    auto content2 = std::make_shared<std::stringstream>("versioning test 2.");
    auto etag1 = ComputeContentETag(*content1);
    auto etag2 = ComputeContentETag(*content2);
    auto key = TestUtils::GetObjectKey("ObjectAclWithVersioningEnableTest");

    PutObjectRequest pRequest(BucketName, key, content1);
    pRequest.MetaData().addHeader("x-oss-object-acl","private");
    auto pOutcome = Client->PutObject(pRequest);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId1 = pOutcome.result().VersionId();

    pOutcome = Client->PutObject(BucketName, key, content2);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId2 = pOutcome.result().VersionId();

    GetObjectAclRequest gaRequest(BucketName, key);
    auto gaOutcome = Client->GetObjectAcl(gaRequest);
    EXPECT_EQ(gaOutcome.isSuccess(), true);
    EXPECT_EQ(gaOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(gaOutcome.result().VersionId(), versionId2);
    EXPECT_EQ(gaOutcome.result().Acl(), CannedAccessControlList::Default);

    gaRequest.setVersionId(versionId1);
    gaOutcome = Client->GetObjectAcl(gaRequest);
    EXPECT_EQ(gaOutcome.isSuccess(), true);
    EXPECT_EQ(gaOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(gaOutcome.result().VersionId(), versionId1);
    EXPECT_EQ(gaOutcome.result().Acl(), CannedAccessControlList::Private);

    gaRequest.setVersionId(versionId2);
    gaOutcome = Client->GetObjectAcl(gaRequest);
    EXPECT_EQ(gaOutcome.isSuccess(), true);
    EXPECT_EQ(gaOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(gaOutcome.result().VersionId(), versionId2);
    EXPECT_EQ(gaOutcome.result().Acl(), CannedAccessControlList::Default);

    //setAcl
    SetObjectAclRequest saRequest(BucketName, key);
    saRequest.setAcl(CannedAccessControlList::PublicRead);
    auto saOutcome = Client->SetObjectAcl(saRequest);
    EXPECT_EQ(saOutcome.isSuccess(), true);
    EXPECT_EQ(saOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(saOutcome.result().VersionId(), versionId2);

    gaOutcome = Client->GetObjectAcl(GetObjectAclRequest(BucketName, key));
    EXPECT_EQ(gaOutcome.isSuccess(), true);
    EXPECT_EQ(gaOutcome.result().Acl(), CannedAccessControlList::PublicRead);

    saRequest.setVersionId(versionId1);
    saRequest.setAcl(CannedAccessControlList::PublicReadWrite);
    saOutcome = Client->SetObjectAcl(saRequest);
    EXPECT_EQ(saOutcome.isSuccess(), true);
    EXPECT_EQ(saOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(saOutcome.result().VersionId(), versionId1);

    gaRequest.setVersionId(versionId1);
    gaOutcome = Client->GetObjectAcl(gaRequest);
    EXPECT_EQ(gaOutcome.isSuccess(), true);
    EXPECT_EQ(gaOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(gaOutcome.result().VersionId(), versionId1);
    EXPECT_EQ(gaOutcome.result().Acl(), CannedAccessControlList::PublicReadWrite);
}

TEST_F(ObjectVersioningTest, ObjectSymlinkWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //test date
    auto content1 = std::make_shared<std::stringstream>("versioning test 1.");
    auto content2 = std::make_shared<std::stringstream>("versioning test 2.");
    auto etag1 = ComputeContentETag(*content1);
    auto etag2 = ComputeContentETag(*content2);
    auto target1 = TestUtils::GetObjectKey("ObjectSymlinkWithVersioningEnableTest-1");
    auto target2 = TestUtils::GetObjectKey("ObjectSymlinkWithVersioningEnableTest-2");
    auto key = target1; key.append("-link");
    
    
    //1
    auto pOutcome = Client->PutObject(BucketName, target1, content1);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId1 = pOutcome.result().VersionId();

    CreateSymlinkRequest cRequest(BucketName, key);
    cRequest.SetSymlinkTarget(target1);
    auto cOutcome = Client->CreateSymlink(cRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);
    EXPECT_EQ(cOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(cOutcome.result().VersionId().empty(), false);
    EXPECT_EQ(cOutcome.result().ETag().empty(), false);
    auto sversionId1 = cOutcome.result().VersionId();

    //2
    pOutcome = Client->PutObject(BucketName, target2, content2);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId2 = pOutcome.result().VersionId();

    cRequest.SetSymlinkTarget(target2);
    cOutcome = Client->CreateSymlink(cRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);
    EXPECT_EQ(cOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(cOutcome.result().VersionId().empty(), false);
    auto sversionId2 = cOutcome.result().VersionId();

    EXPECT_NE(versionId1, versionId2);
    EXPECT_NE(sversionId1, sversionId2);

    //Get
    GetSymlinkRequest gsRequest(BucketName, key);
    auto gsOutcome = Client->GetSymlink(gsRequest);
    EXPECT_EQ(gsOutcome.isSuccess(), true);
    EXPECT_EQ(gsOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(gsOutcome.result().VersionId(), sversionId2);
    EXPECT_EQ(gsOutcome.result().SymlinkTarget(), target2);

    gsRequest.setVersionId(sversionId1);
    gsOutcome = Client->GetSymlink(gsRequest);
    EXPECT_EQ(gsOutcome.isSuccess(), true);
    EXPECT_EQ(gsOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(gsOutcome.result().VersionId(), sversionId1);
    EXPECT_EQ(gsOutcome.result().SymlinkTarget(), target1);

    gsRequest.setVersionId(sversionId2);
    gsOutcome = Client->GetSymlink(gsRequest);
    EXPECT_EQ(gsOutcome.isSuccess(), true);
    EXPECT_EQ(gsOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(gsOutcome.result().VersionId(), sversionId2);
    EXPECT_EQ(gsOutcome.result().SymlinkTarget(), target2);

    GetObjectRequest gRequest(BucketName, key);
    auto gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), sversionId2);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag2);

    gRequest.setVersionId(sversionId1);
    gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), sversionId1);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag1);

    gRequest.setVersionId(sversionId2);
    gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), sversionId2);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag2);
}

TEST_F(ObjectVersioningTest, AppendObjectWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //test date
    auto content = std::make_shared<std::stringstream>("versioning test.");
    auto crc1 = ComputeCRC64(0UL, (void *)"versioning test.", 16UL);
    auto crc2 = ComputeCRC64(0UL, (void *)"versioning test.versioning test.", 32UL);
    auto key = TestUtils::GetObjectKey("AppendObjectWithVersioningEnableTest");

    AppendObjectRequest aRequest(BucketName, key, content);
    aRequest.setPosition(0UL);
    auto aOutcome = Client->AppendObject(aRequest);
    EXPECT_EQ(aOutcome.isSuccess(), true);
    EXPECT_EQ(aOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(aOutcome.result().VersionId(), "null");
    EXPECT_EQ(aOutcome.result().Length(), 16UL);
    EXPECT_EQ(aOutcome.result().CRC64(), crc1);

    aRequest.setPosition(16UL);
    aOutcome = Client->AppendObject(aRequest);
    EXPECT_EQ(aOutcome.isSuccess(), true);
    EXPECT_EQ(aOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(aOutcome.result().VersionId(), "null");
    EXPECT_EQ(aOutcome.result().Length(), 32UL);
    EXPECT_EQ(aOutcome.result().CRC64(), crc2);
}

TEST_F(ObjectVersioningTest, RestoreObjectWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //test date
    auto content1 = std::make_shared<std::stringstream>("versioning test 1.");
    auto content2 = std::make_shared<std::stringstream>("versioning test 2.");
    auto etag1 = ComputeContentETag(*content1);
    auto etag2 = ComputeContentETag(*content2);
    auto key = TestUtils::GetObjectKey("RestoreObjectWithVersioningEnableTest");

    PutObjectRequest pRequest1(BucketName, key, content1);
    pRequest1.MetaData().addHeader("x-oss-storage-class", "Archive");
    auto pOutcome = Client->PutObject(pRequest1);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId1 = pOutcome.result().VersionId();

    PutObjectRequest pRequest2(BucketName, key, content2);
    pRequest2.MetaData().addHeader("x-oss-storage-class", "Archive");
    pOutcome = Client->PutObject(pRequest2);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId2 = pOutcome.result().VersionId();

    EXPECT_NE(versionId1, versionId2);
    EXPECT_NE(etag1, etag2);

    RestoreObjectRequest rRequest(BucketName, key);
    auto rOutcome = Client->RestoreObject(rRequest);
    EXPECT_EQ(rOutcome.isSuccess(), true);
    EXPECT_EQ(rOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(rOutcome.result().VersionId(), versionId2);

    rRequest.setVersionId(versionId1);
    rOutcome = Client->RestoreObject(rRequest);
    EXPECT_EQ(rOutcome.isSuccess(), true);
    EXPECT_EQ(rOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(rOutcome.result().VersionId(), versionId1);
}

TEST_F(ObjectVersioningTest, CopyObjectWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //test date
    auto content1 = std::make_shared<std::stringstream>("versioning test 1.");
    auto content2 = std::make_shared<std::stringstream>("versioning test 2.");
    auto etag1 = ComputeContentETag(*content1);
    auto etag2 = ComputeContentETag(*content2);
    auto key = TestUtils::GetObjectKey("CopyObjectWithVersioningEnableTest");

    auto pOutcome = Client->PutObject(BucketName, key, content1);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId1 = pOutcome.result().VersionId();

    pOutcome = Client->PutObject(BucketName, key, content2);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId2 = pOutcome.result().VersionId();

    EXPECT_NE(versionId1, versionId2);
    EXPECT_NE(etag1, etag2);

    CopyObjectRequest cRequest(BucketName, key);
    cRequest.setCopySource(BucketName, key);
    auto cOutcome = Client->CopyObject(cRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);
    EXPECT_EQ(cOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(cOutcome.result().SourceVersionId(), versionId2);
    EXPECT_EQ(cOutcome.result().VersionId().empty(), false);
    auto versionId3 = cOutcome.result().VersionId();

    cRequest.setCopySource(BucketName, key);
    cRequest.setVersionId(versionId1);
    cOutcome = Client->CopyObject(cRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);
    EXPECT_EQ(cOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(cOutcome.result().SourceVersionId(), versionId1);
    EXPECT_EQ(cOutcome.result().VersionId().empty(), false);
    auto versionId4 = cOutcome.result().VersionId();

    GetObjectRequest gRequest(BucketName, key);
    gRequest.setVersionId(versionId3);
    auto gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), versionId3);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag2);

    gRequest.setVersionId(versionId4);
    gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId(), versionId4);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag1);
}

TEST_F(ObjectVersioningTest, ObjectTaggingWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //test date
    auto content1 = std::make_shared<std::stringstream>("versioning test 1.");
    auto content2 = std::make_shared<std::stringstream>("versioning test 2.");
    auto etag1 = ComputeContentETag(*content1);
    auto etag2 = ComputeContentETag(*content2);
    auto key = TestUtils::GetObjectKey("ObjectTaggingWithVersioningEnableTest");
    Tagging tagging;
    tagging.addTag(Tag("key1", "value1"));
    tagging.addTag(Tag("key2", "value2"));

    PutObjectRequest pRequest(BucketName, key, content1);
    pRequest.setTagging(tagging.toQueryParameters());
    auto pOutcome = Client->PutObject(pRequest);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId1 = pOutcome.result().VersionId();

    pOutcome = Client->PutObject(BucketName, key, content2);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId2 = pOutcome.result().VersionId();

    //get
    GetObjectTaggingRequest gotRequest(BucketName, key);
    auto gtOutcome = Client->GetObjectTagging(gotRequest);
    EXPECT_EQ(gtOutcome.isSuccess(), true);
    EXPECT_EQ(gtOutcome.result().Tagging().Tags().size(), 0U);

    gotRequest.setVersionId(versionId1);
    gtOutcome = Client->GetObjectTagging(gotRequest);
    EXPECT_EQ(gtOutcome.isSuccess(), true);
    EXPECT_EQ(gtOutcome.result().Tagging().Tags().size(), tagging.Tags().size());

    //set
    Tagging tagging2;
    tagging2.addTag(Tag("key2-1", "value1"));
    tagging2.addTag(Tag("key2-2", "value2"));
    tagging2.addTag(Tag("key2-3", "value2"));
    SetObjectTaggingRequest sotRequst(BucketName, key);
    sotRequst.setVersionId(versionId1);
    sotRequst.setTagging(tagging2);
    auto sotOutcome = Client->SetObjectTagging(sotRequst);
    EXPECT_EQ(sotOutcome.isSuccess(), true);

    gotRequest.setVersionId(versionId2);
    gtOutcome = Client->GetObjectTagging(gotRequest);
    EXPECT_EQ(gtOutcome.isSuccess(), true);
    EXPECT_EQ(gtOutcome.result().Tagging().Tags().empty(), true);

    gotRequest.setVersionId(versionId1);
    gtOutcome = Client->GetObjectTagging(gotRequest);
    EXPECT_EQ(gtOutcome.isSuccess(), true);
    EXPECT_EQ(gtOutcome.result().Tagging().Tags().size(), tagging2.Tags().size());

    //delete
    DeleteObjectTaggingRequest dotRequst(BucketName, key);
    dotRequst.setVersionId(versionId1);
    auto dotOutcome = Client->DeleteObjectTagging(dotRequst);
    EXPECT_EQ(dotOutcome.isSuccess(), true);

    gotRequest.setVersionId(versionId1);
    gtOutcome = Client->GetObjectTagging(gotRequest);
    EXPECT_EQ(gtOutcome.isSuccess(), true);
    EXPECT_EQ(gtOutcome.result().Tagging().Tags().empty(), true);
}


TEST_F(ObjectVersioningTest, MultipartWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //test date
    auto content1 = std::make_shared<std::stringstream>("versioning test 1.");
    auto content2 = std::make_shared<std::stringstream>("versioning test 2.");
    auto etag1 = ComputeContentETag(*content1);
    auto etag2 = ComputeContentETag(*content2);
    auto key = TestUtils::GetObjectKey("MultipartWithVersioningEnableTest-basic");

    auto pOutcome = Client->PutObject(BucketName, key, content1);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId1 = pOutcome.result().VersionId();

    pOutcome = Client->PutObject(BucketName, key, content2);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId2 = pOutcome.result().VersionId();

    EXPECT_NE(versionId1, versionId2);
    EXPECT_NE(etag1, etag2);

    auto key1 = TestUtils::GetObjectKey("MultipartWithVersioningEnableTest-multi-1");
    auto initOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, key1));
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_EQ(initOutcome.result().RequestId().size(), 24UL);

    UploadPartCopyRequest request(BucketName, key1, BucketName, key);
    request.setVersionId(versionId1);
    request.setPartNumber(1);
    request.setUploadId(initOutcome.result().UploadId());
    auto uploadPartOutcome = Client->UploadPartCopy(request);
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
    EXPECT_EQ(uploadPartOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(uploadPartOutcome.result().VersionId().empty(), true);
    EXPECT_EQ(uploadPartOutcome.result().SourceVersionId(), versionId1);

    ListPartsRequest listRequest(BucketName, key1);
    listRequest.setUploadId(initOutcome.result().UploadId());
    auto listOutcome = Client->ListParts(listRequest);
    EXPECT_EQ(listOutcome.isSuccess(), true);

    CompleteMultipartUploadRequest completeRequest(BucketName, key1, listOutcome.result().PartList());
    completeRequest.setUploadId(initOutcome.result().UploadId());
    auto cOutcome = Client->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);
    EXPECT_EQ(cOutcome.result().VersionId().empty(), false);
    auto mversion1 = cOutcome.result().VersionId();

    auto key2 = TestUtils::GetObjectKey("MultipartWithVersioningEnableTest-multi-2");
    initOutcome = Client->InitiateMultipartUpload(InitiateMultipartUploadRequest(BucketName, key2));
    EXPECT_EQ(initOutcome.isSuccess(), true);
    EXPECT_EQ(initOutcome.result().RequestId().size(), 24UL);

    request.setKey(key2);
    request.setVersionId("");
    request.setPartNumber(1);
    request.setUploadId(initOutcome.result().UploadId());
    uploadPartOutcome = Client->UploadPartCopy(request);
    EXPECT_EQ(uploadPartOutcome.isSuccess(), true);
    EXPECT_EQ(uploadPartOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(uploadPartOutcome.result().VersionId().empty(), true);
    EXPECT_EQ(uploadPartOutcome.result().SourceVersionId(), versionId2);

    listRequest.setKey(key2);
    listRequest.setUploadId(initOutcome.result().UploadId());
    listOutcome = Client->ListParts(listRequest);
    EXPECT_EQ(listOutcome.isSuccess(), true);

    completeRequest.setKey(key2);
    completeRequest.setPartList(listOutcome.result().PartList());
    completeRequest.setUploadId(initOutcome.result().UploadId());
    cOutcome = Client->CompleteMultipartUpload(completeRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);
    EXPECT_EQ(cOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(cOutcome.result().VersionId().empty(), false);
    auto mversion2 = cOutcome.result().VersionId();
    
    auto gOutcome = Client->GetObject(BucketName, key1);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag1);

    gOutcome = Client->GetObject(BucketName, key2);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag2);
}

TEST_F(ObjectVersioningTest, DeleteObjecsWithVersioningEnableTest)
{
    std::string bucketName = BucketName;
    bucketName.append("-deleteobjects");

    auto cbOutcome = Client->CreateBucket(CreateBucketRequest(bucketName));
    EXPECT_EQ(cbOutcome.isSuccess(), true);

    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(bucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(bucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;


    //ObjectIdentifier
    ObjectIdentifier objectId;
    objectId.setKey("key");
    objectId.setVersionId("version");
    EXPECT_EQ(objectId.Key(), "key");
    EXPECT_EQ(objectId.VersionId(), "version");

    //DeletedObject
    DeletedObject deletedObject;
    EXPECT_EQ(deletedObject.DeleteMarker(), false);
    EXPECT_EQ(deletedObject.Key(), "");
    EXPECT_EQ(deletedObject.VersionId(), "");
    EXPECT_EQ(deletedObject.DeleteMarkerVersionId(), "");
    deletedObject.setDeleteMarker(true);
    deletedObject.setKey("key");
    deletedObject.setVersionId("version");
    deletedObject.setDeleteMarkerVersionId("marker");
    EXPECT_EQ(deletedObject.DeleteMarker(), true);
    EXPECT_EQ(deletedObject.Key(), "key");
    EXPECT_EQ(deletedObject.VersionId(), "version");
    EXPECT_EQ(deletedObject.DeleteMarkerVersionId(), "marker");


    auto keyPrefix = TestUtils::GetObjectKey("DeleteObjecsWithVersioningEnableTest");

    std::vector<std::string> keys;
    std::vector<std::string> versionIds;
    std::vector<std::string> deletedVersionIds;

    for (size_t i = 0; i < 10U; i++) {
        auto key = keyPrefix; key.append(std::to_string(i));
        auto pOutcome = Client->PutObject(bucketName, key, std::make_shared<std::stringstream>("just for test."));
        EXPECT_EQ(pOutcome.isSuccess(), true);
        keys.push_back(key);
        versionIds.push_back(pOutcome.result().VersionId());
    }

    DeleteObjectVersionsRequest dsRequest(bucketName);
    ObjectIdentifierList objectList0_6;
    for (size_t i = 0; i < 7U; i++) {
        objectList0_6.push_back(ObjectIdentifier(keys.at(i)));
    }
    dsRequest.setObjects(objectList0_6);
    dsRequest.addObject(ObjectIdentifier(keys.at(7)));
    dsRequest.addObject(ObjectIdentifier(keys.at(8)));
    dsRequest.addObject(ObjectIdentifier(keys.at(9)));

    auto dsOutcome = Client->DeleteObjectVersions(dsRequest);
    EXPECT_EQ(dsOutcome.isSuccess(), true);
    EXPECT_EQ(dsOutcome.result().DeletedObjects().size(), keys.size());

    size_t index = 0U;
    for (const auto& object: dsOutcome.result().DeletedObjects()) {
        EXPECT_EQ(object.Key(), keys.at(index));
        EXPECT_EQ(object.DeleteMarker(), true);
        EXPECT_EQ(object.DeleteMarkerVersionId().empty(), false);
        EXPECT_EQ(object.VersionId().empty(), true);
        index++;
        deletedVersionIds.push_back(object.DeleteMarkerVersionId());
    }

    auto lsOutcome = Client->ListObjects(bucketName);
    EXPECT_EQ(lsOutcome.isSuccess(), true);
    EXPECT_EQ(lsOutcome.result().ObjectSummarys().empty(), true);

    auto lsvOutcome = Client->ListObjectVersions(bucketName);
    EXPECT_EQ(lsvOutcome.isSuccess(), true);
    EXPECT_EQ(lsvOutcome.result().DeleteMarkerSummarys().size(), keys.size());
    EXPECT_EQ(lsvOutcome.result().ObjectVersionSummarys().size(), keys.size());

    //deletes objects with version id
    dsRequest.clearObjects();
    for (size_t i = 0; i < keys.size(); i++) {
        dsRequest.addObject(ObjectIdentifier(keys.at(i), versionIds.at(i)));
    }
    dsOutcome = Client->DeleteObjectVersions(dsRequest);
    EXPECT_EQ(dsOutcome.isSuccess(), true);
    EXPECT_EQ(dsOutcome.result().DeletedObjects().size(), keys.size());
    index = 0U;
    for (const auto& object : dsOutcome.result().DeletedObjects()) {
        EXPECT_EQ(object.Key(), keys.at(index));
        EXPECT_EQ(object.DeleteMarker(), false);
        EXPECT_EQ(object.DeleteMarkerVersionId().empty(), true);
        EXPECT_EQ(object.VersionId(), versionIds.at(index));
        index++;
    }

    lsvOutcome = Client->ListObjectVersions(ListObjectVersionsRequest(bucketName));
    EXPECT_EQ(lsvOutcome.isSuccess(), true);
    EXPECT_EQ(lsvOutcome.result().DeleteMarkerSummarys().size(), keys.size());
    EXPECT_EQ(lsvOutcome.result().ObjectVersionSummarys().size(), 0UL);

    //delete deleted marker with version id
    dsRequest.clearObjects();
    for (size_t i = 0; i < keys.size(); i++) {
        dsRequest.addObject(ObjectIdentifier(keys.at(i), deletedVersionIds.at(i)));
    }
    dsOutcome = Client->DeleteObjectVersions(dsRequest);
    EXPECT_EQ(dsOutcome.isSuccess(), true);
    EXPECT_EQ(dsOutcome.result().DeletedObjects().size(), keys.size());
    index = 0U;
    for (const auto& object : dsOutcome.result().DeletedObjects()) {
        EXPECT_EQ(object.Key(), keys.at(index));
        EXPECT_EQ(object.DeleteMarker(), true);
        EXPECT_EQ(object.DeleteMarkerVersionId(), deletedVersionIds.at(index));
        EXPECT_EQ(object.VersionId(), deletedVersionIds.at(index));
        index++;
    }

    lsvOutcome = Client->ListObjectVersions(ListObjectVersionsRequest(bucketName));
    EXPECT_EQ(lsvOutcome.isSuccess(), true);
    EXPECT_EQ(lsvOutcome.result().DeleteMarkerSummarys().size(), 0U);
    EXPECT_EQ(lsvOutcome.result().ObjectVersionSummarys().size(), 0U);

    auto dbOutcome = Client->DeleteBucket(bucketName);
    EXPECT_EQ(dbOutcome.isSuccess(), true);
}

TEST_F(ObjectVersioningTest, DeleteObjecsSpecialCharactersWithVersioningEnableTest)
{
    std::string bucketName = BucketName;
    bucketName.append("-deleteobjects-cs");

    auto cbOutcome = Client->CreateBucket(CreateBucketRequest(bucketName));
    EXPECT_EQ(cbOutcome.isSuccess(), true);

    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(bucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(bucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //escape char
    std::string keyPrefix = TestUtils::GetObjectKey("DeleteObjecsSpecialCharactersWithVersioningEnableTest-escape");
    char entities[] = { '\"', '&', '\'', '<', '>' };

    std::vector<std::string> keys;
    std::vector<std::string> versionIds;
    std::vector<std::string> deletedVersionIds;

    for (size_t i = 0; i < sizeof(entities) / sizeof(entities[0]); i++) {
        std::string key = keyPrefix;
        key.append("-").append(std::to_string(i));
        key.push_back(entities[i]);
        key.append(".dat");
        auto outcome = Client->PutObject(bucketName, key, std::make_shared<std::stringstream>("just for test."));
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(outcome.result().VersionId().empty(), false);
        keys.push_back(key);
        versionIds.push_back(outcome.result().VersionId());
    }
    {
        std::string key = keyPrefix;
        key.append("-").append(std::to_string(9));
        key.append("\"&\'<>-10.dat");
        auto outcome = Client->PutObject(bucketName, key, std::make_shared<std::stringstream>("just for test."));
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(outcome.result().VersionId().empty(), false);
        keys.push_back(key);
        versionIds.push_back(outcome.result().VersionId());
    }

    //
    DeleteObjectVersionsRequest dsRequest(bucketName);
    for (size_t i = 0; i < keys.size(); i++) {
        dsRequest.addObject(ObjectIdentifier(keys.at(i)));
    }
    EXPECT_EQ(dsRequest.EncodingType(), "");
    EXPECT_EQ(dsRequest.Quiet(), false);
    EXPECT_EQ(dsRequest.Objects().size(), keys.size());

    auto dsOutcome = Client->DeleteObjectVersions(dsRequest);
    EXPECT_EQ(dsOutcome.isSuccess(), true);
    EXPECT_EQ(dsOutcome.result().DeletedObjects().size(), keys.size());

    for (const auto& object : dsOutcome.result().DeletedObjects()) {
        deletedVersionIds.push_back(object.DeleteMarkerVersionId());
    }

    auto lsvOutcome = Client->ListObjectVersions(ListObjectVersionsRequest(bucketName));
    EXPECT_EQ(lsvOutcome.isSuccess(), true);
    EXPECT_EQ(lsvOutcome.result().DeleteMarkerSummarys().size(), keys.size());
    EXPECT_EQ(lsvOutcome.result().ObjectVersionSummarys().size(), keys.size());

    dsRequest.clearObjects();
    for (size_t i = 0; i < keys.size(); i++) {
        dsRequest.addObject(ObjectIdentifier(keys.at(i), versionIds.at(i)));
    }
    dsOutcome = Client->DeleteObjectVersions(dsRequest);
    EXPECT_EQ(dsOutcome.isSuccess(), true);
    EXPECT_EQ(dsOutcome.result().DeletedObjects().size(), keys.size());

    dsRequest.clearObjects();
    for (size_t i = 0; i < keys.size(); i++) {
        dsRequest.addObject(ObjectIdentifier(keys.at(i), deletedVersionIds.at(i)));
    }
    dsOutcome = Client->DeleteObjectVersions(dsRequest);
    EXPECT_EQ(dsOutcome.isSuccess(), true);
    EXPECT_EQ(dsOutcome.result().DeletedObjects().size(), keys.size());

    lsvOutcome = Client->ListObjectVersions(ListObjectVersionsRequest(bucketName));
    EXPECT_EQ(lsvOutcome.isSuccess(), true);
    EXPECT_EQ(lsvOutcome.result().DeleteMarkerSummarys().size(), 0U);
    EXPECT_EQ(lsvOutcome.result().ObjectVersionSummarys().size(), 0U);

    //url encode test

    keyPrefix = TestUtils::GetObjectKey("DeleteObjecsSpecialCharactersWithVersioningEnableTest-urlencode");
    std::string newKey;

    keys.clear();
    versionIds.clear();
    deletedVersionIds.clear();

    newKey = keyPrefix;
    newKey.append("-1-"); newKey.push_back(0x1c); newKey.push_back(0x1a); newKey.append(".dat");
    keys.push_back(newKey);

    newKey = keyPrefix;
    newKey.append("-2-"); newKey.push_back(0x1c); newKey.push_back(0x1a); newKey.append(".dat");
    keys.push_back(newKey);

    newKey = keyPrefix;
    newKey.append("-3-.dat");
    keys.push_back(newKey);

    for (const auto& key : keys) {
        auto outcome = Client->PutObject(bucketName, key, std::make_shared<std::stringstream>("just for test."));
        EXPECT_EQ(outcome.isSuccess(), true);
        EXPECT_EQ(outcome.result().VersionId().empty(), false);
        versionIds.push_back(outcome.result().VersionId());
    }

    dsRequest.setEncodingType("url");
    dsRequest.clearObjects();
    for (size_t i = 0; i < keys.size(); i++) {
        dsRequest.addObject(ObjectIdentifier(keys.at(i)));
    }

    dsOutcome = Client->DeleteObjectVersions(dsRequest);
    EXPECT_EQ(dsOutcome.isSuccess(), true);
    EXPECT_EQ(dsOutcome.result().DeletedObjects().size(), keys.size());
    for (size_t i = 0; i < keys.size(); i++) {
        EXPECT_EQ(dsOutcome.result().DeletedObjects()[i].Key(), keys[i]);
    }

    for (const auto& object : dsOutcome.result().DeletedObjects()) {
        deletedVersionIds.push_back(object.DeleteMarkerVersionId());
    }

    lsvOutcome = Client->ListObjectVersions(ListObjectVersionsRequest(bucketName));
    EXPECT_EQ(lsvOutcome.isSuccess(), true);
    EXPECT_EQ(lsvOutcome.result().DeleteMarkerSummarys().size(), keys.size());
    EXPECT_EQ(lsvOutcome.result().ObjectVersionSummarys().size(), keys.size());

    dsRequest.clearObjects();
    for (size_t i = 0; i < keys.size(); i++) {
        dsRequest.addObject(ObjectIdentifier(keys.at(i), versionIds.at(i)));
    }
    dsOutcome = Client->DeleteObjectVersions(dsRequest);
    EXPECT_EQ(dsOutcome.isSuccess(), true);
    EXPECT_EQ(dsOutcome.result().DeletedObjects().size(), keys.size());
    for (size_t i = 0; i < keys.size(); i++) {
        EXPECT_EQ(dsOutcome.result().DeletedObjects()[i].Key(), keys[i]);
    }

    ObjectIdentifierList objectList;
    for (size_t i = 0; i < keys.size(); i++) {
        objectList.push_back(ObjectIdentifier(keys.at(i), deletedVersionIds.at(i)));
    }
    dsOutcome = Client->DeleteObjectVersions(bucketName, objectList);
    EXPECT_EQ(dsOutcome.isSuccess(), true);
    EXPECT_EQ(dsOutcome.result().DeletedObjects().size(), keys.size());
    for (size_t i = 0; i < keys.size(); i++) {
        EXPECT_EQ(dsOutcome.result().DeletedObjects()[i].Key(), keys[i]);
    }

    lsvOutcome = Client->ListObjectVersions(ListObjectVersionsRequest(bucketName));
    EXPECT_EQ(lsvOutcome.isSuccess(), true);
    EXPECT_EQ(lsvOutcome.result().DeleteMarkerSummarys().size(), 0U);
    EXPECT_EQ(lsvOutcome.result().ObjectVersionSummarys().size(), 0U);

    //delete bucket
    auto dbOutcome = Client->DeleteBucket(bucketName);
    EXPECT_EQ(dbOutcome.isSuccess(), true);
}

TEST_F(ObjectVersioningTest, ListObjecsWithVersioningEnableTest)
{
    std::string bucketName = BucketName;
    bucketName.append("-listobjects");

    auto cbOutcome = Client->CreateBucket(CreateBucketRequest(bucketName));
    EXPECT_EQ(cbOutcome.isSuccess(), true);

    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(bucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(bucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    std::string keyPrefix = TestUtils::GetObjectKey("ListObjecsWithVersioningEnableTest");

    //Build Test Data
    size_t normalCnt = 10UL;
    std::vector<std::string> normalKeys;
    for (size_t i = 0; i < normalCnt; i++) {
        auto key = keyPrefix;
        key.append("/normal/").append(std::to_string(i)).append(".dat");
        normalKeys.push_back(key);

        auto pOutcome = Client->PutObject(bucketName, key, std::make_shared<std::stringstream>("just for test 1."));
        EXPECT_EQ(pOutcome.isSuccess(), true);

        auto dOutcome = Client->DeleteObject(bucketName, key);
        EXPECT_EQ(pOutcome.isSuccess(), true);

        pOutcome = Client->PutObject(bucketName, key, std::make_shared<std::stringstream>("just for test 2."));
        EXPECT_EQ(pOutcome.isSuccess(), true);

        pOutcome = Client->PutObject(bucketName, key, std::make_shared<std::stringstream>("just for test 3."));
        EXPECT_EQ(pOutcome.isSuccess(), true);
    }

    size_t xmlescapeCnt = 8UL;
    std::vector<std::string> xmlescapeKeys;
    for (size_t i = 0; i < xmlescapeCnt; i++) {
        auto key = keyPrefix;
        key.append("/xmlescape/").append(std::to_string(i)).append("\"&\'<>-.dat");
        xmlescapeKeys.push_back(key);

        auto pOutcome = Client->PutObject(bucketName, key, std::make_shared<std::stringstream>("just for test xmlescape 1."));
        EXPECT_EQ(pOutcome.isSuccess(), true);

        pOutcome = Client->PutObject(bucketName, key, std::make_shared<std::stringstream>("just for test xmlescape 2."));
        EXPECT_EQ(pOutcome.isSuccess(), true);
    }

    size_t hiddenCharsCnt = 5UL;
    std::vector<std::string> hiddenCharsKeys;
    for (size_t i = 0; i < hiddenCharsCnt; i++) {
        auto key = keyPrefix;
        key.append("/hiddenchar/");
        key.push_back(0x1c); key.push_back(0x1a);
        key.append(std::to_string(i)).append(".dat");
        hiddenCharsKeys.push_back(key);

        auto pOutcome = Client->PutObject(bucketName, key, std::make_shared<std::stringstream>("just for test hiddenchar 1."));
    }

    //Test Marker
    ListObjectVersionsRequest request(bucketName);
    request.setMaxKeys(1);
    bool IsTruncated = false;
    size_t total = 0UL;
    size_t size;
    do {
        auto outcome = Client->ListObjectVersions(request);
        EXPECT_EQ(outcome.isSuccess(), true);
        size = outcome.result().DeleteMarkerSummarys().size() + outcome.result().ObjectVersionSummarys().size();
        EXPECT_EQ(size, 1UL);
        EXPECT_EQ(outcome.result().CommonPrefixes().empty(), true);

        request.setKeyMarker(outcome.result().NextKeyMarker());
        request.setVersionIdMarker(outcome.result().NextVersionIdMarker());

        IsTruncated = outcome.result().IsTruncated();
        total++;
    } while (IsTruncated);
    EXPECT_EQ(total, (normalCnt*4 + xmlescapeCnt*2 + hiddenCharsCnt));

    //Test Delimiter
    CommonPrefixeList preflixList;
    std::string prefix = keyPrefix;
    prefix.append("/");
    ListObjectVersionsRequest request2(bucketName);
    request2.setDelimiter("/");
    request2.setMaxKeys(1000);
    request2.setPrefix(prefix);
    auto lsvOutcome = Client->ListObjectVersions(request2);
    EXPECT_EQ(lsvOutcome.isSuccess(), true);
    size = lsvOutcome.result().DeleteMarkerSummarys().size() + lsvOutcome.result().ObjectVersionSummarys().size();
    EXPECT_EQ(size, 0UL);
    preflixList.push_back(std::string(keyPrefix).append("/hiddenchar/"));
    preflixList.push_back(std::string(keyPrefix).append("/normal/"));
    preflixList.push_back(std::string(keyPrefix).append("/xmlescape/"));
    EXPECT_EQ(lsvOutcome.result().CommonPrefixes(), preflixList);

    //Test UrlEncoding
    prefix = keyPrefix;
    prefix.append("/hiddenchar/");
    ListObjectVersionsRequest request3(bucketName);
    request3.setEncodingType("url");
    request3.setPrefix(prefix);
    lsvOutcome = Client->ListObjectVersions(request3);
    EXPECT_EQ(lsvOutcome.isSuccess(), true);
    EXPECT_EQ(lsvOutcome.result().ObjectVersionSummarys().size(), hiddenCharsCnt);
    for (size_t i = 0; i < hiddenCharsKeys.size(); i++) {
        EXPECT_EQ(lsvOutcome.result().ObjectVersionSummarys().at(i).Key(), hiddenCharsKeys.at(i));
    }

    TestUtils::CleanBucketVersioning(*Client, bucketName);
    EXPECT_EQ(Client->DoesBucketExist(bucketName), false);
}

TEST_F(ObjectVersioningTest, ResumableUploadWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //multi-part mode
    std::string key = TestUtils::GetObjectKey("ResumableUploadWithVersioningEnableTestOverPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableUploadWithVersioningEnableTestOverPartSize").append(".tmp");
    // limit file size between 800KB and 2000KB
    int num = 8 + rand() % 12;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num + 10);

    UploadObjectRequest request(BucketName, key, tmpFile);
    request.setPartSize(100 * 1024);
    request.setThreadNum(3);
    auto ruOutcome = Client->ResumableUploadObject(request);
    EXPECT_EQ(ruOutcome.isSuccess(), true);
    EXPECT_EQ(ruOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(ruOutcome.result().VersionId().empty(), false);

    GetObjectRequest gRequest(BucketName, key);
    gRequest.setVersionId(ruOutcome.result().VersionId());
    auto gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(TestUtils::GetFileMd5(tmpFile), ComputeContentMD5(*gOutcome.result().Content()));
    RemoveFile(tmpFile);

    //put object mode
    std::string key1 = TestUtils::GetObjectKey("ResumableUploadWithVersioningEnableTestUnderPartSize");
    std::string tmpFile1 = TestUtils::GetTargetFileName("ResumableUploadWithVersioningEnableTestUnderPartSize").append(".tmp");
    num = rand() % 8;
    TestUtils::WriteRandomDatatoFile(tmpFile1, 10240 * num);

    UploadObjectRequest request1(BucketName, key1, tmpFile1);
    request1.setPartSize(100 * 1024);
    request1.setThreadNum(1);
    auto ruOutcome1 = Client->ResumableUploadObject(request1);
    EXPECT_EQ(ruOutcome1.isSuccess(), true);
    EXPECT_EQ(ruOutcome1.result().RequestId().size(), 24UL);
    EXPECT_EQ(ruOutcome1.result().VersionId().empty(), false);

    GetObjectRequest gRequest1(BucketName, key1);
    gRequest1.setVersionId(ruOutcome1.result().VersionId());
    auto gOutcome1 = Client->GetObject(gRequest1);
    EXPECT_EQ(gOutcome1.isSuccess(), true);
    EXPECT_EQ(TestUtils::GetFileMd5(tmpFile1), ComputeContentMD5(*gOutcome1.result().Content()));
    RemoveFile(tmpFile1);
}

TEST_F(ObjectVersioningTest, ResumableDownloadWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //multi-part mode
    std::string key = TestUtils::GetObjectKey("ResumableDownloadWithVersioningEnableTestOverPartSize");
    std::string tmpFile = TestUtils::GetTargetFileName("ResumableDownloadWithVersioningEnableTestOverPartSize").append(".tmp");
    std::string tmpFileDonwload = TestUtils::GetTargetFileName("ResumableDownloadWithVersioningEnableTestOverPartSize").append(".dat");
    // limit file size between 800KB and 2000KB
    int num = 8 + rand() % 12;
    TestUtils::WriteRandomDatatoFile(tmpFile, 1024 * 100 * num + 10);

    auto pOutcome = Client->PutObject(BucketName, key, tmpFile);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);

    auto dOutcome = Client->DeleteObject(BucketName, key);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().VersionId().empty(), false);
    EXPECT_EQ(dOutcome.result().DeleteMarker(), true);

    DownloadObjectRequest rdRequest(BucketName, key, tmpFileDonwload);
    rdRequest.setPartSize(100 * 1024);
    rdRequest.setThreadNum(3);
    auto rdOutcome = Client->ResumableDownloadObject(rdRequest);
    EXPECT_EQ(rdOutcome.isSuccess(), false);
    EXPECT_EQ(rdOutcome.error().Code(), "ServerError:404");

    rdRequest.setVersionId(pOutcome.result().VersionId());
    rdOutcome = Client->ResumableDownloadObject(rdRequest);
    EXPECT_EQ(rdOutcome.isSuccess(), true);
    EXPECT_EQ(rdOutcome.result().VersionId(), pOutcome.result().VersionId());
    EXPECT_EQ(TestUtils::GetFileMd5(tmpFile), TestUtils::GetFileMd5(tmpFileDonwload));

    RemoveFile(tmpFileDonwload);

    //get object mode
    std::string tmpFileDonwload1 = TestUtils::GetTargetFileName("ResumableDownloadWithVersioningEnableTestUnderPartSize").append(".dat");
    DownloadObjectRequest rdRequest1(BucketName, key, tmpFileDonwload1);
    rdRequest1.setPartSize(4 * 1024 * 1024);
    rdOutcome = Client->ResumableDownloadObject(rdRequest1);
    EXPECT_EQ(rdOutcome.isSuccess(), false);
    EXPECT_EQ(rdOutcome.error().Code(), "ServerError:404");

    rdRequest1.setVersionId(pOutcome.result().VersionId());
    rdOutcome = Client->ResumableDownloadObject(rdRequest1);
    EXPECT_EQ(rdOutcome.isSuccess(), true);
    EXPECT_EQ(rdOutcome.result().VersionId(), pOutcome.result().VersionId());
    EXPECT_EQ(TestUtils::GetFileMd5(tmpFile), TestUtils::GetFileMd5(tmpFileDonwload1));

    RemoveFile(tmpFileDonwload1);
    RemoveFile(tmpFile);
}

TEST_F(ObjectVersioningTest, ResumableCopyWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //multi-part mode
    std::string srcKey = TestUtils::GetObjectKey("ResumableCopyWithVersioningEnableTestOverPartSize");
    // limit file size between 800KB and 2000KB
    int num = 3 + rand() % 5;
    auto content = TestUtils::GetRandomStream(1024 * 100 * num + 10);

    auto pOutcome = Client->PutObject(BucketName, srcKey, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);

    auto dOutcome = Client->DeleteObject(BucketName, srcKey);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().VersionId().empty(), false);
    EXPECT_EQ(dOutcome.result().DeleteMarker(), true);

    std::string key = TestUtils::GetObjectKey("ResumableCopyWithVersioningEnableTestOverPartSize");

    MultiCopyObjectRequest mcRequest(BucketName, key, BucketName, srcKey);
    mcRequest.setPartSize(100 * 1024);
    mcRequest.setThreadNum(3);
    auto rcOutcome = Client->ResumableCopyObject(mcRequest);
    EXPECT_EQ(rcOutcome.isSuccess(), false);
    EXPECT_EQ(rcOutcome.error().Code(), "ServerError:404");

    mcRequest.setVersionId(pOutcome.result().VersionId());
    rcOutcome = Client->ResumableCopyObject(mcRequest);
    EXPECT_EQ(rcOutcome.isSuccess(), true);
    EXPECT_EQ(rcOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(rcOutcome.result().VersionId().empty(), false);

    auto gOutcome = Client->GetObject(BucketName, key);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(ComputeContentMD5(*gOutcome.result().Content()), ComputeContentMD5(*content));

    //copyObject mode
    key = TestUtils::GetObjectKey("ResumableCopyWithVersioningEnableTestUnderPartSize");

    MultiCopyObjectRequest mcRequest1(BucketName, key, BucketName, srcKey);
    mcRequest1.setPartSize(4 * 1024 * 1024);
    mcRequest1.setThreadNum(3);
    auto rcOutcome1 = Client->ResumableCopyObject(mcRequest1);
    EXPECT_EQ(rcOutcome1.isSuccess(), false);
    EXPECT_EQ(rcOutcome1.error().Code(), "ServerError:404");

    mcRequest1.setVersionId(pOutcome.result().VersionId());
    rcOutcome1 = Client->ResumableCopyObject(mcRequest1);
    EXPECT_EQ(rcOutcome1.isSuccess(), true);
    EXPECT_EQ(rcOutcome1.result().RequestId().size(), 24UL);
    EXPECT_EQ(rcOutcome1.result().VersionId().empty(), false);

    auto gOutcome1 = Client->GetObject(BucketName, key);
    EXPECT_EQ(gOutcome1.isSuccess(), true);
    EXPECT_EQ(ComputeContentMD5(*gOutcome1.result().Content()), ComputeContentMD5(*content));

}

TEST_F(ObjectVersioningTest, SignUrlWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    //test put, get, head and getmeta 
    auto content1 = std::make_shared<std::stringstream>("versioning test 1.");
    auto content2 = std::make_shared<std::stringstream>("versioning test 2.");
    auto etag1 = ComputeContentETag(*content1);
    auto etag2 = ComputeContentETag(*content2);
    auto key = TestUtils::GetObjectKey("SignUrlWithVersioningEnableTest");

    auto pOutcome = Client->PutObject(BucketName, key, content1);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId1 = pOutcome.result().VersionId();

    pOutcome = Client->PutObject(BucketName, key, content2);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId2 = pOutcome.result().VersionId();

    GeneratePresignedUrlRequest urlRequest(BucketName, key, Http::Get);
    auto urlOutcome = Client->GeneratePresignedUrl(urlRequest);
    EXPECT_EQ(urlOutcome.isSuccess(), true);
    EXPECT_EQ(urlOutcome.result().find("versionId="), std::string::npos);

    auto gOutcome = Client->GetObjectByUrl(urlOutcome.result());
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag2);

    urlRequest.setVersionId(versionId1);
    urlOutcome = Client->GeneratePresignedUrl(urlRequest);
    EXPECT_EQ(urlOutcome.isSuccess(), true);
    std::string pat = "versionId="; pat.append(UrlEncode(versionId1));
    EXPECT_NE(urlOutcome.result().find(pat), std::string::npos);

    gOutcome = Client->GetObjectByUrl(urlOutcome.result());
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag1);

    urlRequest.setVersionId(versionId2);
    urlOutcome = Client->GeneratePresignedUrl(urlRequest);
    EXPECT_EQ(urlOutcome.isSuccess(), true);
    pat = "versionId="; pat.append(UrlEncode(versionId2));
    EXPECT_NE(urlOutcome.result().find(pat), std::string::npos);

    gOutcome = Client->GetObjectByUrl(urlOutcome.result());
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(ComputeContentETag(*gOutcome.result().Content()), etag2);
}

TEST_F(ObjectVersioningTest, ProcessObjectWithVersioningEnableTest)
{
    auto bsOutcome = Client->SetBucketVersioning(SetBucketVersioningRequest(BucketName, VersioningStatus::Enabled));
    EXPECT_EQ(bsOutcome.isSuccess(), true);

    auto bfOutcome = Client->GetBucketInfo(BucketName);
    EXPECT_EQ(bfOutcome.isSuccess(), true);
    EXPECT_EQ(bfOutcome.result().VersioningStatus(), VersioningStatus::Enabled);

    if (bfOutcome.result().VersioningStatus() != VersioningStatus::Enabled)
        return;

    std::string imageFilePath = Config::GetDataPath();
    imageFilePath.append("example.jpg");
    std::string process = "image/resize,m_fixed,w_100,h_100";
    std::string imageInfo = "{\n    \"FileSize\": {\"value\": \"3267\"},\n    \"Format\": {\"value\": \"jpg\"},\n    \"ImageHeight\": {\"value\": \"100\"},\n    \"ImageWidth\": {\"value\": \"100\"},\n    \"ResolutionUnit\": {\"value\": \"1\"},\n    \"XResolution\": {\"value\": \"1/1\"},\n    \"YResolution\": {\"value\": \"1/1\"}}";
    auto key = TestUtils::GetObjectKey("ProcessObjectWithVersioningEnableTest");
    key.append("-example.jpg");

    auto pOutcome = Client->PutObject(BucketName, key, imageFilePath);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), false);
    auto versionId1 = pOutcome.result().VersionId();

    std::string saveAskey = TestUtils::GetObjectKey("ProcessObjectWithVersioningEnableTest");
    saveAskey.append("-saveas.jpg");

    GetObjectRequest gRequest(BucketName, key, process);
    auto gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), true);

    pOutcome = Client->PutObject(BucketName, saveAskey, gOutcome.result().Content());
    EXPECT_EQ(pOutcome.isSuccess(), true);

    gOutcome = Client->GetObject(GetObjectRequest(BucketName, saveAskey, "image/info"));
    EXPECT_EQ(gOutcome.isSuccess(), true);
    std::istreambuf_iterator<char> isb(*gOutcome.result().Content().get()), end;
    std::string imageInfo1(isb, end);

    const char * srcPtr = strstr(imageInfo.c_str(), "Format");
    const char * dstPtr = strstr(imageInfo1.c_str(), "Format");
    EXPECT_EQ(strcmp(srcPtr, dstPtr), 0);

    //do not support
    auto dOutcome = Client->DeleteObject(BucketName, key);
    EXPECT_EQ(dOutcome.isSuccess(), true);

    gRequest.setVersionId(versionId1);
    gOutcome = Client->GetObject(gRequest);
    EXPECT_EQ(gOutcome.isSuccess(), false);
    EXPECT_EQ(gOutcome.error().Code(), "NoSuchKey");

    std::stringstream ss;
    ss << process
        << "|sys/saveas"
        << ",o_" << Base64EncodeUrlSafe(saveAskey)
        << ",b_" << Base64EncodeUrlSafe(BucketName);

    ProcessObjectRequest poRequest(BucketName, key, ss.str());
    poRequest.setVersionId(versionId1);
    auto poOutcome = Client->ProcessObject(poRequest);
    EXPECT_EQ(poOutcome.isSuccess(), false);
    EXPECT_EQ(poOutcome.error().Code(), "NoSuchKey");
}

TEST_F(ObjectVersioningTest, ObjectOperationWithoutVersioningTest)
{
    auto client = std::make_shared<OssClient>(Config::Endpoint, Config::AccessKeyId, Config::AccessKeySecret, ClientConfiguration());
    auto bucketName = BucketName; bucketName.append("-no");
    client->CreateBucket(CreateBucketRequest(bucketName));

    auto content = std::make_shared<std::stringstream>("just for test.");
    auto key = TestUtils::GetObjectKey("ObjectOperationWithoutVersioningTest");

    auto pOutcome = client->PutObject(bucketName, key, content);
    EXPECT_EQ(pOutcome.isSuccess(), true);
    EXPECT_EQ(pOutcome.result().VersionId().empty(), true);
    EXPECT_EQ(pOutcome.result().RequestId().size(), 24UL);

    auto gOutcome = client->GetObject(bucketName, key);
    EXPECT_EQ(gOutcome.isSuccess(), true);
    EXPECT_EQ(gOutcome.result().VersionId().empty(), true);
    EXPECT_EQ(gOutcome.result().RequestId().size(), 24UL);

    auto saOutcome = client->SetObjectAcl(SetObjectAclRequest(bucketName, key, CannedAccessControlList::Private));
    EXPECT_EQ(saOutcome.isSuccess(), true);
    EXPECT_EQ(saOutcome.result().VersionId().empty(), true);
    EXPECT_EQ(saOutcome.result().RequestId().size(), 24UL);

    auto gaOutcome = client->GetObjectAcl(GetObjectAclRequest(bucketName, key));
    EXPECT_EQ(gaOutcome.isSuccess(), true);
    EXPECT_EQ(gaOutcome.result().VersionId().empty(), true);
    EXPECT_EQ(gaOutcome.result().Acl(), CannedAccessControlList::Private);
    EXPECT_EQ(gaOutcome.result().RequestId().size(), 24UL);

    auto hOutcome = client->HeadObject(HeadObjectRequest(bucketName, key));
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().VersionId().empty(), true);

    hOutcome = client->GetObjectMeta(GetObjectMetaRequest(bucketName, key));
    EXPECT_EQ(hOutcome.isSuccess(), true);
    EXPECT_EQ(hOutcome.result().VersionId().empty(), true);

    auto key_link = key; key_link.append("-link");
    CreateSymlinkRequest csRequest(bucketName, key_link);
    csRequest.SetSymlinkTarget(key);
    auto csOutcome = client->CreateSymlink(csRequest);
    EXPECT_EQ(csOutcome.isSuccess(), true);
    EXPECT_EQ(csOutcome.result().VersionId().empty(), true);
    EXPECT_EQ(csOutcome.result().RequestId().size(), 24UL);

    auto gsOutcome = client->GetSymlink(GetSymlinkRequest(bucketName, key_link));
    EXPECT_EQ(gsOutcome.isSuccess(), true);
    EXPECT_EQ(gsOutcome.result().VersionId().empty(), true);
    EXPECT_EQ(gsOutcome.result().RequestId().size(), 24UL);

    auto key_copy = key;  key_copy.append("-copy");
    CopyObjectRequest coRequest(bucketName, key_copy);
    coRequest.setCopySource(bucketName, key);
    auto cOutcome = client->CopyObject(coRequest);
    EXPECT_EQ(cOutcome.isSuccess(), true);
    EXPECT_EQ(cOutcome.result().VersionId().empty(), true);
    EXPECT_EQ(cOutcome.result().RequestId().size(), 24UL);

    auto key_append = key;  key_append.append("-append");
    content->clear();
    AppendObjectRequest aoRequest(bucketName, key_append, content);
    auto aOutcome = client->AppendObject(aoRequest);
    EXPECT_EQ(aOutcome.isSuccess(), true);
    EXPECT_EQ(aOutcome.result().VersionId().empty(), true);
    EXPECT_EQ(aOutcome.result().RequestId().size(), 24UL);

    auto dOutcome = client->DeleteObject(bucketName, key);
    EXPECT_EQ(dOutcome.isSuccess(), true);
    EXPECT_EQ(dOutcome.result().VersionId().empty(), true);
    EXPECT_EQ(dOutcome.result().RequestId().size(), 24UL);
    client->DeleteObject(bucketName, key_link);
    client->DeleteObject(bucketName, key_copy);
    client->DeleteObject(bucketName, key_append);

    client->DeleteBucket(bucketName);
}

TEST_F(ObjectVersioningTest, ObjectVersionsWithInvalidResponseBodyTest)
{
    ListObjectVersionsRequest lsRequest(BucketName);
    lsRequest.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto listOutcome = Client->ListObjectVersions(lsRequest);
    EXPECT_EQ(listOutcome.isSuccess(), false);
    EXPECT_EQ(listOutcome.error().Code(), "ParseXMLError");

    DeleteObjectVersionsRequest dovRequest(BucketName);
    dovRequest.addObject(ObjectIdentifier("key"));
    dovRequest.setResponseStreamFactory([=]() {
        auto content = std::make_shared<std::stringstream>();
        content->write("invlid data", 11);
        return content;
    });
    auto dOutcome = Client->DeleteObjectVersions(dovRequest);
    EXPECT_EQ(dOutcome.isSuccess(), false);
    EXPECT_EQ(dOutcome.error().Code(), "ParseXMLError");
}

TEST_F(ObjectVersioningTest, DeleteObjectVersionsResultTest)
{
    std::string xml = R"(
        <DeleteResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            <Deleted>
               <Key>multipart.data</Key>
               <VersionId>CAEQNRiBgIDyz.6C0BYiIGQ2NWEwNmVhNTA3ZTQ3MzM5ODliYjM1ZTdjYjA4MDE1</VersionId>
            </Deleted>
        </DeleteResult>)";

    DeleteObjectVersionsResult result(xml);
    EXPECT_EQ(result.Quiet(), false);
    EXPECT_EQ(result.DeletedObjects().size(), 1U);
    EXPECT_EQ(result.DeletedObjects()[0].Key(), "multipart.data");

    DeleteObjectVersionsResult result1("");
    EXPECT_EQ(result1.Quiet(), true);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <DeleteResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            <Deleted>
               <Key>multipart.data</Key>
               <VersionId>CAEQNRiBgIDyz.6C0BYiIGQ2NWEwNmVhNTA3ZTQ3MzM5ODliYjM1ZTdjYjA4MDE1</VersionId>
            </Deleted>
        <>
        )";
    result1 = DeleteObjectVersionsResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <DeleteResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            <EncodingType></EncodingType>
            <Deleted>
               <Key></Key>
               <VersionId></VersionId>
               <DeleteMarker></DeleteMarker>
               <DeleteMarkerVersionId></DeleteMarkerVersionId>
            </Deleted>
        <DeleteResult>)";
    result1 = DeleteObjectVersionsResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <DeleteResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            <EncodingType></EncodingType>
            <Deleted>
            </Deleted>
        <DeleteResult>)";
    result1 = DeleteObjectVersionsResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <DeleteResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
        <DeleteResult>)";
    result1 = DeleteObjectVersionsResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <OtherResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
        <OtherResult>)";
    result1 = DeleteObjectVersionsResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        )";
    result1 = DeleteObjectVersionsResult(xml);

    xml = R"(
        invalid xml
        )";
    result1 = DeleteObjectVersionsResult(xml);
}

TEST_F(ObjectVersioningTest, ObjectOperationNGTest)
{
    DeleteObjectVersionsRequest dsRequest("Invalid-Bucket");
    dsRequest.setQuiet(true);
    EXPECT_EQ(dsRequest.Quiet(), true);
    auto dsOutcome = Client->DeleteObjectVersions(dsRequest);
    EXPECT_EQ(dsOutcome.isSuccess(), false);


    ListObjectVersionsRequest loRequest("Invalid-Bucket");
    auto lsOutcome = Client->ListObjectVersions(loRequest);
    EXPECT_EQ(lsOutcome.isSuccess(), false);
}

TEST_F(ObjectVersioningTest, ListObjectVersionsResultTest)
{
    std::string xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <ListVersionsResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            <Name>oss-example</Name>
            <Prefix></Prefix>
            <KeyMarker>example</KeyMarker>
            <VersionIdMarker>CAEQMxiBgICbof2D0BYiIGRhZjgwMzJiMjA3MjQ0ODE5MWYxZDYwMzJlZjU1YmMy</VersionIdMarker>
            <MaxKeys>100</MaxKeys>
            <Delimiter></Delimiter>
            <IsTruncated>false</IsTruncated>
            <DeleteMarker>
                <Key>example</Key>
                <VersionId>CAEQMxiBgICAof2D0BYiIDJhMGE3N2M1YTI1NDQzOGY5NTkyNTI3MGYyMzJmNTI2</VersionId>
                <IsLatest>false</IsLatest>
                <LastModified>2019-04-09T07:27:28.000Z</LastModified>
                <Owner>
                  <ID>12345125285864390</ID>
                  <DisplayName>12345125285864390</DisplayName>
                </Owner>
            </DeleteMarker>
            <Version>
                <Key>example</Key>
                <VersionId>CAEQMxiBgMDNoP2D0BYiIDE3MWUxNzgxZDQxNTRiODI5OGYwZGMwNGY3MzZjNDVm</VersionId>
                <IsLatest>false</IsLatest>
                <LastModified>2019-04-09T07:27:28.000Z</LastModified>
                <ETag>"250F8A0AE989679A22926A875F0A2B95"</ETag>
                <Type>Normal</Type>
                <Size>93731</Size>
                <StorageClass>Standard</StorageClass>
                <Owner>
                  <ID>12345125285864390</ID>
                  <DisplayName>12345125285864390</DisplayName>
                </Owner>
            </Version>
        </ListVersionsResult>
        )";
    auto result = ListObjectVersionsResult(std::make_shared<std::stringstream>(xml));

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <ListVersionsResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            <Name></Name>
            <Prefix></Prefix>
            <KeyMarker></KeyMarker>
            <VersionIdMarker></VersionIdMarker>
            <MaxKeys></MaxKeys>
            <Delimiter></Delimiter>
            <IsTruncated></IsTruncated>
            <DeleteMarker>
                <Key></Key>
                <VersionId></VersionId>
                <IsLatest></IsLatest>
                <LastModified></LastModified>
                <Owner>
                  <ID></ID>
                  <DisplayName></DisplayName>
                </Owner>
            </DeleteMarker>
            <Version>
                <Key></Key>
                <VersionId></VersionId>
                <IsLatest></IsLatest>
                <LastModified></LastModified>
                <ETag></ETag>
                <Type></Type>
                <Size></Size>
                <StorageClass></StorageClass>
                <Owner>
                  <ID></ID>
                  <DisplayName></DisplayName>
                </Owner>
            </Version>
        </ListVersionsResult>
        )";
    result = ListObjectVersionsResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <ListVersionsResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            <Name></Name>
            <Prefix></Prefix>
            <KeyMarker></KeyMarker>
            <VersionIdMarker></VersionIdMarker>
            <MaxKeys></MaxKeys>
            <Delimiter></Delimiter>
            <IsTruncated></IsTruncated>
            <DeleteMarker>
                <Key></Key>
                <VersionId></VersionId>
                <IsLatest></IsLatest>
                <LastModified></LastModified>
                <Owner>
                </Owner>
            </DeleteMarker>
            <Version>
                <Key></Key>
                <VersionId></VersionId>
                <IsLatest></IsLatest>
                <LastModified></LastModified>
                <ETag></ETag>
                <Type></Type>
                <Size></Size>
                <StorageClass></StorageClass>
                <Owner>
                </Owner>
            </Version>
            <CommonPrefixes>
                <Prefix></Prefix>
            </CommonPrefixes>
        </ListVersionsResult>
        )";
    result = ListObjectVersionsResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <ListVersionsResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            <Name></Name>
            <Prefix></Prefix>
            <KeyMarker></KeyMarker>
            <VersionIdMarker></VersionIdMarker>
            <MaxKeys></MaxKeys>
            <Delimiter></Delimiter>
            <IsTruncated></IsTruncated>
            <EncodingType>url</EncodingType>
            <DeleteMarker>
                <Key></Key>
                <VersionId></VersionId>
                <IsLatest></IsLatest>
                <LastModified></LastModified>
                <Owner>
                </Owner>
            </DeleteMarker>
            <Version>
                <Key></Key>
                <VersionId></VersionId>
                <IsLatest></IsLatest>
                <LastModified></LastModified>
                <ETag></ETag>
                <Type></Type>
                <Size></Size>
                <StorageClass></StorageClass>
                <Owner>
                </Owner>
            </Version>
            <CommonPrefixes>
                <Prefix>key</Prefix>
            </CommonPrefixes>
        </ListVersionsResult>
        )";
    result = ListObjectVersionsResult(xml);


    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <ListVersionsResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
            <Name></Name>
            <Prefix></Prefix>
            <KeyMarker></KeyMarker>
            <VersionIdMarker></VersionIdMarker>
            <MaxKeys></MaxKeys>
            <Delimiter></Delimiter>
            <IsTruncated></IsTruncated>
            <NextKeyMarker></NextKeyMarker>
            <NextVersionIdMarker></NextVersionIdMarker>
            <EncodingType></EncodingType>
            <DeleteMarker>
            </DeleteMarker>
            <Version>
            </Version>
            <CommonPrefixes>
            </CommonPrefixes>
        </ListVersionsResult>
        )";
    result = ListObjectVersionsResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <ListVersionsResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
        </ListVersionsResult>
        )";
    result = ListObjectVersionsResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <NonListVersionsResult xmlns="http://doc.oss-cn-hangzhou.aliyuncs.com">
        </NonListVersionsResult>
        )";
    result = ListObjectVersionsResult(xml);

    xml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        )";
    result = ListObjectVersionsResult(xml);

    xml = R"(
        invalid xml
        )";
    result = ListObjectVersionsResult(xml);
}

TEST_F(ObjectVersioningTest, DeleteObjectVersionsRequestTest)
{
    DeleteObjectVersionsRequest request(BucketName);
    request.setRequestPayer(RequestPayer::Requester);
    auto headers = request.Headers();
    EXPECT_EQ(headers.at("x-oss-request-payer"), "requester");
}

}
}