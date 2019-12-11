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
#pragma once

#include <alibabacloud/oss/Export.h>
#include <string>
#include <vector>
namespace AlibabaCloud
{
namespace OSS
{
    class ALIBABACLOUD_OSS_EXPORT OSSBucketDestination
    {
    public:
        const std::string& Format() const { return format_; }
        const std::string& AccountId() const { return accountId_; }
        const std::string& RoleArn() const { return roleArn_; }
        const std::string& Bucket() const { return bucket_; }
        const std::string& Prefix() const { return prefix_; }
        const std::string& Encryption() const { return encryption_; }

        void setFormat(const std::string& format) { format_ = format; }
        void setAccountId(const std::string& accountId) { accountId_ = accountId; }
        void setRoleArn(const std::string& roleArn) { roleArn_ = roleArn; }
        void setBucket(const std::string& bucket) { bucket_ = bucket; }
        void setPrefix(const std::string& prefix) { prefix_ = prefix; }
        void setEncryption(const std::string& encryption) { encryption_ = encryption; }

    private:
        std::string format_;
        std::string accountId_;
        std::string roleArn_;
        std::string bucket_;
        std::string prefix_;
        std::string encryption_;
    };

    using OptionalFields = std::vector<InventoryOptionalFields>;

    class ALIBABACLOUD_OSS_EXPORT Filter
    {
        public:
        const std::string& Prefix() const { return prefix_; }

        void setPrefix(const std::string& prefix) { prefix_ = prefix; }

        private:
            std::string prefix_;
    };

    class ALIBABACLOUD_OSS_EXPORT InventoryConfiguration
    {
    public:
        const std::string& Id() const { return id_; }
        bool IsEnabled() const { return isEnabled_; }
        const AlibabaCloud::OSS::Filter& Filter() const { return filter_; }
        const OSSBucketDestination& Destination() const { return destination_; }
        const InventoryFrequency& Schedule() const { return schedule_; }
        const InventoryIncludedObjectVersions& IncludedObjectVersions() const { return includedObjectVersions_; }
        const AlibabaCloud::OSS::OptionalFields& OptionalFields() const { return optionalFields_; }

        void setId(const std::string& id) { id_ = id; }
        void setIsEnabled(bool isEnabled) { isEnabled_ = isEnabled; }
        void setFilter(const AlibabaCloud::OSS::Filter& prefix) { filter_ = prefix; }
        void setDestination(const OSSBucketDestination& destination) { destination_ = destination; }
        void setSchedule(const InventoryFrequency& schedule) { schedule_ = schedule; }
        void setIncludedObjectVersions(const InventoryIncludedObjectVersions& includedObjectVersions) { includedObjectVersions_ = includedObjectVersions; }
        void setOptionalFields(const AlibabaCloud::OSS::OptionalFields& opt) { optionalFields_ = opt; }

    private:
        std::string id_;
        bool isEnabled_;
        AlibabaCloud::OSS::Filter filter_;
        OSSBucketDestination destination_;
        InventoryFrequency schedule_;
        InventoryIncludedObjectVersions includedObjectVersions_;
        AlibabaCloud::OSS::OptionalFields optionalFields_;
    };

    using InventoryConfigurationList = std::vector<InventoryConfiguration>;
}
}
