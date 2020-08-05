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


#include <alibabacloud/oss/model/GetBucketWormResult.h>
#include <tinyxml2/tinyxml2.h>
using namespace AlibabaCloud::OSS;
using namespace tinyxml2;

GetBucketWormResult::GetBucketWormResult() :
    OssResult(),
    day_(0)
{
}

GetBucketWormResult::GetBucketWormResult(const std::string& result):
    GetBucketWormResult()
{
    *this = result;
}

GetBucketWormResult::GetBucketWormResult(const std::shared_ptr<std::iostream>& result):
    GetBucketWormResult()
{
    std::istreambuf_iterator<char> isb(*result.get()), end;
    std::string str(isb, end);
    *this = str;
}

GetBucketWormResult& GetBucketWormResult::operator =(const std::string& result)
{
    XMLDocument doc;
    XMLError xml_err;
    if ((xml_err = doc.Parse(result.c_str(), result.size())) == XML_SUCCESS) {
        XMLElement* root =doc.RootElement();
        if (root && !std::strncmp("WormConfiguration", root->Name(), 17)) {
            XMLElement *node;
            node = root->FirstChildElement("WormId");
            if (node && node->GetText()) wormId_ = node->GetText();

            node = root->FirstChildElement("CreationDate");
            if (node && node->GetText()) creationDate_ = node->GetText();

            node = root->FirstChildElement("State");
            if (node && node->GetText()) state_ = node->GetText();

            node = root->FirstChildElement("RetentionPeriodInDays");
            if (node && node->GetText()) day_ = (uint32_t)std::strtoul(node->GetText(), nullptr, 10);

            parseDone_ = true;
        }
    }
    return *this;
}
