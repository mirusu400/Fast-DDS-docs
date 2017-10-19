// Copyright 2017 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include <tinyxml2.h>
#include <fastrtps/xmlparser/XMLParserCommon.h>
#include <fastrtps/xmlparser/XMLParser.h>

namespace eprosima {
namespace fastrtps {
namespace xmlparser {

std::map<std::string, ParticipantAttributes*> XMLParser::m_participant_profiles;
ParticipantAttributes default_participant_attributes;
std::map<std::string, PublisherAttributes*>   XMLParser::m_publisher_profiles;
PublisherAttributes default_publisher_attributes;
std::map<std::string, SubscriberAttributes*>  XMLParser::m_subscriber_profiles;
SubscriberAttributes default_subscriber_attributes;

BaseNode* XMLParser::root = nullptr;

XMLP_ret XMLParser::fillParticipantAttributes(const std::string &profile_name, ParticipantAttributes &atts)
{
    part_map_iterator_t it = m_participant_profiles.find(profile_name);
    if (it == m_participant_profiles.end())
    {
        logError(XMLPARSER, "Profile '" << profile_name << "' not found '");
        return XMLP_ret::XML_ERROR;
    }
    atts = *it->second;
    return XMLP_ret::XML_OK;
}

void XMLParser::getDefaultParticipantAttributes(ParticipantAttributes& participant_attributes)
{
    participant_attributes = default_participant_attributes;
}

void XMLParser::getDefaultPublisherAttributes(PublisherAttributes& publisher_attributes)
{
    publisher_attributes = default_publisher_attributes;
}

void XMLParser::getDefaultSubscriberAttributes(SubscriberAttributes& subscriber_attributes)
{
    subscriber_attributes = default_subscriber_attributes;
}

XMLP_ret XMLParser::fillPublisherAttributes(const std::string &profile_name, PublisherAttributes &atts)
{
    publ_map_iterator_t it = m_publisher_profiles.find(profile_name);
    if (it == m_publisher_profiles.end())
    {
        logError(XMLPARSER, "Profile '" << profile_name << "' not found '");
        return XMLP_ret::XML_ERROR;
    }
    atts = *it->second;
    return XMLP_ret::XML_OK;
}

XMLP_ret XMLParser::fillSubscriberAttributes(const std::string &profile_name, SubscriberAttributes &atts)
{
    subs_map_iterator_t it = m_subscriber_profiles.find(profile_name);
    if (it == m_subscriber_profiles.end())
    {
        logError(XMLPARSER, "Profile '" << profile_name << "' not found");
        return XMLP_ret::XML_ERROR;
    }
    atts = *it->second;
    return XMLP_ret::XML_OK;
}

XMLP_ret XMLParser::loadDefaultXMLFile()
{
    return loadXMLFile(DEFAULT_FASTRTPS_PROFILES);
}

XMLP_ret XMLParser::parseXML(XMLDocument& xmlDoc)
{
    XMLP_ret ret;
    XMLElement* p_root = xmlDoc.FirstChildElement(ROOT);
    if (nullptr == p_root)
    {
        if (nullptr == (p_root = xmlDoc.FirstChildElement(PROFILES)))
        {
            logError(XMLPARSER, "Not found root tag");
            ret = XMLP_ret::XML_ERROR;
        }
        else
        {
            root = new BaseNode{nullptr, NodeType::ROOT};
            ret = parseProfiles(p_root, *root);
        }
    }
    else
    {
        root = new BaseNode{nullptr, NodeType::ROOT};
        ret = parseRoot(p_root, *root);
    }
    return ret;
}

XMLP_ret XMLParser::parseRoot(XMLElement* p_root, BaseNode& rootNode)
{
    XMLP_ret ret;
    XMLElement *root_child = nullptr;
    if (nullptr == (root_child = p_root->FirstChildElement(PROFILES)))
    {
        logError(XMLPARSER, "Not found '" << PROFILES << "' tag");
        ret = XMLP_ret::XML_ERROR;
    }
    else
    {
        std::unique_ptr<BaseNode> profiles_node = std::unique_ptr<BaseNode>(new BaseNode{&rootNode, NodeType::ROOT});
        if (XMLP_ret::XML_OK == (ret = parseProfiles(root_child, *profiles_node)))
        {
            rootNode.addChild(std::move(profiles_node));
        }
    }
    return ret;
}

XMLP_ret XMLParser::parseProfiles(XMLElement* p_root, BaseNode& profilesNode)
{
    std::string profile_name = "";
    unsigned int profileCount = 0u;
    XMLElement *p_profile = p_root->FirstChildElement();
    const char *tag = nullptr;
    while (nullptr != p_profile)
    {
        if (nullptr != (tag = p_profile->Value()))
        {
            bool is_default_profile = false;
            p_profile->QueryBoolAttribute("is_default_profile", &is_default_profile);

            // If profile parsing functions fails, log and continue.
            if (strcmp(tag, PARTICIPANT) == 0)
            {
                std::unique_ptr<ParticipantAttributes> participant_atts{new ParticipantAttributes};             
                if (XMLP_ret::XML_OK == parseXMLParticipantProf(p_profile, *participant_atts, profile_name))
                {
                    if(is_default_profile)
                    {
                        default_participant_attributes = *participant_atts;
                    }
                    std::unique_ptr<Node<ParticipantAttributes>> participant_node{new Node<ParticipantAttributes>{&profilesNode, NodeType::PARTICIPANT, std::move(participant_atts)}};
                    if (false == m_participant_profiles.emplace(profile_name, participant_node->getData()).second)
                    {
                        logError(XMLPARSER, "Error adding profile '" << profile_name << "'");
                    }
                    profilesNode.addChild(std::move(participant_node));
                    ++profileCount;
                }
                else
                {
                    logError(XMLPARSER, "Error parsing participant profile");
                }
            }
            else if (strcmp(tag, PUBLISHER) == 0)
            {
                std::unique_ptr<PublisherAttributes> publisher_atts{new PublisherAttributes};
                if (XMLP_ret::XML_OK == parseXMLPublisherProf(p_profile, *publisher_atts, profile_name))
                {
                    if(is_default_profile)
                    {
                        default_publisher_attributes = *publisher_atts;
                    }
                    
                    std::unique_ptr<Node<PublisherAttributes>> publisher_node{new Node<PublisherAttributes>{&profilesNode, NodeType::PUBLISHER, std::move(publisher_atts)}};
                    if (false == m_publisher_profiles.emplace(profile_name, publisher_node->getData()).second)
                    {
                        logError(XMLPARSER, "Error adding profile '" << profile_name << "'");
                    }
                    profilesNode.addChild(std::move(publisher_node));
                    ++profileCount;
                }
                else
                {
                    logError(XMLPARSER, "Error parsing publisher profile");
                }
            }
            else if (strcmp(tag, SUBSCRIBER) == 0)
            {
                std::unique_ptr<SubscriberAttributes> subscriber_atts{new SubscriberAttributes};
                if (XMLP_ret::XML_OK == parseXMLSubscriberProf(p_profile, *subscriber_atts, profile_name))
                {
                    if(is_default_profile)
                    {
                        default_subscriber_attributes = *subscriber_atts;
                    }
                    
                    std::unique_ptr<Node<SubscriberAttributes>> subscriber_node{new Node<SubscriberAttributes>{&profilesNode, NodeType::SUBSCRIBER, std::move(subscriber_atts)}};
                    if (false == m_subscriber_profiles.emplace(profile_name, subscriber_node->getData()).second)
                    {
                        logError(XMLPARSER, "Error adding profile '" << profile_name << "'");
                    }
                    profilesNode.addChild(std::move(subscriber_node));
                    ++profileCount;
                }
                else
                {
                    logError(XMLPARSER, "Error parsing subscriber profile");
                }
            }
            else if (strcmp(tag, QOS_PROFILE))
            {

            }
            else if (strcmp(tag, APPLICATION))
            {

            }
            else if (strcmp(tag, TYPE))
            {

            }
            else if (strcmp(tag, TOPIC))
            {

            }
            else if (strcmp(tag, DATA_WRITER))
            {

            }
            else if (strcmp(tag, DATA_READER))
            {

            }
            else
            {
                logError(XMLPARSER, "Not expected tag: '" << tag << "'");
            }
        }
        p_profile = p_profile->NextSiblingElement();
    }
    return XMLP_ret::XML_OK;
}

XMLP_ret XMLParser::loadXMLFile(const std::string &filename)
{
    if (filename.empty())
    {
        logError(XMLPARSER, "Error loading XML file, filename empty");
        return XMLP_ret::XML_ERROR;
    }

    tinyxml2::XMLDocument xmlDoc;
    if (XML_SUCCESS != xmlDoc.LoadFile(filename.c_str()))
    {
        if (filename != std::string(DEFAULT_FASTRTPS_PROFILES))
            logError(XMLPARSER, "Error opening '" << filename << "'");

        return XMLP_ret::XML_ERROR;
    }

    logInfo(XMLPARSER, "File '" << filename << "' opened successfully");
    return parseXML(xmlDoc);
}

XMLP_ret XMLParser::loadXML(const char* data, size_t length)
{
    tinyxml2::XMLDocument xmlDoc;    
    if (XML_SUCCESS != xmlDoc.Parse(data, length))
    {
        logError(XMLPARSER, "Error parsing XML buffer");
        return XMLP_ret::XML_ERROR;
    }
    return parseXML(xmlDoc);
}

XMLP_ret XMLParser::parseXMLParticipantProf(XMLElement *p_profile,
                                                   ParticipantAttributes &participant_atts,
                                                   std::string &profile_name)
{
    /*<xs:complexType name="rtpsParticipantAttributesType">
      <xs:all minOccurs="0">
        <xs:element name="defaultUnicastLocatorList" type="locatorListType"/>
        <xs:element name="defaultMulticastLocatorList" type="locatorListType"/>
        <xs:element name="defaultOutLocatorList" type="locatorListType"/>
        <xs:element name="defaultSendPort" type="uint32Type"/>
        <xs:element name="sendSocketBufferSize" type="uint32Type"/>
        <xs:element name="listenSocketBufferSize" type="uint32Type"/>
        <xs:element name="builtin" type="builtinAttributesType"/>
        <xs:element name="port" type="portType"/>
        <xs:element name="userData" type="octetVectorType"/>
        <xs:element name="participantID" type="int32Type"/>
        <xs:element name="use_IP4_to_send" type="boolType"/>
        <xs:element name="use_IP6_to_send" type="boolType"/>
        <xs:element name="throughputController" type="throughputControllerType"/>
        <!-- <xs:element name="userTransports" type="XXX"/> -->
        <xs:element name="useBuiltinTransports" type="boolType"/>
        <xs:element name="propertiesPolicy" type="propertyPolicyType"/>
        <xs:element name="name" type="stringType"/>
      </xs:all>
    </xs:complexType>*/

    if (nullptr == p_profile)
    {
        logError(XMLPARSER, "Bad parameters!");
        return XMLP_ret::XML_ERROR;
    }
    const char *prof_name = p_profile->Attribute(PROFILE_NAME);
    if (nullptr == prof_name)
    {
        logError(XMLPARSER, "Not found '" << PROFILE_NAME << "' attribute");
        return XMLP_ret::XML_ERROR;
    }
    profile_name = prof_name;

    XMLElement *p_element = p_profile->FirstChildElement(RTPS);
    if (nullptr == p_element)
    {
        logError(XMLPARSER, "Not found '" << RTPS << "' tag");
        return XMLP_ret::XML_ERROR;
    }

    uint8_t ident = 1;
    XMLElement *p_aux = nullptr;
    // defaultUnicastLocatorList
    if (nullptr != (p_aux = p_element->FirstChildElement(DEF_UNI_LOC_LIST)))
    {
        if (XMLP_ret::XML_OK != getXMLLocatorList(p_aux, participant_atts.rtps.defaultUnicastLocatorList, ident))
            return XMLP_ret::XML_ERROR;
    }
    // defaultMulticastLocatorList
    if (nullptr != (p_aux = p_element->FirstChildElement(DEF_MULTI_LOC_LIST)))
    {
        if (XMLP_ret::XML_OK != getXMLLocatorList(p_aux, participant_atts.rtps.defaultMulticastLocatorList, ident))
            return XMLP_ret::XML_ERROR;
    }
    // defaultOutLocatorList
    if (nullptr != (p_aux = p_element->FirstChildElement(DEF_OUT_LOC_LIST)))
    {
        if (XMLP_ret::XML_OK != getXMLLocatorList(p_aux, participant_atts.rtps.defaultOutLocatorList, ident))
            return XMLP_ret::XML_ERROR;
    }
    // defaultSendPort - uint32Type
    if (nullptr != (p_aux = p_element->FirstChildElement(DEF_SEND_PORT)))
    {
        if (XMLP_ret::XML_OK != getXMLUint(p_aux, &participant_atts.rtps.defaultSendPort, ident))
            return XMLP_ret::XML_ERROR;
    }
    // sendSocketBufferSize - uint32Type
    if (nullptr != (p_aux = p_element->FirstChildElement(SEND_SOCK_BUF_SIZE)))
    {
        if (XMLP_ret::XML_OK != getXMLUint(p_aux, &participant_atts.rtps.sendSocketBufferSize, ident))
            return XMLP_ret::XML_ERROR;
    }
    // listenSocketBufferSize - uint32Type
    if (nullptr != (p_aux = p_element->FirstChildElement(LIST_SOCK_BUF_SIZE)))
    {
        if (XMLP_ret::XML_OK != getXMLUint(p_aux, &participant_atts.rtps.listenSocketBufferSize, ident))
            return XMLP_ret::XML_ERROR;
    }
    // builtin
    if (nullptr != (p_aux = p_element->FirstChildElement(BUILTIN)))
    {
        if (XMLP_ret::XML_OK != getXMLBuiltinAttributes(p_aux, participant_atts.rtps.builtin, ident))
            return XMLP_ret::XML_ERROR;
    }
    // port
    if (nullptr != (p_aux = p_element->FirstChildElement(PORT)))
    {
        if (XMLP_ret::XML_OK != getXMLPortParameters(p_aux, participant_atts.rtps.port, ident))
            return XMLP_ret::XML_ERROR;
    }
    // TODO: userData
    if (nullptr != (p_aux = p_element->FirstChildElement(USER_DATA)))
    {
        if (XMLP_ret::XML_OK != getXMLOctetVector(p_aux, participant_atts.rtps.userData, ident))
            return XMLP_ret::XML_ERROR;
    }
    // participantID - int32Type
    if (nullptr != (p_aux = p_element->FirstChildElement(PART_ID)))
    {
        if (XMLP_ret::XML_OK != getXMLInt(p_aux, &participant_atts.rtps.participantID, ident))
            return XMLP_ret::XML_ERROR;
    }
    // use_IP4_to_send - boolType
    if (nullptr != (p_aux = p_element->FirstChildElement(IP4_TO_SEND)))
    {
        if (XMLP_ret::XML_OK != getXMLBool(p_aux, &participant_atts.rtps.use_IP4_to_send, ident))
            return XMLP_ret::XML_ERROR;
    }
    // use_IP6_to_send - boolType
    if (nullptr != (p_aux = p_element->FirstChildElement(IP6_TO_SEND)))
    {
        if (XMLP_ret::XML_OK != getXMLBool(p_aux, &participant_atts.rtps.use_IP6_to_send, ident))
            return XMLP_ret::XML_ERROR;
    }
    // throughputController
    if (nullptr != (p_aux = p_element->FirstChildElement(THROUGHPUT_CONT)))
    {
        if (XMLP_ret::XML_OK != getXMLThroughputController(p_aux, participant_atts.rtps.throughputController, ident))
            return XMLP_ret::XML_ERROR;
    }
    // TODO: userTransports
    if (nullptr != (p_aux = p_element->FirstChildElement(USER_TRANS)))
    {
        logError(XMLPARSER, "Attribute '" << p_aux->Value() << "' do not supported for now");
    }

    // useBuiltinTransports - boolType
    if (nullptr != (p_aux = p_element->FirstChildElement(USE_BUILTIN_TRANS)))
    {
        if (XMLP_ret::XML_OK != getXMLBool(p_aux, &participant_atts.rtps.useBuiltinTransports, ident))
            return XMLP_ret::XML_ERROR;
    }
    // propertiesPolicy
    if (nullptr != (p_aux = p_element->FirstChildElement(PROPERTIES_POLICY)))
    {
        if (XMLP_ret::XML_OK != getXMLPropertiesPolicy(p_aux, participant_atts.rtps.properties, ident))
            return XMLP_ret::XML_ERROR;
    }
    // name - stringType
    if (nullptr != (p_aux = p_element->FirstChildElement(NAME)))
    {
        std::string s = "";
        if (XMLP_ret::XML_OK != getXMLString(p_aux, &s, ident)) return XMLP_ret::XML_ERROR;
        participant_atts.rtps.setName(s.c_str());
    }
    return XMLP_ret::XML_OK;
}

XMLP_ret XMLParser::parseXMLPublisherProf(XMLElement *p_profile,
                                                 PublisherAttributes &publisher_atts,
                                                 std::string &profile_name)
{
    /*<xs:complexType name="publisherProfileType">
      <xs:all minOccurs="0">
        <xs:element name="topic" type="topicAttributesType"/>
        <xs:element name="qos" type="writerQosPoliciesType"/>
        <xs:element name="times" type="writerTimesType"/>
        <xs:element name="unicastLocatorList" type="locatorListType"/>
        <xs:element name="multicastLocatorList" type="locatorListType"/>
        <xs:element name="outLocatorList" type="locatorListType"/>
        <xs:element name="throughputController" type="throughputControllerType"/>
        <xs:element name="historyMemoryPolicy" type="historyMemoryPolicyType"/>
        <xs:element name="userDefinedID" type="int16Type"/>
        <xs:element name="entityID" type="int16Type"/>
      </xs:all>
      <xs:attribute name="profile_name" type="stringType" use="required"/>
    </xs:complexType>*/

    if (nullptr == p_profile)
    {
        logError(XMLPARSER, "Bad parameters!");
        return XMLP_ret::XML_ERROR;
    }
    const char *prof_name = p_profile->Attribute(PROFILE_NAME);
    if (nullptr == prof_name)
    {
        logError(XMLPARSER, "Not found '" << PROFILE_NAME << "' attribute");
        return XMLP_ret::XML_ERROR;
    }
    profile_name = prof_name;

    uint8_t ident = 1;
    XMLElement *p_aux = nullptr;
    // topic
    if (nullptr != (p_aux = p_profile->FirstChildElement(TOPIC)))
    {
        if (XMLP_ret::XML_OK != getXMLTopicAttributes(p_aux, publisher_atts.topic, ident))
            return XMLP_ret::XML_ERROR;
    }
    // qos
    if (nullptr != (p_aux = p_profile->FirstChildElement(QOS)))
    {
        if (XMLP_ret::XML_OK != getXMLWriterQosPolicies(p_aux, publisher_atts.qos, ident))
            return XMLP_ret::XML_ERROR;
    }
    // times
    if (nullptr != (p_aux = p_profile->FirstChildElement(TIMES)))
    {
        if (XMLP_ret::XML_OK != getXMLWriterTimes(p_aux, publisher_atts.times, ident))
            return XMLP_ret::XML_ERROR;
    }
    // unicastLocatorList
    if (nullptr != (p_aux = p_profile->FirstChildElement(UNI_LOC_LIST)))
    {
        if (XMLP_ret::XML_OK != getXMLLocatorList(p_aux, publisher_atts.unicastLocatorList, ident))
            return XMLP_ret::XML_ERROR;
    }
    // multicastLocatorList
    if (nullptr != (p_aux = p_profile->FirstChildElement(MULTI_LOC_LIST)))
    {
        if (XMLP_ret::XML_OK != getXMLLocatorList(p_aux, publisher_atts.multicastLocatorList, ident))
            return XMLP_ret::XML_ERROR;
    }
    // outLocatorList
    if (nullptr != (p_aux = p_profile->FirstChildElement(OUT_LOC_LIST)))
    {
        if (XMLP_ret::XML_OK != getXMLLocatorList(p_aux, publisher_atts.outLocatorList, ident))
            return XMLP_ret::XML_ERROR;
    }
    // throughputController
    if (nullptr != (p_aux = p_profile->FirstChildElement(THROUGHPUT_CONT)))
    {
        if (XMLP_ret::XML_OK != getXMLThroughputController(p_aux, publisher_atts.throughputController, ident))
            return XMLP_ret::XML_ERROR;
    }
    // historyMemoryPolicy
    if (nullptr != (p_aux = p_profile->FirstChildElement(HIST_MEM_POLICY)))
    {
        if (XMLP_ret::XML_OK != getXMLHistoryMemoryPolicy(p_aux, publisher_atts.historyMemoryPolicy, ident))
            return XMLP_ret::XML_ERROR;
    }
    // propertiesPolicy
    if (nullptr != (p_aux = p_profile->FirstChildElement(PROPERTIES_POLICY)))
    {
        if (XMLP_ret::XML_OK != getXMLPropertiesPolicy(p_aux, publisher_atts.properties, ident))
            return XMLP_ret::XML_ERROR;
    }
    // userDefinedID - int16type
    if (nullptr != (p_aux = p_profile->FirstChildElement(USER_DEF_ID)))
    {
        int i = 0;
        if (XMLP_ret::XML_OK != getXMLInt(p_aux, &i, ident) || i > 255) return XMLP_ret::XML_ERROR;
        publisher_atts.setUserDefinedID(static_cast<uint8_t>(i));
    }
    // entityID - int16Type
    if (nullptr != (p_aux = p_profile->FirstChildElement(ENTITY_ID)))
    {
        int i = 0;
        if (XMLP_ret::XML_OK != getXMLInt(p_aux, &i, ident) || i > 255) return XMLP_ret::XML_ERROR;
        publisher_atts.setEntityID(static_cast<uint8_t>(i));
    }
    return XMLP_ret::XML_OK;
}

XMLP_ret XMLParser::parseXMLSubscriberProf(XMLElement *p_profile,
                                                  SubscriberAttributes &subscriber_atts,
                                                  std::string &profile_name)
{
    /*<xs:complexType name="subscriberProfileType">
      <xs:all minOccurs="0">
        <xs:element name="topic" type="topicAttributesType"/>
        <xs:element name="qos" type="readerQosPoliciesType"/>
        <xs:element name="times" type="readerTimesType"/>
        <xs:element name="unicastLocatorList" type="locatorListType"/>
        <xs:element name="multicastLocatorList" type="locatorListType"/>
        <xs:element name="outLocatorList" type="locatorListType"/>
        <xs:element name="expectsInlineQos" type="boolType"/>
        <xs:element name="historyMemoryPolicy" type="historyMemoryPolicyType"/>
        <xs:element name="propertiesPolicy" type="propertyPolicyType"/>
        <xs:element name="userDefinedID" type="int16Type"/>
        <xs:element name="entityID" type="int16Type"/>
      </xs:all>
      <xs:attribute name="profile_name" type="stringType" use="required"/>
    </xs:complexType>*/

    if (nullptr == p_profile)
    {
        logError(XMLPARSER, "Bad parameters!");
        return XMLP_ret::XML_ERROR;
    }
    const char *prof_name = p_profile->Attribute(PROFILE_NAME);
    if (nullptr == prof_name)
    {
        logError(XMLPARSER, "Not found '" << PROFILE_NAME << "' attribute");
        return XMLP_ret::XML_ERROR;
    }
    profile_name = prof_name;

    uint8_t ident = 1;
    XMLElement *p_aux = nullptr;
    // topic
    if (nullptr != (p_aux = p_profile->FirstChildElement(TOPIC)))
    {
        if (XMLP_ret::XML_OK != getXMLTopicAttributes(p_aux, subscriber_atts.topic, ident))
            return XMLP_ret::XML_ERROR;
    }
    // qos
    if (nullptr != (p_aux = p_profile->FirstChildElement(QOS)))
    {
        if (XMLP_ret::XML_OK != getXMLReaderQosPolicies(p_aux, subscriber_atts.qos, ident))
            return XMLP_ret::XML_ERROR;
    }
    // times
    if (nullptr != (p_aux = p_profile->FirstChildElement(TIMES)))
    {
        if (XMLP_ret::XML_OK != getXMLReaderTimes(p_aux, subscriber_atts.times, ident))
            return XMLP_ret::XML_ERROR;
    }
    // unicastLocatorList
    if (nullptr != (p_aux = p_profile->FirstChildElement(UNI_LOC_LIST)))
    {
        if (XMLP_ret::XML_OK != getXMLLocatorList(p_aux, subscriber_atts.unicastLocatorList, ident))
            return XMLP_ret::XML_ERROR;
    }
    // multicastLocatorList
    if (nullptr != (p_aux = p_profile->FirstChildElement(MULTI_LOC_LIST)))
    {
        if (XMLP_ret::XML_OK != getXMLLocatorList(p_aux, subscriber_atts.multicastLocatorList, ident))
            return XMLP_ret::XML_ERROR;
    }
    // outLocatorList
    if (nullptr != (p_aux = p_profile->FirstChildElement(OUT_LOC_LIST)))
    {
        if (XMLP_ret::XML_OK != getXMLLocatorList(p_aux, subscriber_atts.outLocatorList, ident))
            return XMLP_ret::XML_ERROR;
    }
    // expectsInlineQos - boolType
    if (nullptr != (p_aux = p_profile->FirstChildElement(EXP_INLINE_QOS)))
    {
        if (XMLP_ret::XML_OK != getXMLBool(p_aux, &subscriber_atts.expectsInlineQos, ident))
            return XMLP_ret::XML_ERROR;
    }
    // historyMemoryPolicy
    if (nullptr != (p_aux = p_profile->FirstChildElement(HIST_MEM_POLICY)))
    {
        if (XMLP_ret::XML_OK != getXMLHistoryMemoryPolicy(p_aux, subscriber_atts.historyMemoryPolicy, ident))
            return XMLP_ret::XML_ERROR;
    }
    // propertiesPolicy
    if (nullptr != (p_aux = p_profile->FirstChildElement(PROPERTIES_POLICY)))
    {
        if (XMLP_ret::XML_OK != getXMLPropertiesPolicy(p_aux, subscriber_atts.properties, ident))
            return XMLP_ret::XML_ERROR;
    }
    // userDefinedID - int16Type
    if (nullptr != (p_aux = p_profile->FirstChildElement(USER_DEF_ID)))
    {
        int i = 0;
        if (XMLP_ret::XML_OK != getXMLInt(p_aux, &i, ident) || i > 255) return XMLP_ret::XML_ERROR;
        subscriber_atts.setUserDefinedID(static_cast<uint8_t>(i));
    }
    // entityID - int16Type
    if (nullptr != (p_aux = p_profile->FirstChildElement(ENTITY_ID)))
    {
        int i = 0;
        if (XMLP_ret::XML_OK != getXMLInt(p_aux, &i, ident) || i > 255) return XMLP_ret::XML_ERROR;
        subscriber_atts.setEntityID(static_cast<uint8_t>(i));
    }
    return XMLP_ret::XML_OK;
}

} /* xmlparser  */
} /* namespace  */
} /* namespace eprosima */
