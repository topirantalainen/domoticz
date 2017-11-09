/*
 * misc.hpp
 *
 *  Created on: 5. nov. 2017
 *      Author: Jonas Bo Jalling
 */

#ifndef HARDWARE_IHC_MISC_HPP_
#define HARDWARE_IHC_MISC_HPP_
static const std::string emptyQuery = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\"><soapenv:Body></soapenv:Body></soapenv:Envelope>";
std::string url = "http://192.168.1.99/ws/AuthenticationService";

#include "../../httpclient/HTTPClient.h"

    std::string sendQuery(std::string httpUrl, std::string query, std::string action = "")
    {

        std::string result;
        std::vector < std::string > ExtraHeaders;
        if (action.length())
        {
            ExtraHeaders.push_back("SOAPAction: \"" + action + "\"");
        }

        ExtraHeaders.push_back("Content-Type: text/xml; charset=UTF-8");
        HTTPClient::SetTimeout(8);

        bool ret = HTTPClient::POST(httpUrl, query, ExtraHeaders, result);
        if (!ret) {
            throw std::runtime_error("Seems like the IHC controller has crashed...");//std::logic_error("LK IHC: Error connecting to Server...");
        }
        return result;
    }
    void parseValue(TiXmlDocument const doc, std::string const xpath, std::string& result)
    {
        std::string xp = xpath + "/text()"; // <-- Fixing what I think is a bug in tinyXpath
        TinyXPath::xpath_processor processor(doc.RootElement(), xp.c_str());
        TiXmlString xresult = processor.S_compute_xpath();
        result = xresult.c_str();
    }

    void parseValue(TiXmlDocument const doc, std::string const xpath, bool& result)
    {
        //bool result;

        std::string xresult;
        parseValue(doc, xpath, xresult);
        std::istringstream is(xresult);
        is >> std::boolalpha >> result;
        //return result;
    }


    void getValue(std::string const location, std::string const xpath,  int & result)
        {
            std::string sResult;
            sResult = sendQuery(url, emptyQuery, location);
            TiXmlDocument doc;
            doc.Parse(sResult.c_str());

            std::string stringValue;
            parseValue(doc, xpath, stringValue);
            result = boost::lexical_cast<int>(stringValue);
        }


#endif /* HARDWARE_IHC_MISC_HPP_ */
