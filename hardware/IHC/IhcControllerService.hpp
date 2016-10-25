/*
 * IhcControllerService.hpp
 *
 *  Created on: 1. okt. 2016
 *      Author: Jonas Bo Jalling
 */

#ifndef IHC_IHCCONTROLLERSERVICE_HPP_
#define IHC_IHCCONTROLLERSERVICE_HPP_

#include "datatypes/WSBaseDataType.hpp"

static const std::string emptyQuery = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\"><soapenv:Body></soapenv:Body></soapenv:Envelope>";
class IhcControllerService : public IhcHttpClient
{
private:
	std::string url;
	//static const std::string emptyQuery;// = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\"><soapenv:Body></soapenv:Body></soapenv:Envelope>";

public:
	IhcControllerService(std::string hostname)
	{
		url = "https://" + hostname + "/ws/ControllerService";
	}

public:

	WSProjectInfo getProjectInfo()
	{
		std::cout << "her1" << std::endl;
		TiXmlDocument doc;
		WSProjectInfo projectInfo;
		std::string sResult;
		std::cout << "her2" << std::endl;
		sResult = sendQuery(url, emptyQuery, "getProjectInfo");
		std::cout << "her3" << std::endl;
		doc.Parse(sResult.c_str());
		std::cout << "her4" << std::endl;
		projectInfo.encodeData(doc);
		std::cout << "her5" << std::endl;
		std::cout << "JH" << std::endl;
		return projectInfo;
	}

	int getProjectNumberOfSegments()
	{
		std::string sResult;
		sResult = sendQuery(url, emptyQuery, "getIHCProjectNumberOfSegments");
		TiXmlDocument doc;
		doc.Parse(sResult.c_str());

		std::string numberOfSegments = WSBaseDataType::parseValue(doc, "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:getIHCProjectNumberOfSegments1");
		std::cout << "Segments: " <<  numberOfSegments << std::endl;
		return boost::lexical_cast<int>(numberOfSegments);
	}

	int getProjectSegmentationSize()
	{
		std::string sResult;
		sResult = sendQuery(url, emptyQuery, "getIHCProjectSegmentationSize");
		TiXmlDocument doc;
		doc.Parse(sResult.c_str());

		std::string segmentationSize = WSBaseDataType::parseValue(doc, "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:getIHCProjectSegmentationSize1");
		std::cout << "SegmentSize: " <<  segmentationSize << std::endl;
		return boost::lexical_cast<int>(segmentationSize);
    }

	WSFile getProjectSegment(int index, int major, int minor)
	{
		std::string query = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
		query += "<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">";
		query += "<soap:Body>";
		query += "<ns1:getIHCProjectSegment1 xmlns:ns1=\"utcs\" xsi:type=\"xsd:int\">" + boost::to_string(index) + "</ns1:getIHCProjectSegment1>";
		query += "<ns2:getIHCProjectSegment2 xmlns:ns2=\"utcs\" xsi:type=\"xsd:int\">" + boost::to_string(major) + "</ns2:getIHCProjectSegment2>";
		query += "<ns3:getIHCProjectSegment3 xmlns:ns3=\"utcs\" xsi:type=\"xsd:int\">" + boost::to_string(minor) + "</ns3:getIHCProjectSegment3>";
		query += "</soap:Body></soap:Envelope>";

		std::string sResult;
		TiXmlDocument doc;
		sResult = sendQuery(url, query, "getIHCProjectSegment");
		doc.Parse(sResult.c_str());

		WSFile file;// = new WSFile();
		file.encodeData(doc);
		return file;
	}

	/**
	 * Query controller current state.
	 *
	 * @return controller's current state.
	 */
	WSControllerState getControllerState()
	{
		std::string sResult;
		TiXmlDocument doc;

		sResult = sendQuery(url, emptyQuery, "getState");
		doc.Parse(sResult.c_str());

		WSControllerState controllerState;
		controllerState.encodeData(doc);
		std::cout << controllerState.getState() << std::endl;

		return controllerState;
	}

	void getRF()
		{
		std::string url2 = url = "https://192.168.1.99/ws/AirlinkManagementService";
			std::string sResult;
			TiXmlDocument doc;

			sResult = sendQuery(url, emptyQuery, "getDetectedDeviceList");
			doc.Parse(sResult.c_str());
			doc.Print();
/*
			WSControllerState controllerState;
			controllerState.encodeData(doc);
			std::cout << controllerState.getState() << std::endl;
*/

		}


};

#endif /* IHC_IHCCONTROLLERSERVICE_HPP_ */
