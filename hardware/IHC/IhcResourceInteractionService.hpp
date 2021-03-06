/*
 * IhcResourceInteractionService.hpp
 *
 *  Created on: 30. sep. 2016
 *      Author: Jonas Bo Jalling
 */

#ifndef IHC_IHCRESOURCEINTERACTIONSERVICE_HPP_
#define IHC_IHCRESOURCEINTERACTIONSERVICE_HPP_

#include <memory>
#include <string>
#include "datatypes/WSResourceValue.hpp"
#include <boost/variant.hpp>
#include <iostream>
#include <memory>
class IhcResourceInteractionService : public IhcHttpClient
{
private:
	std::string url;

public:
IhcResourceInteractionService(std::string hostname)
{
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);
	url = "http://" + hostname + "/ws/ResourceInteractionService";
}

ResourceValue resourceQuery(int resourceId)
{
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);

	std::stringstream sstr;
    std::string httpData = sstr.str();

    TiXmlDocument doc;

    std::string sResult;

    std::string query = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
    query += "<soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\"><soapenv:Body>";
    query += " <ns1:getRuntimeValue1 xmlns:ns1=\"utcs\">" + boost::to_string(resourceId) + "</ns1:getRuntimeValue1></soapenv:Body>";
    query += "</soapenv:Envelope>";

    sResult = sendQuery(url, query);

    if (doc.Parse(sResult.c_str()))
    {

        TinyXPath::xpath_processor processor ( doc.RootElement(), "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:getRuntimeValue2");
        if (1 == processor.u_compute_xpath_node_set())
        {
            TiXmlNode* sss = processor.XNp_get_xpath_node(0);

            ResourceValue val(boost::lexical_cast<int>( sss-> FirstChildElement("ns1:resourceID")->GetText()));

            if (!strcmp(sss->FirstChildElement("ns1:value")->Attribute("xsi:type"), "ns2:WSIntegerValue"))
            {

                val.value = RangedInteger(boost::lexical_cast<int>(sss->FirstChildElement("ns1:value")->FirstChildElement("ns2:integer")->GetText()),
                boost::lexical_cast<int>(sss->FirstChildElement("ns1:value")->FirstChildElement("ns2:minimumValue")->GetText()),
                boost::lexical_cast<int>(sss->FirstChildElement("ns1:value")->FirstChildElement("ns2:maximumValue")->GetText()));
                return val;
            }
            if (!strcmp(sss->FirstChildElement("ns1:value")->Attribute("xsi:type"), "ns2:WSBooleanValue"))
            {
                //doc.Print();
                val.value =  (!strcmp(sss->FirstChildElement("ns1:value")->FirstChildElement("ns2:value")->GetText(), "true"));
                return val;
            }
            else
            {
                //std::cout << "Unknown type: (" << sss->FirstChildElement("ns1:value")->Value() << ")" << std::endl;
                //doc.Print();
            }
        }
        else
        {
            //std::cout << "No resource value found" << std::endl;

        }
    }
}


std::string getValue( TiXmlElement* a, std::string t)
{
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);
	TinyXPath::xpath_processor proc(a, t.c_str());

	if (1 == proc.u_compute_xpath_node_set())
	{
		TiXmlNode* thisNode = proc.XNp_get_xpath_node(0);

		std::string res = thisNode->ToElement()->GetText();

		return res;
	}
	return "";
}

bool resourceUpdate(ResourceValue const value)
{
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);
	return doResourceUpdate(value);
}

private:

bool doResourceUpdate(ResourceValue const &query)
{
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);
	TiXmlDocument doc;
#ifdef _DEBUG
    std::cout << "------------------\n";
    std::cout << query.toXml() << std::endl;
    std::cout << "------------------\n";
#endif
    std::string const sResult = sendQuery(url, query.toXml());
    doc.Parse(sResult.c_str());
    return WSLoginResult::parseValueToBoolean(doc, "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:setResourceValue2");

}

