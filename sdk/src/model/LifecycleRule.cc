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


#include <alibabacloud/oss/model/LifecycleRule.h>

using namespace AlibabaCloud::OSS;

LifeCycleExpiration::LifeCycleExpiration() :
    days_(0),
    createdBeforeDate_()
{
}

LifeCycleExpiration::LifeCycleExpiration(uint32_t days) :
    days_(days),
    createdBeforeDate_()
{
}

LifeCycleExpiration::LifeCycleExpiration(const std::string &createdBeforeDate) :
    days_(0),
    createdBeforeDate_(createdBeforeDate)
{
}

void LifeCycleExpiration::setDays(uint32_t days) 
{
    days_ = days; 
    createdBeforeDate_.clear();
}

void LifeCycleExpiration::setCreatedBeforeDate(const std::string &date) 
{
    createdBeforeDate_ = date; 
    days_ = 0;
}

LifeCycleTransition::LifeCycleTransition(const LifeCycleExpiration& expiration, AlibabaCloud::OSS::StorageClass storageClass) :
    expiration_(expiration),
    storageClass_(storageClass)
{
}

void LifeCycleTransition::setExpiration(const LifeCycleExpiration &expiration)
{
    expiration_ = expiration;
}

void LifeCycleTransition::setStorageClass(AlibabaCloud::OSS::StorageClass storageClass)
{
    storageClass_ = storageClass;
}

bool LifecycleRule::hasExpiration() const
{
    return (expiration_.Days() > 0 || !expiration_.CreatedBeforeDate().empty());
}

bool LifecycleRule::hasTransitionList() const
{
    return !transitionList_.empty();
}

bool LifecycleRule::hasAbortMultipartUpload() const
{
    return (abortMultipartUpload_.Days() > 0 || !abortMultipartUpload_.CreatedBeforeDate().empty());
}

bool LifecycleRule::operator==(const LifecycleRule& right) const
{
    if (id_ != right.id_ ||
        prefix_ != right.prefix_ ||
        status_ != right.status_) {
        return false;
    }

    if (expiration_.Days() != right.expiration_.Days() ||
        expiration_.CreatedBeforeDate() != right.expiration_.CreatedBeforeDate()) {
        return false;
    }

    if (abortMultipartUpload_.Days() != right.abortMultipartUpload_.Days() ||
        abortMultipartUpload_.CreatedBeforeDate() != right.abortMultipartUpload_.CreatedBeforeDate()) {
        return false;
    }

    if (transitionList_.size() != right.transitionList_.size()) {
        return false;
    }

    auto first = transitionList_.begin();
    auto Rightfirst = right.transitionList_.begin();

    for (; first != transitionList_.end(); ) {

        if (first->Expiration().Days() != Rightfirst->Expiration().Days() ||
            first->Expiration().CreatedBeforeDate() != Rightfirst->Expiration().CreatedBeforeDate() ||
            first->StorageClass() != Rightfirst->StorageClass()) {
            return false;
        }
        first++;
        Rightfirst++;
    }

    if (tagSet_.size() != right.tagSet_.size()) {
        return false;
    }

    auto firstTag = tagSet_.begin();
    auto RightfirstTag = right.tagSet_.begin();

    for (; firstTag != tagSet_.end(); ) {

        if (firstTag->Key()  != RightfirstTag->Key() ||
            firstTag->Value()!= RightfirstTag->Value()) {
            return false;
        }
        firstTag++;
        RightfirstTag++;
    }

    return true;
}
