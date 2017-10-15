/*!
 * \file hardware/LKIHC.cpp
 * \author  Jonas Bo Jalling <jonas@jalling.dk>
 * \version 0.1
 *
 * \section LICENSE
 *
 * The MIT License
 *
 * Copyright (c) 2017 Jonas Bo Jalling
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * \section DESCRIPTION
 *
 * This provides a interface to the LK IHC home automation system
 *
 */

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
#include <exception>
#include <cstdlib>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#define RESOURCE_NOTIFICATION_TIMEOUT_S 5

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

    /*Start worker thread*/
    m_bIsStarted=true;
    sOnConnected(this);
    ihcC = new ihcClient(m_IPAddress, m_UserName, m_Password);

    m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CLKIHC::Do_Work, this)));

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
    int crashCounter = 0;

    bool firstTime = true;
    int sec_counter = 28;

    while (!m_stoprequested)
    {

	if (m_stoprequested)
	    break;
	sec_counter++;
	//if (sec_counter % 10 == 0) {
	m_LastHeartbeat = mytime(NULL);
	//}

	if (ihcC->CONNECTED != ihcC->connState)
	    {
	    try {
		if (sec_counter % 30 == 0)
		    ihcC->openConnection();
	    }
	    catch (...)
		{
		ihcC->reset();
		firstTime = true;
		}
	    }

	if (ihcC->CONNECTED == ihcC->connState)
	    {

	    try {
		if (firstTime)
		    {
		    firstTime = false;

		    WSProjectInfo inf = ihcC->getProjectInfo();
		    std::cout << inf;

		    std::vector<int> resourceIdList;

		    std::vector<std::vector<std::string> > result;
		    result = m_sql.safe_query("SELECT DeviceID FROM DeviceStatus WHERE HardwareID==%d AND Used == 1", m_HwdID);

		    if (result.size() > 0)
			{
			std::vector<std::vector<std::string> >::const_iterator itt;
			for (itt = result.begin(); itt != result.end(); ++itt)
			    {
			    std::vector<std::string> sd = *itt;
			    resourceIdList.push_back(std::strtoul(sd[0].c_str(), NULL, 16));
			    }
			}

		    ihcC->enableRuntimeValueNotification(resourceIdList);

		    }

		std::vector<boost::shared_ptr<ResourceValue> > updatedResources;
		updatedResources = ihcC->waitResourceValueNotifications(RESOURCE_NOTIFICATION_TIMEOUT_S);

		// Handle object state changes
		for (std::vector<boost::shared_ptr<ResourceValue> >::iterator it = updatedResources.begin(); it != updatedResources.end(); ++it)
		    {
		    ResourceValue & obj = *(*it);

		    int nvalue = obj.intValue();
		    bool tIsOn = (nvalue != 0);
		    int lastLevel = 0;
		    int value = obj.intValue();

		    _tGeneralSwitch ycmd;
		    ycmd.subtype = sSwitchIHCAirRelay;
		    char szID[10];
		    std::sprintf(szID, "%08lX", (long unsigned int)obj.ID);

		    std::vector<std::vector<std::string> > result;
		    result = m_sql.safe_query("SELECT SubType FROM DeviceStatus WHERE (DeviceID='%q' AND HardwareID=='%d')", szID, m_HwdID);
		    if (result.size() != 0)
			{
			ycmd.subtype =  atoi(result[0][0].c_str());
			}

		    ycmd.id =  (long unsigned int)obj.ID;
		    ycmd.unitcode = 0;
		    ycmd.battery_level = 10;

		    if (obj.intValue() > 1)
			{
			ycmd.cmnd = 2;
			ycmd.level=obj.intValue();
			}
		    else
			{
			ycmd.cmnd = obj.intValue();
			ycmd.level=0;
			}
		    //TODO: FIX Rssi
		    ycmd.rssi = 12;

		    m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, NULL, 100);

		    }

	    }
	    catch(...)
		{
		crashCounter++;
		_log.Log(LOG_ERROR, "The IHC controller has crashed %d times since Domoticz was started", crashCounter);
		ihcC->reset();
		firstTime = true;
		}

	    }
	sleep_seconds(1);
    }

    _log.Log(LOG_STATUS,"LK IHC: Worker stopped...");
}
bool CLKIHC::WriteToHardware(const char *pdata, const unsigned char length)
{
    bool result = false;
    const tRBUF *pSen = reinterpret_cast<const tRBUF*>(pdata);
    if (ihcC->CONNECTED == ihcC->connState)
    {
	try
	{
	    if (pSen->ICMND.packettype == pTypeGeneralSwitch)
	    {
		const _tGeneralSwitch *general = reinterpret_cast<const _tGeneralSwitch*>(pdata);
		switch (pSen->ICMND.subtype)
		{

		    case sSwitchIHCFBInput:
		    {
			/* Boolean value */
			ResourceValue const t(general->id, general->cmnd == gswitch_sOn ? true : false);
			result = ihcC->resourceUpdate(t);
			/* sleep_milliseconds(500);
				    ResourceValue const t2(general->id, false);
				    result = ihcC->resourceUpdate(t2);*/
			break;
		    }
		    case sSwitchIHCFBOutput:
			return true;
			break;
		    case sSwitchIHCWiredRelay:
		    case sSwitchIHCWiredBtns:
		    case sSwitchIHCAirRelay:
		    case sSwitchIHCAirBtns:
		    {
			/* Boolean value */
			ResourceValue const t(general->id, general->cmnd == gswitch_sOn ? true : false);
			result = ihcC->resourceUpdate(t);
			break;
		    }
		    case sSwitchIHCWiredDimmer:
		    case sSwitchIHCAirDimmer:
		    {
			/* Integer value */
			uint8_t val = 0;
			if (general->cmnd == gswitch_sOff)
			{
			val = 0;
			    }
			else if (general->cmnd == gswitch_sOn)
			{
			    val = 100;
			}
			else
			{
			    val = general->level;
			}
			ResourceValue const t(general->id, RangedInteger(val));
			result = ihcC->resourceUpdate(t);
			break;
		    }
		}
	    }

	    if (result)
	    {
		_log.Log(LOG_STATUS, "Resource update was successful");
	    }
	    else
	    {
		_log.Log(LOG_STATUS, "Failed resource update");
	    }

	}

	catch (...)
	{
	    ihcC->reset();
	}

	return result;
    }
    else
        return false;
}