boost::shared_ptr<ResourceValue> parseResourceValue(TiXmlElement* xmlRes, int const index)
{
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);
	std::string resourceID = getValue(xmlRes, "/ns1:resourceID");

	if (!resourceID.empty())
	{
		int const id = boost::lexical_cast<int>(resourceID);

		boost::shared_ptr<ResourceValue>  var = boost::shared_ptr<ResourceValue>(new ResourceValue(id));

		std::string const boolVal = getValue(xmlRes, "/ns1:value/ns" + boost::to_string(index) + ":value");
		if (boolVal.size() > 0)
		{
			(strcmp(boolVal.c_str(), "true") == 0) ? var->value = true : var->value = false;
			return boost::move(var);
		}

		std::string const intVal = getValue(xmlRes, "/ns1:value/ns" + boost::to_string(index) + ":integer");
		if (intVal.size() > 0)
		{
			var->value = RangedInteger(boost::lexical_cast<int>(intVal),
					boost::lexical_cast<int>(getValue(xmlRes, "/ns1:value/ns" + boost::to_string(index) + ":maximumValue")),
					boost::lexical_cast<int>(getValue(xmlRes, "/ns1:value/ns" + boost::to_string(index) + ":minimumValue")));
							//	return val;

			return boost::move(var);
		}

		//Todo: throw unknown type
	}

	boost::shared_ptr<ResourceValue> r = boost::shared_ptr<ResourceValue>(new ResourceValue(123));
	r->value = false;
	return r;
}

public:

void enableRuntimeValueNotifications(std::vector<int> &items)
{
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);
	std::string queryPrefix = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	queryPrefix += "<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">";
	queryPrefix += "<soap:Body><enableRuntimeValueNotifications1 xmlns=\"utcs\">";

	std::string querySuffix = "</enableRuntimeValueNotifications1></soap:Body></soap:Envelope>";

	std::string query = queryPrefix;
	for (std::vector<int>::iterator it = items.begin(); it != items.end(); ++it)
	{
		query += "<xsd:arrayItem>" + boost::to_string(*it) + "</xsd:arrayItem>";
	}

	query += querySuffix;

	std::string sResult;
	sResult = sendQuery(url, query);
}


std::vector<boost::shared_ptr<ResourceValue> > waitResourceValueNotifications(int const timeoutInSeconds)
{
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);
	std::string query = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	query += "<soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:utcs=\"utcs\">";
	query += "<soapenv:Header/><soapenv:Body>";
	query += "<utcs:waitForResourceValueChanges1>" + boost::to_string(timeoutInSeconds) + "</utcs:waitForResourceValueChanges1></soapenv:Body>";
	query += "</soapenv:Envelope>";

	std::string sResult;

	sResult = sendQuery(url, query);

	TiXmlDocument doc;
    doc.Parse(sResult.c_str());
#ifdef _DEBUG
    doc.Print();
#endif
    _log.Log(LOG_STATUS,"IHC_RESULT: %s", sResult.c_str());
    std::vector<boost::shared_ptr<ResourceValue> > resourceList;

    TinyXPath::xpath_processor processor ( doc.RootElement(), "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:waitForResourceValueChanges2/ns1:arrayItem");
    int n = processor.u_compute_xpath_node_set(); // <-- this is important. It executes the Xpath expression
    if (n > 0)
    {
        if (processor.XNp_get_xpath_node(0)->FirstChild() == 0)
	{
    	// Timed out, return empty list
		return resourceList;
	}
	else
	{
		for (int i = 0; i < processor.u_compute_xpath_node_set(); i++)
		{
			TiXmlNode* thisNode = processor.XNp_get_xpath_node(i);

			TiXmlElement * res = thisNode->FirstChild()->ToElement();
			resourceList.push_back(parseResourceValue(res, i + 2));
		}
	}
    }
    else
    {
        _log.Log(LOG_STATUS,"IHC_RESULT: Controller crash!? ---------------------------------");
        throw std::runtime_error("Empty result");
    }

    return resourceList;
}

};

#endif /* IHC_IHCRESOURCEINTERACTIONSERVICE_HPP_ */
