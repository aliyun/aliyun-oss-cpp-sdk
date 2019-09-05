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
#include <string>
#include <vector>
#include <alibabacloud/oss/Export.h>
#include <alibabacloud/oss/Types.h>


class ALIBABACLOUD_OSS_EXPORT  MirrorMultiAlternate
{
public:
    MirrorMultiAlternate() = default;
    void setMirrorMultiAlternateNumber(const std::string& data)
    {
        number_ = data;
    }
    void setMirrorMultiAlternateUrl(const std::string& data)
    {
        url_ = data;
    }
    const std::string& getMirrorMultiAlternateNumber() const
    {
        return number_;
    }
    const std::string& getMirrorMultiAlternateUrl() const
    {
        return url_;
    }

private:
    std::string number_;
    std::string url_;
};

using MirrorMultiAlternates = std::vector<MirrorMultiAlternate>;

using stringlist = std::vector<std::string>;

class ALIBABACLOUD_OSS_EXPORT  MirrorHeadersSet
{
public:
    MirrorHeadersSet() = default;
    void setKey(const std::string& data)
    {
        key_ = data;
    }
    void setValue(const std::string& data)
    {
        value_ = data;
    }
    const std::string& getKey() const
    {
        return key_;
    }
    const std::string& getValue() const
    {
        return value_;
    }

private:
    std::string key_;
    std::string value_;
};

using MirrorHeadersSetList = std::vector<MirrorHeadersSet>;

class ALIBABACLOUD_OSS_EXPORT  RoutingRuleMirrorHeaders
{
public:
    RoutingRuleMirrorHeaders() = default;
    void setPassAll(const std::string& data)
    {
        passAll_ = data;
    }
    void setPass(stringlist& data)
    {
        pass_ = data;
    }
    void setRemove(stringlist& data)
    {
        remove_ = data;
    }
    void setMirrorHeadersSetList(MirrorHeadersSetList& data)
    {
        set_ = data;
    }
    bool hasRoutingRuleMirrorHeaders() const
    {
        return (!passAll_.empty() || !pass_.empty() || !remove_.empty() || !set_.empty());
    }
    const std::string& getPassAll() const
    {
        return passAll_;
    }
    const stringlist& getPass() const
    {
        return pass_;
    }
    const stringlist& getRemove() const
    {
        return remove_;
    }
    const MirrorHeadersSetList& getMirrorHeadersSetList() const
    {
        return set_;
    }
private:
    std::string passAll_;
    stringlist pass_;
    stringlist remove_;
    MirrorHeadersSetList set_;
};

class ALIBABACLOUD_OSS_EXPORT  RoutingRuleRedirect
{
public:
    RoutingRuleRedirect() : redirectType_(AlibabaCloud::OSS::RedirectType::INVALID) {}
    void setPassQueryString(const std::string& data)
    {
        passQueryString_ = data;
    }
    void setRedirectType(AlibabaCloud::OSS::RedirectType redirectType)
    {
        redirectType_ = redirectType;
    }
    void setMirrorURL(const std::string& data)
    {
        mirrorURL_ = data;
    }
    void setMirrorPassQueryString(const std::string& data)
    {
        mirrorPassQueryString_ = data;
    }
    void setMirrorFollowRedirect(const std::string& data)
    {
        mirrorFollowRedirect_ = data;
    }
    void setMirrorCheckMd5(const std::string& data)
    {
        mirrorCheckMd5_ = data;
    }
    void setMirrorHeaders(RoutingRuleMirrorHeaders& data)
    {
        mirrorHeaders_ = data;
    }
    void setProtocol(const std::string& data)
    {
        protocol_ = data;
    }
    void setHostName(const std::string& data)
    {
        hostName_ = data;
    }
    void setHttpRedirectCode(const std::string& data)
    {
        httpRedirectCode_ = data;
    }
    void setReplaceKeyPrefixWith(const std::string& data)
    {
        replaceKeyPrefixWith_ = data;
    }
    void setReplaceKeyWith(const std::string& data)
    {
        replaceKeyWith_ = data;
    }
    void setMirrorMultiAlternates(const MirrorMultiAlternates& data)
    {
        alternates = data;
    }
    const AlibabaCloud::OSS::RedirectType& getRedirectType() const
    {
        return redirectType_;
    }
    const std::string& getPassQueryString() const
    {
        return passQueryString_;
    }
    const std::string& getMirrorURL() const
    {
        return mirrorURL_;
    }
    const std::string& getMirrorPassQueryString() const
    {
        return mirrorPassQueryString_;
    }
    const std::string& getMirrorFollowRedirect() const
    {
        return mirrorFollowRedirect_;
    }
    const std::string& getMirrorCheckMd5() const
    {
        return mirrorCheckMd5_;
    }
    const RoutingRuleMirrorHeaders& getRoutingRuleMirrorHeaders() const
    {
        return mirrorHeaders_;
    }
    const std::string& getProtocol() const
    {
        return protocol_;
    }
    const std::string& getHostName() const
    {
        return hostName_;
    }
    const std::string& getHttpRedirectCode() const
    {
        return httpRedirectCode_;
    }
    const std::string& getReplaceKeyPrefixWith() const
    {
        return replaceKeyPrefixWith_;
    }
    const std::string& getReplaceKeyWith() const
    {
        return replaceKeyWith_;
    }
    const MirrorMultiAlternates& getMirrorMultiAlternates() const
    {
        return alternates;
    }
private:
    std::string passQueryString_;
    AlibabaCloud::OSS::RedirectType redirectType_;
    std::string mirrorURL_;
    std::string mirrorPassQueryString_;
    std::string mirrorFollowRedirect_;
    std::string mirrorCheckMd5_;
    RoutingRuleMirrorHeaders mirrorHeaders_;
    std::string protocol_;
    std::string hostName_;
    std::string httpRedirectCode_;
    std::string replaceKeyPrefixWith_;
    std::string replaceKeyWith_;
    MirrorMultiAlternates alternates;
};

