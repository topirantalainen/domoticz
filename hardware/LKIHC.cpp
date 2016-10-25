#include "stdafx.h"
#include "LKIHC.h"
#include "../main/Helper.h"
#include "../main/Logger.h"
#include "../main/localtime_r.h"
#include "../main/SQLHelper.h"
#include "../main/mainworker.h"
#include "../main/WebServer.h"
#include "../webserver/cWebem.h"
#include "../httpclient/HTTPClient.h"
#include "hardwaretypes.h"
#include "../tinyxpath/tinyxml.h"
#include "../tinyxpath/xpath_static.h"
#include <boost/shared_ptr.hpp>
#include "IHC/IhcClient.hpp"
#include <string.h>
#include "../json/json.h"

CLKIHC::CLKIHC(const int ID, const std::string &IPAddress, const unsigned short Port, const std::string &Username, const std::string &Password) :
m_IPAddress(IPAddress),
m_UserName(Username),
m_Password(Password)
{
	m_HwdID=ID;
	m_Port = Port;
	m_stoprequested=false;
	Init();
}

CLKIHC::~CLKIHC(void)
{
}


void CLKIHC::Init()
{
;;
}
ihcClient* ihcC;

bool CLKIHC::StartHardware()
{
	Init();
	//Start worker thread
	m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CLKIHC::Do_Work, this)));
	m_bIsStarted=true;
	sOnConnected(this);
	ihcC = new ihcClient(m_IPAddress, m_UserName, m_Password);
	ihcC->openConnection();

	GetDevices();
	return (m_thread!=NULL);
}

bool CLKIHC::StopHardware()
{
	if (m_thread!=NULL)
	{
		assert(m_thread);
		m_stoprequested = true;
		m_thread->join();
	}
    m_bIsStarted=false;
    return true;
}

void CLKIHC::Do_Work()
{
	_log.Log(LOG_STATUS,"LK IHC: Worker started...");
	/*ihcClient ihcClient(m_IPAddress, m_UserName, m_Password);

	ihcClient.openConnection();


    //WSProjectInfo ts =  ihcClient.getProjectInfo();
	//ihcClient.loadProject();
	//std::cout << ts << std::endl;

    ResourceValue const ttt(1157981, RangedInteger(100));

    ResourceValue const t(83550, true);

    //a.value = false;

    if (ihcClient.resourceUpdate(t))
    {
        std::cout << "Resource update was successful\n";
    }
    else
        std::cout << "Failed resource update\n";*/
	std::string asd = "hej";
	char ID[40];
	sprintf(ID, "%lu", (long unsigned int)0x13645e);
	//m_sql.UpdateValue(1    , ID, 0, pTypeIHCWireless, sTypeRelay, 10, 255, 0, asd);
	std::string devname;
	//m_sql.UpdateValue(m_HwdID, ID   , 0, pTypeGeneralSwitch, sSwitchIHCAirRelay, 10, 255, 0, "Normal", devname);

	int sec_counter = 0;
	while (!m_stoprequested)
	{
		sec_counter++;

		if (sec_counter  % 12 == 0) {
			m_LastHeartbeat=mytime(NULL);

		}

	}
	_log.Log(LOG_STATUS,"LK IHC: Worker stopped...");
}

bool CLKIHC::WriteToHardware(const char *pdata, const unsigned char length)
{
	_log.Log(LOG_STATUS, "asdasd");

	_log.Log(LOG_STATUS, __FUNCTION__);

	const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
	if (pSen->ICMND.packettype == pTypeGeneralSwitch && pSen->ICMND.subtype == sSwitchIHCAirRelay)
	{
		std::cout << "Got relay with ID: " << std::endl;
		const _tGeneralSwitch *general = reinterpret_cast<const _tGeneralSwitch*>(pdata);

		std::cout << general->id << std::endl;

		ResourceValue const t(general->id, general->cmnd == gswitch_sOn ? true : false);
	    if (ihcC->resourceUpdate(t))
	    {
	        std::cout << "Resource update was successful\n";
	    }
	    else
	        std::cout << "Failed resource update\n";
	}
	else
	{
		std::cout << "Got relay with ID: " << std::endl;
		const _tGeneralSwitch *general = reinterpret_cast<const _tGeneralSwitch*>(pdata);

		std::cout << general->id << std::endl;

		ResourceValue const t(general->id, RangedInteger(general->level));
	    if (ihcC->resourceUpdate(t))
	    {
	        std::cout << "Resource update was successful\n";
	    }
	    else
	        std::cout << "Failed resource update\n";
	}


	return true;
}

bool CLKIHC::GetDevices()
{


	std::vector<std::vector<std::string> > result;
	result = m_sql.safe_query("SELECT DeviceID FROM DeviceStatus WHERE HardwareID==%d AND Used == 1",
			m_HwdID);



	if (result.size() > 0)
				{
					std::vector<std::vector<std::string> >::const_iterator itt;

					for (itt = result.begin(); itt != result.end(); ++itt)
					{
						std::vector<std::string> sd = *itt;
std::cout << sd[0] << std::endl;


					}
				}
	return false;
}

//Webserver helpers
namespace http {
	namespace server {
		void CWebServer::Cmd_LKIHCGetNodes(WebEmSession & session, const request& req, Json::Value &root)
		{
			if (session.rights != 2)
				return;//Only admin user allowed
			std::string hwid = request::findValue(&req, "idx");
			if (hwid == "")
				return;
			int iHardwareID = atoi(hwid.c_str());
			CDomoticzHardwareBase *pHardware = m_mainworker.GetHardware(iHardwareID);
			if (pHardware == NULL)
				return;
			std::cout << hwid << std::endl;
			/*if (pHardware->HwdType != HTYPE_WOL)
				return;*/

			root["status"] = "OK";
			root["title"] = "LKIHCGetNodes";

			std::vector<std::vector<std::string> > result;
			result = m_sql.safe_query("SELECT DeviceID, Name, (SubType-%d), StrParam1 FROM DeviceStatus WHERE (HardwareID==%d)",
					sSwitchIHCAirRelay,
				iHardwareID);
			if (result.size() > 0)
			{
				std::vector<std::vector<std::string> >::const_iterator itt;
				int ii = 0;
				for (itt = result.begin(); itt != result.end(); ++itt)
				{
					std::vector<std::string> sd = *itt;
std::cout << sd[1] << std::endl;
					root["result"][ii]["idx"] = sd[0];
					root["result"][ii]["Name"] = sd[1];
					root["result"][ii]["Type"] = sd[2];
					root["result"][ii]["Mac"] = sd[3];
					ii++;
				}
			}
		}
	}
}