void CLKIHC::addDeviceIfNotExists(const TiXmlNode* device, const unsigned char deviceType, bool functionBlock)
{
    std::string devID = device->ToElement()->Attribute("id");
    std::string d = devID.substr(3);

    char sid[10];
    sprintf(sid, "%08X", (static_cast<unsigned int>(std::strtoul(d.c_str(), NULL, 16))));

    std::vector<std::vector<std::string> > result;
    result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')",
                              m_HwdID, sid);
    if (result.size() < 1)
    {
        #ifdef _DEBUG
                _log.Log(LOG_NORM, "LK IHC: Added device %d %s: %s", id, sid ,devname);
        #endif
        char buff[100];
        if (functionBlock)
        {
            snprintf(buff, sizeof(buff), "%s | %s | %s",
                device->Parent()->ToElement()->Parent()->ToElement()->Parent()->ToElement()->Attribute("name"),
                device->Parent()->ToElement()->Parent()->ToElement()->Attribute("name"),
                device->ToElement()->Attribute("name"));

        }
        else
        {
            snprintf(buff, sizeof(buff), "%s | %s | %s",
                device->Parent()->ToElement()->Parent()->ToElement()->Attribute("name"),
                device->Parent()->ToElement()->Attribute("position"),
                device->ToElement()->Attribute("name"));
        }
        switch (deviceType)
        {
        case sSwitchIHCFBInput:
        case sSwitchIHCWiredBtns:
        case sSwitchIHCAirBtns:
            m_sql.safe_query(
                "INSERT INTO DeviceStatus (HardwareID, DeviceID, Type, SubType, SwitchType, SignalLevel, BatteryLevel, Name, nValue, AddjValue, AddjValue2, sValue) "
                "VALUES (%d,'%q',%d,%d,%d,12,255,'%q',0, %f, %f, ' ')",
                m_HwdID, sid, pTypeGeneralSwitch, deviceType, STYPE_Contact, 1.0, 1.0, buff);
            break;
        case sSwitchIHCFBOutput:
            m_sql.safe_query(
                "INSERT INTO DeviceStatus (HardwareID, DeviceID, Type, SubType, SwitchType, SignalLevel, BatteryLevel, Name, nValue, AddjValue, AddjValue2, sValue) "
                "VALUES (%d,'%q',%d,%d,%d,12,255,'%q',0, %f, %f, ' ')",
                m_HwdID, sid, pTypeGeneralSwitch, deviceType, STYPE_PushOn, 1.0, 1.0, buff);
            break;

        case sSwitchIHCWiredDimmer:
        case sSwitchIHCAirDimmer:
            m_sql.safe_query(
                "INSERT INTO DeviceStatus (HardwareID, DeviceID, Type, SubType, SwitchType, SignalLevel, BatteryLevel, Name, nValue, sValue) "
                "VALUES (%d,'%q',%d,%d,%d,12,255,'%q',0, ' ')",
                m_HwdID, sid, pTypeGeneralSwitch, deviceType, STYPE_Dimmer, buff);

            break;


        case sSwitchIHCWiredRelay:
        case sSwitchIHCAirRelay:
            m_sql.safe_query(
                "INSERT INTO DeviceStatus (HardwareID, DeviceID, Type, SubType, SwitchType, SignalLevel, BatteryLevel, Name, nValue, sValue) "
                "VALUES (%d,'%q',%d,%d,%d,12,255,'%q',0, ' ')",
                m_HwdID, sid, pTypeGeneralSwitch, deviceType, STYPE_OnOff, buff);

            break;



        /*case sSwitchIHCWiredBtns:
            m_sql.safe_query(
                "INSERT INTO DeviceStatus (HardwareID, DeviceID, Type, SubType, SwitchType, SignalLevel, BatteryLevel, Name, nValue, AddjValue, sValue) "
                "VALUES (%d,'%q',%d,%d,%d,12,255,'%q',0, %f, ' ')",
                m_HwdID, sid, pTypeGeneralSwitch, sSwitchIHCWiredBtns, STYPE_PushOn, 1.0, buff);
            break;
*/
        /*case sSwitchIHCWiredDimmer:
            m_sql.safe_query(
                "INSERT INTO DeviceStatus (HardwareID, DeviceID, Type, SubType, SwitchType, SignalLevel, BatteryLevel, Name, nValue, sValue) "
                "VALUES (%d,'%q',%d,%d,%d,12,255,'%q',0, ' ')",
                m_HwdID, sid, pTypeGeneralSwitch, sSwitchIHCWiredDimmer, STYPE_Dimmer, buff);

            break;
*/
  /*      case sSwitchIHCWiredRelay:
            m_sql.safe_query(
                "INSERT INTO DeviceStatus (HardwareID, DeviceID, Type, SubType, SwitchType, SignalLevel, BatteryLevel, Name, nValue, sValue) "
                "VALUES (%d,'%q',%d,%d,%d,12,255,'%q',0, ' ')",
                m_HwdID, sid, pTypeGeneralSwitch, sSwitchIHCWiredRelay, STYPE_OnOff, buff);

            break;*/
        }
    }
}