class ALIBABACLOUD_OSS_EXPORT  RoutingRuleIncludeHeader
{
public:
    RoutingRuleIncludeHeader() = default;
    void setKey(const std::string& data)
    {
        key_ = data;
    }
    void setEquals(const std::string& data)
    {
        equals_ = data;
    }
    const std::string& getKey() const
    {
        return key_;
    }
    const std::string& getEquals() const
    {
        return equals_;
    }

private:
    std::string key_;
    std::string equals_;
};

using RoutingRuleIncludeHeaderList = std::vector<RoutingRuleIncludeHeader>;

class ALIBABACLOUD_OSS_EXPORT  RoutingRuleCondition
{
public:
    RoutingRuleCondition() = default;
    void setKeyPrefixEquals(const std::string& data)
    {
        keyPrefixEquals_ = data;
    }
    void setHttpErrorCodeReturnedEquals(const std::string& data)
    {
        httpErrorCodeReturnedEquals_ = data;
    }
    void setRoutingRuleIncludeHeaderList(RoutingRuleIncludeHeaderList& data)
    {
        includeHeaderList_ = data;
    }
    bool hasRoutingRuleCondition() const
    {
        return (!keyPrefixEquals_.empty() || !httpErrorCodeReturnedEquals_.empty() || !includeHeaderList_.empty());
    }
    bool hasKeyPrefixEquals() const
    {
        return !keyPrefixEquals_.empty();
    }
    bool hasHttpErrorCodeReturnedEquals() const
    {
        return !httpErrorCodeReturnedEquals_.empty();
    }
    bool hasRoutingRuleIncludeHeaderList() const
    {
        return !includeHeaderList_.empty();
    }
    const std::string& getKeyPrefixEquals() const
    {
        return keyPrefixEquals_;
    }
    const std::string& getHttpErrorCodeReturnedEquals() const
    {
        return httpErrorCodeReturnedEquals_;
    }
    const RoutingRuleIncludeHeaderList& getIncludeHeaderList() const
    {
        return includeHeaderList_;
    }
private:
    std::string keyPrefixEquals_;
    std::string httpErrorCodeReturnedEquals_;
    RoutingRuleIncludeHeaderList includeHeaderList_;
};

class ALIBABACLOUD_OSS_EXPORT WebsiteRoutingRule
{
public:
    WebsiteRoutingRule() = default;
    void setRoutingRuleCondition(RoutingRuleCondition& data)
    {
        condition_ = data;
    }
    void setRoutingRuleRedirect(RoutingRuleRedirect& data)
    {
        redirect_ = data;
    }
    void setRuleNumber(std::string num)
    {
        ruleNumber_ = num;
    }
    const RoutingRuleCondition& getRoutingRuleCondition() const { return condition_; }
    const RoutingRuleRedirect& getRoutingRuleRedirect() const { return redirect_; }
    const std::string& getRuleNumber() const { return ruleNumber_; }


private:
    std::string ruleNumber_;
    RoutingRuleCondition condition_;
    RoutingRuleRedirect redirect_;
};

using WebsiteRoutingRuleList = std::vector<WebsiteRoutingRule>;
