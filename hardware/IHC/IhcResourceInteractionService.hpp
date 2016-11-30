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
	std::cout << __func__ << std::endl;
	url = "http://" + hostname + "/ws/ResourceInteractionService";
}


/*
std::unique_ptr<WSResourceValue> resourceQuery(int resourceId)
{
	std::stringstream sstr;
	std::string httpData = sstr.str();

	TiXmlDocument doc;

	//std::string httpUrl(m_address + ":" + std::to_string(m_portnr) + "/ws/ResourceInteractionService");
	std::string sResult;

	std::string query = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	query += "<soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\"><soapenv:Body>";
	query += " <ns1:getRuntimeValue1 xmlns:ns1=\"utcs\">" + std::to_string(resourceId) + "</ns1:getRuntimeValue1></soapenv:Body>";
	query += "</soapenv:Envelope>";

	sResult = sendQuery(url, query);
	if (doc.Parse(sResult.c_str()))
	{
		TinyXPath::xpath_processor processor ( doc.RootElement(), "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:getRuntimeValue2");
		if (1 == processor.u_compute_xpath_node_set())
		{
			TiXmlNode* sss = processor.XNp_get_xpath_node(0);
			TiXmlElement * a = sss->ToElement();

			auto val = parseResourceValue(a->FirstChild()->ToElement(), 2);

			if (val->getResourceID() == resourceId)
			{
				return val;
			}
			else
			{
				std::cout << "No resource id found" << std::endl;
			}
		}
		else
			std::cout << "No resource value found" << std::endl;
	}
}

*/

ResourceValue resourceQuery(int resourceId)
{
	std::cout << __func__ << std::endl;

	std::stringstream sstr;
		std::string httpData = sstr.str();


		TiXmlDocument doc;

		//std::string httpUrl(m_address + ":" + std::to_string(m_portnr) + "/ws/ResourceInteractionService");
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
					doc.Print();
					std::cout << "printing bool" << std::endl;
					val.value =  (!strcmp(sss->FirstChildElement("ns1:value")->FirstChildElement("ns2:value")->GetText(), "true"));
					return val;
				}
				else
				{
					std::cout << "Unknown type: (" << sss->FirstChildElement("ns1:value")->Value() << ")" << std::endl;
					doc.Print();
				}
			}
			else
			{
				std::cout << "No resource value found" << std::endl;

			}
		}
	}


std::string getValue( TiXmlElement* a, std::string t)
{
	TinyXPath::xpath_processor proc(a, t.c_str());

	if (1 == proc.u_compute_xpath_node_set())
	{
		TiXmlNode* thisNode = proc.XNp_get_xpath_node(0);

		std::string res = thisNode->ToElement()->GetText();

		return res;
	}
	return "";
}


/*
bool resourceUpdate(WSBooleanValue value)
{
	std::cout << __func__ << std::endl;
	std::string status;
	status = value.isValue() ? "true" : "false";
	std::string query = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	query += "<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">";
	query +=  "<soap:Body><setResourceValue1 xmlns=\"utcs\">";
	query +=  "  <value xmlns:q1=\"utcs.values\" xsi:type=\"q1:WSBooleanValue\"><q1:value>" + status + "</q1:value>";
	query +=  "  </value><resourceID>" + boost::to_string(value.getResourceID()) + "</resourceID><isValueRuntime>true</isValueRuntime>";
	query +=  " </setResourceValue1></soap:Body></soap:Envelope>";

	return doResourceUpdate(query);
}*/

bool resourceUpdate2(ResourceValue const value)
{
	std::cout << __func__ << std::endl;
	return doResourceUpdate2(value);
}
/*
bool resourceUpdate(WSIntegerValue value)
{
	std::cout << __func__ << std::endl;
	std::string query = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	query += "<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">";
	query += "<soap:Body><setResourceValue1 xmlns=\"utcs\">";
	query += "  <value xmlns:q1=\"utcs.values\" xsi:type=\"q1:WSIntegerValue\">";
	query += "   <q1:maximumValue>" + boost::to_string(value.getMaxValue()) + "</q1:maximumValue><q1:minimumValue>" + boost::to_string(value.getMinValue()) + "</q1:minimumValue>";
	query += "   <q1:integer>" + boost::to_string(value.getInteger()) + "</q1:integer></value><resourceID>" + boost::to_string(value.getResourceID()) + "</resourceID>";
	query += "  <isValueRuntime>true</isValueRuntime></setResourceValue1></soap:Body>";
	query += "</soap:Envelope>";

	return doResourceUpdate(query);
}*/


private:

bool doResourceUpdate2(ResourceValue const query)
{
	std::cout << __func__ << std::endl;
    TiXmlDocument doc;
    std::cout << "--aadd----------------\n";
    std::cout << query.toXml() << std::endl;
    std::cout << "--ddaa----------------\n";
    std::string const sResult = sendQuery(url, query.toXml());
    doc.Parse(sResult.c_str());
    return WSLoginResult::parseValueToBoolean(doc, "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:setResourceValue2/text()");

}
bool doResourceUpdate(std::string const query)
{
	std::cout << __func__ << std::endl;
    TiXmlDocument doc;

    std::string const sResult = sendQuery(url, query);
    doc.Parse(sResult.c_str());
    return WSLoginResult::parseValueToBoolean(doc, "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:setResourceValue2/text()");

}

boost::shared_ptr<ResourceValue> parseResourceValue2(TiXmlElement* xmlRes, int const index)
{
	std::cout << __func__ << std::endl;

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

		std::cout << " None found\n";
		//Todo: throw unknown type
	}