void CLKIHC::iterateDevices(const TiXmlNode* deviceNode)
{
    if (strcmp(deviceNode->Value(), "airlink_dimming") == 0)
    {
        unsigned char const deviceType = sSwitchIHCAirDimmer;
        addDeviceIfNotExists(deviceNode, deviceType);
    }
    else if (strcmp(deviceNode->Value(), "airlink_relay") == 0)
    {
        unsigned char const deviceType = sSwitchIHCAirRelay;
        addDeviceIfNotExists(deviceNode, deviceType);
    }
    else if (strcmp(deviceNode->Value(), "airlink_input") == 0)
    {
        unsigned char const deviceType = sSwitchIHCAirBtns;
        addDeviceIfNotExists(deviceNode, deviceType);
    }
    else if (strcmp(deviceNode->Value(), "dataline_input") == 0)
    {
        unsigned char const deviceType = sSwitchIHCWiredBtns;
        addDeviceIfNotExists(deviceNode, deviceType);
    }
    else if (strcmp(deviceNode->Value(), "dataline_output") == 0)
    {
        unsigned char const deviceType = sSwitchIHCWiredRelay;
        addDeviceIfNotExists(deviceNode, deviceType);
    }
    else if (strcmp(deviceNode->Value(), "inputs") == 0)
    {
        for (const TiXmlNode* node = deviceNode->FirstChild(); node; node = node->NextSibling())
        {
            if (strcmp(node->Value(), "resource_input") == 0)
            {
                unsigned char const deviceType = sSwitchIHCFBInput;
                addDeviceIfNotExists(node, deviceType, true);
            }
        }
    }
    else if (strcmp(deviceNode->Value(), "outputs") == 0)
    {
        for (const TiXmlNode* node = deviceNode->FirstChild(); node; node = node->NextSibling())
        {
            if (strcmp(node->Value(), "resource_output") == 0)
            {
                unsigned char const deviceType = sSwitchIHCFBOutput;
                addDeviceIfNotExists(node, deviceType, true);
            }
        }
    }

    for (const TiXmlNode* node = deviceNode->FirstChild(); node; node = node->NextSibling())
    {
        iterateDevices(node);
    }
}

void CLKIHC::GetDevicesFromController()
{
    try
    {
	TiXmlDocument doc = ihcC->loadProject();

	TinyXPath::xpath_processor processor ( doc.RootElement(), "/utcs_project/groups/*/*[self::functionblock or self::product_dataline or self::product_airlink]");

	unsigned const numberOfDevices = processor.u_compute_xpath_node_set();

	for (int i = 0; i < numberOfDevices; i++)
	{
	    TiXmlNode* thisNode = processor.XNp_get_xpath_node(i);
	    iterateDevices(thisNode);
	}
    }
    catch (...)
    {
	ihcC->reset();
    }
}

//Webserver helpers
namespace http {
namespace server {

void CWebServer::GetIHCProjectFromController(WebEmSession & session, const request& req, std::string & redirect_uri)
{
    redirect_uri = "/index.html";
    if (session.rights != 2)
    {
        //No admin user, and not allowed to be here
        return;
    }

    std::string idx = request::findValue(&req, "idx");
    if (idx == "") {
        return;
    }

    int iHardwareID = atoi(idx.c_str());
    CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
    if (pBaseHardware == NULL)
        return;
    if (pBaseHardware->HwdType != HTYPE_IHC)
        return;
    CLKIHC *pHardware = reinterpret_cast<CLKIHC*>(pBaseHardware);

    try
    {
        pHardware->GetDevicesFromController();
    }
    catch (const char* msg)
    {
        _log.Log(LOG_ERROR, "LKIHC Plugin: Exception: '%s'", msg);
    }


    //m_mainworker.RestartHardware(idx);
}

}
}
