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


#include <alibabacloud/oss/model/GetBucketWebsiteResult.h>
#include <tinyxml2/tinyxml2.h>
#include "../utils/Utils.h"
using namespace AlibabaCloud::OSS;
using namespace tinyxml2;


GetBucketWebsiteResult::GetBucketWebsiteResult() :
    OssResult()
{
}

GetBucketWebsiteResult::GetBucketWebsiteResult(const std::string& result):
    GetBucketWebsiteResult()
{
    *this = result;
}

GetBucketWebsiteResult::GetBucketWebsiteResult(const std::shared_ptr<std::iostream>& result):
    GetBucketWebsiteResult()
{
    std::istreambuf_iterator<char> isb(*result.get()), end;
    std::string str(isb, end);
    *this = str;
}

GetBucketWebsiteResult& GetBucketWebsiteResult::operator =(const std::string& result)
{
    XMLDocument doc;
    XMLError xml_err;
    if ((xml_err = doc.Parse(result.c_str(), result.size())) == XML_SUCCESS) {
        XMLElement* root =doc.RootElement();
        if (root && !std::strncmp("WebsiteConfiguration", root->Name(), 20)) {
            XMLElement *node;
            node = root->FirstChildElement("IndexDocument");
            if (node) node = node->FirstChildElement("Suffix");
            if (node && node->GetText()) indexDocument_ = node->GetText();

            node = root->FirstChildElement("ErrorDocument");
            if (node) node = node->FirstChildElement("Key");
            if (node && node->GetText()) errorDocument_ = node->GetText();

            node = root->FirstChildElement("RoutingRules");
            if (node) {
                XMLElement* referer_node = node->FirstChildElement("RoutingRule");
                for (; referer_node; referer_node = referer_node->NextSiblingElement()) {
                    WebsiteRoutingRule routingrule;
                    XMLElement* rulenumber_node = referer_node->FirstChildElement("RuleNumber");
                    if (rulenumber_node && rulenumber_node->GetText()) routingrule.setRuleNumber(rulenumber_node->GetText());

                    XMLElement* condition_node = referer_node->FirstChildElement("Condition");
                    if (condition_node)
                    {
                        RoutingRuleCondition condition;
                        XMLElement* keyPrefixEquals_node = condition_node->FirstChildElement("KeyPrefixEquals");
                        if (keyPrefixEquals_node && keyPrefixEquals_node->GetText()) condition.setKeyPrefixEquals(keyPrefixEquals_node->GetText());

                        XMLElement* httpErrorCode_node = condition_node->FirstChildElement("HttpErrorCodeReturnedEquals");
                        if (httpErrorCode_node && httpErrorCode_node->GetText()) condition.setHttpErrorCodeReturnedEquals(httpErrorCode_node->GetText());

                        XMLElement* header_node = condition_node->FirstChildElement("IncludeHeader");
                        if (header_node) {
                            RoutingRuleIncludeHeaderList includeHeaderList;
                            for (; header_node; header_node = header_node->NextSiblingElement()) {

                                RoutingRuleIncludeHeader header;

                                XMLElement* key_node = header_node->FirstChildElement("Key");
                                if (key_node && key_node->GetText()) header.setKey(key_node->GetText());

                                XMLElement* equals_node = header_node->FirstChildElement("Equals");
                                if (equals_node && equals_node->GetText()) header.setEquals(equals_node->GetText());

                                includeHeaderList.push_back(header);
                            }
                            condition.setRoutingRuleIncludeHeaderList(includeHeaderList);
                        }
                        routingrule.setRoutingRuleCondition(condition);
                    }

                    XMLElement* redirect_node = referer_node->FirstChildElement("Redirect");
                    if (redirect_node)
                    {
                        RoutingRuleRedirect redirect;

                        XMLElement* redirectType_node = redirect_node->FirstChildElement("RedirectType");
                        if (redirectType_node && redirectType_node->GetText()) redirect.setRedirectType(ToRedirectType(redirectType_node->GetText()));

                        XMLElement* pass_node = redirect_node->FirstChildElement("PassQueryString");
                        if (pass_node && pass_node->GetText()) redirect.setPassQueryString(pass_node->GetText());

                        if (redirect.RedirectType() == RedirectType::Mirror) {
                            XMLElement* mirror_node = redirect_node->FirstChildElement("MirrorURL");
                            if (mirror_node && mirror_node->GetText()) redirect.setMirrorURL(mirror_node->GetText());

                            XMLElement* mirrorPass_node = redirect_node->FirstChildElement("MirrorPassQueryString");
                            if (mirrorPass_node && mirrorPass_node->GetText()) redirect.setMirrorPassQueryString(mirrorPass_node->GetText());

                            XMLElement* mirrorFollow_node = redirect_node->FirstChildElement("MirrorFollowRedirect");
                            if (mirrorFollow_node && mirrorFollow_node->GetText()) redirect.setMirrorFollowRedirect(mirrorFollow_node->GetText());

                            XMLElement* mirrorCheck_node = redirect_node->FirstChildElement("MirrorCheckMd5");
                            if (mirrorCheck_node && mirrorCheck_node->GetText()) redirect.setMirrorCheckMd5(mirrorCheck_node->GetText());

                            XMLElement* mirrorHeaders_node = redirect_node->FirstChildElement("MirrorHeaders");
                            if (mirrorHeaders_node)
                            {
                                RoutingRuleMirrorHeaders mirrorHeaders;

                                XMLElement* passAll_node = mirrorHeaders_node->FirstChildElement("PassAll");
                                if (passAll_node && passAll_node->GetText()) mirrorHeaders.setPassAll(passAll_node->GetText());

                                XMLElement* pass_node = mirrorHeaders_node->FirstChildElement("Pass");
                                if (pass_node) {
                                    stringlist pass;
                                    for (; pass_node; pass_node = pass_node->NextSiblingElement()) {
                                        if (pass_node && pass_node->GetText()) pass.push_back(pass_node->GetText());
                                    }
                                    mirrorHeaders.setPass(pass);
                                }

                                XMLElement* remove_node = mirrorHeaders_node->FirstChildElement("Remove");
                                if (remove_node) {
                                    stringlist remove;
                                    for (; remove_node; remove_node = remove_node->NextSiblingElement()) {
                                        if (remove_node && remove_node->GetText()) remove.push_back(remove_node->GetText());
                                    }
                                    mirrorHeaders.setRemove(remove);
                                }

                                XMLElement* set_node = mirrorHeaders_node->FirstChildElement("Set");
                                if (set_node) {
                                    MirrorHeadersSetList setList;
                                    for (; set_node; set_node = set_node->NextSiblingElement()) {
                                        MirrorHeadersSet	set;

                                        XMLElement* key_node = set_node->FirstChildElement("Key");
                                        if (key_node && key_node->GetText()) set.setKey(key_node->GetText());

                                        XMLElement* value_node = set_node->FirstChildElement("Value");
                                        if (value_node && value_node->GetText()) set.setValue(value_node->GetText());

                                        setList.push_back(set);
                                    }

                                    mirrorHeaders.setMirrorHeadersSetList(setList);
                                }

                                redirect.setMirrorHeaders(mirrorHeaders);
                            }
                        }
                        if ((redirect.RedirectType() == RedirectType::AliCDN) || 
                            (redirect.RedirectType() == RedirectType::External)) {
                            XMLElement* protocol_node = redirect_node->FirstChildElement("Protocol");
                            if (protocol_node && protocol_node->GetText()) redirect.setProtocol(protocol_node->GetText());

                            XMLElement* hostName_node = redirect_node->FirstChildElement("HostName");
                            if (hostName_node && hostName_node->GetText()) redirect.setHostName(hostName_node->GetText());

                            XMLElement* httpRedirectCode_node = redirect_node->FirstChildElement("HttpRedirectCode");
                            if (httpRedirectCode_node && httpRedirectCode_node->GetText()) redirect.setHttpRedirectCode(httpRedirectCode_node->GetText());
                        }

                        if ((redirect.RedirectType() == RedirectType::AliCDN) ||
                            (redirect.RedirectType() == RedirectType::External) ||
                            (redirect.RedirectType() == RedirectType::Internal)) {
                            XMLElement* replace_node = redirect_node->FirstChildElement("ReplaceKeyPrefixWith");
                            if (replace_node && replace_node->GetText()) redirect.setReplaceKeyPrefixWith(replace_node->GetText());

                            XMLElement* replaceKeyWith_node = redirect_node->FirstChildElement("ReplaceKeyWith");
                             if (replaceKeyWith_node && replaceKeyWith_node->GetText()) redirect.setReplaceKeyWith(replaceKeyWith_node->GetText());
                        }

                        XMLElement* AlternateURL_node = redirect_node->FirstChildElement("MirrorMultiAlternates");
                        if (AlternateURL_node)
                        {
                            MirrorMultiAlternates alternates;
                            XMLElement* url_node = AlternateURL_node->FirstChildElement("MirrorMultiAlternate");
                            if (url_node) {
                                for (; url_node; url_node = url_node->NextSiblingElement()) {
                                    MirrorMultiAlternate alternateUrl;
                                    XMLElement* AlternateNumber_node = url_node->FirstChildElement("MirrorMultiAlternateNumber");
                                    if (AlternateNumber_node && AlternateNumber_node->GetText()) alternateUrl.setMirrorMultiAlternateNumber(AlternateNumber_node->GetText());
                                    XMLElement* AlternateUrl_node = url_node->FirstChildElement("MirrorMultiAlternateURL");
                                    if (AlternateUrl_node && AlternateUrl_node->GetText()) alternateUrl.setMirrorMultiAlternateUrl(AlternateUrl_node->GetText());
                                    alternates.push_back(alternateUrl);
                                }
                            }
                            redirect.setMirrorMultiAlternates(alternates);
                        }


                        routingrule.setRoutingRuleRedirect(redirect);
                    }
                    websiteRoutingRule_.push_back(routingrule);
                }
            }
            parseDone_ = true;
        }
    }
    return *this;
}
