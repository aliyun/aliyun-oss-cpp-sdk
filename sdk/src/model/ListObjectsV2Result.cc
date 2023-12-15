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


#include <alibabacloud/oss/model/ListObjectsV2Result.h>
#include <tinyxml2/tinyxml2.h>
#include <alibabacloud/oss/model/Owner.h>
#include "../utils/Utils.h"
using namespace AlibabaCloud::OSS;
using namespace tinyxml2;

ListObjectsV2Result::ListObjectsV2Result() :
    OssResult(),
    name_(),
    prefix_(),
    startAfter_(),
    continuationToken_(),
    nextContinuationToken_(),
    delimiter_(),
    maxKeys_(),
    keyCount_(),
    isTruncated_(false),
    commonPrefixes_(),
    objectSummarys_()
{
}

ListObjectsV2Result::ListObjectsV2Result(const std::string& result):
    ListObjectsV2Result()
{
    *this = result;
}

ListObjectsV2Result::ListObjectsV2Result(const std::shared_ptr<std::iostream>& result):
    ListObjectsV2Result()
{
    std::istreambuf_iterator<char> isb(*result.get()), end;
    std::string str(isb, end);
    *this = str;
}

ListObjectsV2Result& ListObjectsV2Result::operator =(const std::string& result)
{
    XMLDocument doc;
    XMLError xml_err;
    if ((xml_err = doc.Parse(result.c_str(), result.size())) == XML_SUCCESS) {
        XMLElement* root =doc.RootElement();
        if (root && !std::strncmp("ListBucketResult", root->Name(), 16)) {
            XMLElement *node;
            node = root->FirstChildElement("Name");
            if (node && node->GetText()) name_ = node->GetText();

            node = root->FirstChildElement("Prefix");
            if (node && node->GetText()) prefix_ = node->GetText();

            node = root->FirstChildElement("StartAfter");
            if (node && node->GetText()) startAfter_ = node->GetText();

            node = root->FirstChildElement("ContinuationToken");
            if (node && node->GetText()) continuationToken_ = node->GetText();

            node = root->FirstChildElement("NextContinuationToken");
            if (node && node->GetText()) nextContinuationToken_ = node->GetText();

            node = root->FirstChildElement("Delimiter");
            if (node && node->GetText()) delimiter_ = node->GetText();

            node = root->FirstChildElement("MaxKeys");
            if (node && node->GetText()) maxKeys_ = atoi(node->GetText());

            node = root->FirstChildElement("KeyCount");
            if (node && node->GetText()) keyCount_ = atoi(node->GetText());

            node = root->FirstChildElement("IsTruncated");
            if (node && node->GetText()) isTruncated_ = !std::strncmp("true", node->GetText(), 4);

            node = root->FirstChildElement("EncodingType");
            if (node && node->GetText()) encodingType_ = node->GetText();

            //Detect encode type
            bool useUrlDecode = !ToLower(encodingType_.c_str()).compare(0, 3, "url", 3);

            //CommonPrefixes
            node = root->FirstChildElement("CommonPrefixes");
            for (; node; node = node->NextSiblingElement("CommonPrefixes")) {
                XMLElement *prefix_node = node->FirstChildElement("Prefix");
                if (prefix_node && prefix_node->GetText()) commonPrefixes_.push_back(
                    (useUrlDecode ? UrlDecode(prefix_node->GetText()) : prefix_node->GetText()));
            }

            //Contents
            XMLElement *contents_node = root->FirstChildElement("Contents");
            for (; contents_node; contents_node = contents_node->NextSiblingElement("Contents")) {
                ObjectSummary content;
                node = contents_node->FirstChildElement("Key");
                if (node && node->GetText()) content.key_ = useUrlDecode ? UrlDecode(node->GetText()) : node->GetText();

                node = contents_node->FirstChildElement("LastModified");
                if (node && node->GetText()) content.lastModified_ = node->GetText();

                node = contents_node->FirstChildElement("ETag");
                if (node && node->GetText()) content.eTag_ = TrimQuotes(node->GetText());

                node = contents_node->FirstChildElement("Size");
                if (node && node->GetText()) content.size_ = std::atoll(node->GetText());

                node = contents_node->FirstChildElement("StorageClass");
                if (node && node->GetText()) content.storageClass_ = node->GetText();

                node = contents_node->FirstChildElement("Type");
                if (node && node->GetText()) content.type_ = node->GetText();

                node = contents_node->FirstChildElement("Owner");
                std::string owner_ID, owner_DisplayName;
                if (node) {
                    XMLElement *sub_node;
                    sub_node = node->FirstChildElement("ID");
                    if (sub_node && sub_node->GetText()) owner_ID = sub_node->GetText();

                    sub_node = node->FirstChildElement("DisplayName");
                    if (sub_node && sub_node->GetText()) owner_DisplayName = sub_node->GetText();
                }
                content.owner_ = Owner(owner_ID, owner_DisplayName);

                node = contents_node->FirstChildElement("RestoreInfo");
                if (node && node->GetText()) content.restoreInfo_ = node->GetText();

                objectSummarys_.push_back(content);
            }

            //EncodingType
            if (useUrlDecode) {
                delimiter_  = UrlDecode(delimiter_);
                startAfter_ = UrlDecode(startAfter_);
                prefix_     = UrlDecode(prefix_);
            }
        }

        //TODO check the result and the parse flag;
        parseDone_ = true;
    }

    return *this;
}