/*
	TiXmlElement * pxmlChild = pxmlParent->FirstChildElement( "child" );
	std::stringstream ss;*/
	std::cout << (xmlRes->Value());

	//xmlRes->ToDocument()->Print();
	std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
			boost::shared_ptr<ResourceValue> r = boost::shared_ptr<ResourceValue>(new ResourceValue(123));
			r->value = false;
	return r;
}
/*
boost::shared_ptr<WSResourceValue> parseResourceValue(TiXmlElement* xmlRes, int const index)
{
	std::cout << __func__ << std::endl;
	std::string const resourceId = getValue(xmlRes, "/ns1:resourceID");
	if (!resourceId.empty())
	{
		auto const id = boost::lexical_cast<int>(resourceId);

		auto const boolVal = getValue(xmlRes, "/ns1:value/ns" + boost::to_string(index) + ":value");
		if (boolVal.size() > 0)
		{
			boost::shared_ptr<WSBooleanValue>  var = boost::shared_ptr<WSBooleanValue>(new WSBooleanValue);
			var->setResourceID(id);
			(strcmp(boolVal.c_str(), "true") == 0) ? var->setValue(true) : var->setValue(false);
			return std::move(var);
		}

		auto const intVal = getValue(xmlRes, "/ns1:value/ns" + boost::to_string(index) + ":integer");
		if (intVal.size() > 0)
		{
			auto  var = std::unique_ptr<WSIntegerValue>(new WSIntegerValue);
			var->setResourceID(id);
			var->setInteger(boost::lexical_cast<int>(intVal));

			auto const maxVal = getValue(xmlRes, "/ns1:value/ns" + boost::to_string(index) + ":maximumValue");
			if (maxVal.size() > 0)
			{
				var->setMaxValue(boost::lexical_cast<int>(maxVal));
			}

			auto const minVal = getValue(xmlRes, "/ns1:value/ns" + boost::to_string(index) + ":minimumValue");
			if (minVal.size() > 0)
			{
				var->setMinValue(boost::lexical_cast<int>(minVal));
			}

			return std::move(var);
		}

		std::cout << " None found\n";
		//Todo: throw unknown type
	}
	std::cout << " None found\n";
}
*/
public:
void enableRuntimeValueNotifications(std::vector <int> items)
{
	std::cout << __func__ << "b" << std::endl;
	std::string queryPrefix = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	queryPrefix += "<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">";
	queryPrefix += "<soap:Body><enableRuntimeValueNotifications1 xmlns=\"utcs\">";

	std::string const querySuffix = "</enableRuntimeValueNotifications1></soap:Body></soap:Envelope>";

	std::string query = queryPrefix;
std::cout << "here\n";
	for (std::vector<int>::const_iterator itt = items.begin(); itt != items.end(); ++itt)
	{
	//for( auto&& pointer : items ) {
		query += "<xsd:arrayItem>" + boost::to_string(*itt) + "</xsd:arrayItem>";
	}

	query += querySuffix;

	std::string const sResult = sendQuery(url, query);
}
void enableRuntimeValueNotifications2(std::vector<int> items)
{
	std::cout << __func__ << std::endl;
	std::cout << "her \n";
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
std::cout << "her2\n";
	TiXmlDocument doc;

	std::string sResult;
	sResult = sendQuery(url, query);
	std::cout << "enabling runtime notifications: " << sResult<<std::endl;
}


std::vector<boost::shared_ptr<ResourceValue> > waitResourceValueNotifications2(int const timeoutInSeconds)
{
	std::cout << "START: " << __func__ << std::endl;

	std::string query = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	query += "<soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:utcs=\"utcs\">";
	query += "<soapenv:Header/><soapenv:Body>";
	query += "<utcs:waitForResourceValueChanges1>" + boost::to_string(timeoutInSeconds) + "</utcs:waitForResourceValueChanges1></soapenv:Body>";
	query += "</soapenv:Envelope>";

	std::string sResult;
	// TODO: Dont load from file
	sResult = sendQuery(url, query);

	TiXmlDocument doc;
	//doc.LoadFile("multipleResponse.xml");
    doc.Parse(sResult.c_str());
    //doc.Print();
    std::vector<boost::shared_ptr<ResourceValue> > resourceList;

    TinyXPath::xpath_processor processor ( doc.RootElement(), "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:waitForResourceValueChanges2/ns1:arrayItem");
    int n = processor.u_compute_xpath_node_set(); // <-- this is important. It executes the Xpath expression
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
			resourceList.push_back(parseResourceValue2(res, i + 2));
		}
	}
    std::cout << "END: " << __func__ << std::endl;
    return resourceList;
}
/*
std::vector<boost::shared_ptr<WSResourceValue> > waitResourceValueNotifications(int const timeoutInSeconds)
{
	std::cout << __func__ << std::endl;

	std::string query = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
	query += "<soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:utcs=\"utcs\">";
	query += "<soapenv:Header/><soapenv:Body>";
	query += "<utcs:waitForResourceValueChanges1>" + boost::to_string(timeoutInSeconds) + "</utcs:waitForResourceValueChanges1></soapenv:Body>";
	query += "</soapenv:Envelope>";
	std::string sResult;
	sResult = sendQuery(url, query);

	TiXmlDocument doc;
    doc.Parse(sResult.c_str());
    doc.Print();
    std::vector<boost::shared_ptr<WSResourceValue> > resourceList;

    TinyXPath::xpath_processor processor ( doc.RootElement(), "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:waitForResourceValueChanges2/ns1:arrayItem");
    int n = processor.u_compute_xpath_node_set(); // <-- this is important. It executes the Xpath expression

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

    return resourceList;
}
*/
};

#endif /* IHC_IHCRESOURCEINTERACTIONSERVICE_HPP_ */
