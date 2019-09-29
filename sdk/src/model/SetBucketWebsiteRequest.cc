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

#include <alibabacloud/oss/model/SetBucketWebsiteRequest.h>
#include <sstream>
#include <../utils/Utils.h>
#include "ModelError.h"

using namespace AlibabaCloud::OSS;

SetBucketWebsiteRequest::SetBucketWebsiteRequest(const std::string& bucket) :
    OssBucketRequest(bucket),
    indexDocumentIsSet_(false),
    errorDocumentIsSet_(false)
{
    setFlags(Flags() | REQUEST_FLAG_CONTENTMD5);
}

std::string SetBucketWebsiteRequest::payload() const
{
    std::stringstream ss;
    ss << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>" << std::endl;
    ss << "<WebsiteConfiguration>" << std::endl;
    ss << "  <IndexDocument>" << std::endl;
    ss << "    <Suffix>" << indexDocument_ << "</Suffix>" << std::endl;
    ss << "  </IndexDocument>" << std::endl;
    if (errorDocumentIsSet_) {
        ss << "  <ErrorDocument>" << std::endl;
        ss << "    <Key>" << errorDocument_ << "</Key>" << std::endl;
        ss << "  </ErrorDocument>" << std::endl;
    }
    if (!websiteRoutingRule_.empty()) {
        ss << "  <RoutingRules>" << std::endl;
        for (const auto& rule : websiteRoutingRule_) {
            ss << "  <RoutingRule>" << std::endl;
            ss << "  <RuleNumber>" << rule.RuleNumber() << "</RuleNumber>" << std::endl;
            if (rule.RoutingRuleCondition().RoutingRuleConditionIsSet()) {
                ss << "<Condition>" << std::endl;
                if (!rule.RoutingRuleCondition().KeyPrefixEquals().empty()) {
                    ss << "<KeyPrefixEquals>" << rule.RoutingRuleCondition().KeyPrefixEquals() << "</KeyPrefixEquals>" << std::endl;
                }
                if (!rule.RoutingRuleCondition().HttpErrorCodeReturnedEquals().empty()) {
                    ss << "<HttpErrorCodeReturnedEquals>" << rule.RoutingRuleCondition().HttpErrorCodeReturnedEquals() << "</HttpErrorCodeReturnedEquals>" << std::endl;
                }
                if (!rule.RoutingRuleCondition().IncludeHeaderList().empty()) {
                    ss << "<IncludeHeader>" << std::endl;

                    for (const auto& header : rule.RoutingRuleCondition().IncludeHeaderList()) {
                        ss << "  <Key>" << header.Key() << "</Key>" << std::endl;
                        ss << "  <Equals>" << header.Equals() << "</Equals>" << std::endl;
                    }
                    ss << "</IncludeHeader>" << std::endl;
                }
                ss << "</Condition>" << std::endl;
            }

            ss << "  <Redirect>" << std::endl;
            ss << "  <RedirectType>" << ToRedirectTypeName(rule.RoutingRuleRedirect().RedirectType()) << "</RedirectType>" << std::endl;
            if (!rule.RoutingRuleRedirect().PassQueryString().empty())
            {
                ss << "  <PassQueryString>" << rule.RoutingRuleRedirect().PassQueryString() << "</PassQueryString>" << std::endl;
            }
            if (rule.RoutingRuleRedirect().RedirectType() == RedirectType::Mirror) {
                if (!rule.RoutingRuleRedirect().MirrorURL().empty())
                {
                    ss << "  <MirrorURL>" << rule.RoutingRuleRedirect().MirrorURL() << "</MirrorURL>" << std::endl;
                }
                if (!rule.RoutingRuleRedirect().MirrorPassQueryString().empty())
                {
                    ss << "  <MirrorPassQueryString>" << rule.RoutingRuleRedirect().MirrorPassQueryString() << "</MirrorPassQueryString>" << std::endl;
                }
                if (!rule.RoutingRuleRedirect().MirrorFollowRedirect().empty())
                {
                    ss << "  <MirrorFollowRedirect>" << rule.RoutingRuleRedirect().MirrorFollowRedirect() << "</MirrorFollowRedirect>" << std::endl;
                }
                if (!rule.RoutingRuleRedirect().MirrorCheckMd5().empty())
                {
                    ss << "  <MirrorCheckMd5>" << rule.RoutingRuleRedirect().MirrorCheckMd5() << "</MirrorCheckMd5>" << std::endl;
                }

                if (rule.RoutingRuleRedirect().RoutingRuleMirrorHeaders().RoutingRuleMirrorHeadersIsSet())
                {
                    ss << "  <MirrorHeaders>" << std::endl;
                    if (!rule.RoutingRuleRedirect().RoutingRuleMirrorHeaders().PassAll().empty())
                    {
                        ss << "  <PassAll>" << rule.RoutingRuleRedirect().RoutingRuleMirrorHeaders().PassAll() << "</PassAll>" << std::endl;
                    }
                    if (!rule.RoutingRuleRedirect().RoutingRuleMirrorHeaders().Pass().empty())
                    {
                        for (const auto& pass : rule.RoutingRuleRedirect().RoutingRuleMirrorHeaders().Pass())
                        {
                            ss << "  <Pass>" << pass << "</Pass>" << std::endl;
                        }
                    }
                    if (!rule.RoutingRuleRedirect().RoutingRuleMirrorHeaders().Remove().empty())
                    {
                        for (const auto& remove : rule.RoutingRuleRedirect().RoutingRuleMirrorHeaders().Remove())
                        {
                            ss << "  <Remove>" << remove << "</Remove>" << std::endl;
                        }
                    }
                    if (!rule.RoutingRuleRedirect().RoutingRuleMirrorHeaders().MirrorHeadersSetList().empty())
                    {
                        ss << "  <Set>" << std::endl;
                        for (const auto& set : rule.RoutingRuleRedirect().RoutingRuleMirrorHeaders().MirrorHeadersSetList())
                        {

                            ss << "  <Key>" << set.Key() << "</Key>" << std::endl;
                            ss << "  <Value>" << set.Value() << "</Value>" << std::endl;
                        }
                        ss << "  </Set>" << std::endl;
                    }

                    ss << "  </MirrorHeaders>" << std::endl;

                }
            }
            if (!rule.RoutingRuleRedirect().MirrorMultiAlternates().empty())
            {
                ss << "<MirrorMultiAlternates>" << std::endl;
                for (const auto& data : rule.RoutingRuleRedirect().MirrorMultiAlternates())
                {
                    ss << "<MirrorMultiAlternate>" << std::endl;

                    ss << "<MirrorMultiAlternateNumber>" << data.MirrorMultiAlternateNumber() << "</MirrorMultiAlternateNumber>" << std::endl;
                    ss << "<MirrorMultiAlternateURL>" << data.MirrorMultiAlternateUrl() << "</MirrorMultiAlternateURL>" << std::endl;

                    ss << "</MirrorMultiAlternate>" << std::endl;
                }
                ss << "</MirrorMultiAlternates>" << std::endl;
            }
            if ((rule.RoutingRuleRedirect().RedirectType() == RedirectType::External) || 
                (rule.RoutingRuleRedirect().RedirectType() == RedirectType::AliCDN)) {
                if (!rule.RoutingRuleRedirect().Protocol().empty())
                {
                    ss << "  <Protocol>" << rule.RoutingRuleRedirect().Protocol() << "</Protocol>" << std::endl;
                }
                if (!rule.RoutingRuleRedirect().HostName().empty())
                {
                    ss << "  <HostName>" << rule.RoutingRuleRedirect().HostName() << "</HostName>" << std::endl;
                }
                if (!rule.RoutingRuleRedirect().HttpRedirectCode().empty())
                {
                    ss << "  <HttpRedirectCode>" << rule.RoutingRuleRedirect().HttpRedirectCode() << "</HttpRedirectCode>" << std::endl;
                }
            }
            if ((rule.RoutingRuleRedirect().RedirectType() == RedirectType::External) ||
                (rule.RoutingRuleRedirect().RedirectType() == RedirectType::Internal) ||
                rule.RoutingRuleRedirect().RedirectType() == RedirectType::AliCDN) {
                if (!rule.RoutingRuleRedirect().ReplaceKeyPrefixWith().empty())
                {
                    ss << "  <ReplaceKeyPrefixWith>" << rule.RoutingRuleRedirect().ReplaceKeyPrefixWith() << "</ReplaceKeyPrefixWith>" << std::endl;
                }
                if (!rule.RoutingRuleRedirect().ReplaceKeyWith().empty())
                {
                    ss << "  <ReplaceKeyWith>" << rule.RoutingRuleRedirect().ReplaceKeyWith() << "</ReplaceKeyWith>" << std::endl;
                }
            }
            ss << "  </Redirect>" << std::endl;

            ss << "  </RoutingRule>" << std::endl;
        }
        ss << "  </RoutingRules>" << std::endl;
    }

    ss << "</WebsiteConfiguration>" << std::endl;
    return ss.str();
}

ParameterCollection SetBucketWebsiteRequest::specialParameters() const
{
    ParameterCollection parameters;
    parameters["website"] = "";
    return parameters;
}

static bool IsValidWebpage(const std::string &webpage)
{
    const std::string pageSuffix = ".html";
    return (webpage.size() > pageSuffix.size()) &&
        (webpage.substr(webpage.size() - 5).compare(".html") == 0);
}

int SetBucketWebsiteRequest::validate() const
{
    int ret = OssBucketRequest::validate();
    if (ret != 0) 
        return ret;

    if (indexDocument_.empty())
        return ARG_ERROR_WEBSITE_INDEX_DOCCUMENT_EMPTY;

    if (!IsValidWebpage(indexDocument_))
        return ARG_ERROR_WEBSITE_INDEX_DOCCUMENT_NAME_INVALID;

    if (errorDocumentIsSet_ && !IsValidWebpage(errorDocument_))
        return ARG_ERROR_WEBSITE_ERROR_DOCCUMENT_NAME_INVALID;

    return 0;
}

