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

#include <alibabacloud/oss/model/SetBucketInventoryConfigurationRequest.h>
#include "../utils/Utils.h"
#include <sstream>

using namespace AlibabaCloud::OSS;

SetBucketInventoryConfigurationRequest::SetBucketInventoryConfigurationRequest(const std::string& bucket) :
    OssBucketRequest(bucket)
{
    setFlags(Flags() | REQUEST_FLAG_CONTENTMD5);
}

void SetBucketInventoryConfigurationRequest::setInventoryConfiguration(InventoryConfiguration conf)
{
    inventoryConfiguration_ = conf;
}

ParameterCollection SetBucketInventoryConfigurationRequest::specialParameters() const
{
    ParameterCollection parameters;
    parameters["inventory"] = "";
    parameters["inventoryId"] = inventoryConfiguration_.Id();
    return parameters;
}

std::string SetBucketInventoryConfigurationRequest::payload() const
{
    std::stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
    ss << "<InventoryConfiguration>" << std::endl;
    ss << "<Id>" << inventoryConfiguration_.Id() << "</Id>" << std::endl;
    ss << "<IsEnabled>" << (inventoryConfiguration_.IsEnabled() ? "true" : "false") << "</IsEnabled>" << std::endl;

    ss << "<Filter>" << std::endl;
    ss << "<Prefix>" << inventoryConfiguration_.Filter().Prefix() << "</Prefix>" << std::endl;
    ss << "</Filter>" << std::endl;

    ss << "<Destination>" << std::endl;
    ss << "<OSSBucketDestination>" << std::endl;
    ss << "<Format>" << inventoryConfiguration_.Destination().Format() << "</Format>" << std::endl;
    ss << "<AccountId>" << inventoryConfiguration_.Destination().AccountId() << "</AccountId>" << std::endl;
    ss << "<RoleArn>" << inventoryConfiguration_.Destination().RoleArn() << "</RoleArn>" << std::endl;
    ss << "<Bucket>" << inventoryConfiguration_.Destination().Bucket() << "</Bucket>" << std::endl;
    ss << "<Prefix>" << inventoryConfiguration_.Destination().Prefix() << "</Prefix>" << std::endl;
    ss << "<Encryption>" << std::endl;
    ss << "<SSE-KMS>" << std::endl;
    ss << "<KeyId>" << inventoryConfiguration_.Destination().Encryption() << "</KeyId>" << std::endl;
    ss << "</SSE-KMS>" << std::endl;
    ss << "</Encryption>" << std::endl;
    ss << "</OSSBucketDestination>" << std::endl;
    ss << "</Destination>" << std::endl;

    ss << "<Schedule>" << std::endl;
    ss << "<Frequency>" << ToInventoryFrequencyName(inventoryConfiguration_.Schedule()) << "</Frequency>" << std::endl;
    ss << "</Schedule>" << std::endl;
    ss << "<IncludedObjectVersions>" << ToInventoryIncludedObjectVersionsName(inventoryConfiguration_.IncludedObjectVersions()) << "</IncludedObjectVersions>" << std::endl;

    ss << "  <OptionalFields>" << std::endl;
    for (const auto& field : inventoryConfiguration_.OptionalFields()) {
        ss << "    <Field>" << ToInventoryOptionalFieldsName(field) << "</Field>" << std::endl;
    }
    ss << "  </OptionalFields>" << std::endl;

    ss << "</InventoryConfiguration>" << std::endl;
    return ss.str();
}