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
#define BATTERY_RSSI_POLL_INTERVAL 1000

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

ihcClient* ihcC;
void CLKIHC::Init()
{

}

bool CLKIHC::StartHardware()
{
    /*Start worker thread*/
    m_bIsStarted = true;
    sOnConnected(this);
    ihcC = new ihcClient(m_IPAddress, m_UserName, m_Password);
    m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CLKIHC::Do_Work, this)));

    return (m_thread != NULL);
}

bool CLKIHC::StopHardware()
{
    if (m_thread != NULL)
    {
        assert(m_thread);
        m_stoprequested = true;
        m_thread->join();
    }
    m_bIsStarted = false;

    return true;
}

std::string CLKIHC::getValue( TiXmlElement* const a, std::string const t)
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

void CLKIHC::Do_Work()
{
    _log.Log(LOG_STATUS,"LK IHC: Worker started...");
    int crashCounter = 0;
    bool firstTime = true;
    int sec_counter = 28;
    int batteryAndRSSI_counter = BATTERY_RSSI_POLL_INTERVAL - 1;

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
            try
            {
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
            batteryAndRSSI_counter++;

            try
            {
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

                if (0 == (batteryAndRSSI_counter % BATTERY_RSSI_POLL_INTERVAL))
                {
                    if (!UpdateBatteryAndRSSI())
                        batteryAndRSSI_counter--; //This is a hack, but if the update fails, retry it again in a while
                }

                // Handle object state changes
                for (std::vector<boost::shared_ptr<ResourceValue> >::iterator it = updatedResources.begin(); it != updatedResources.end(); ++it)
                {
                    ResourceValue & obj = *(*it);

                    _tGeneralSwitch switchcmd;
                    char szID[10];
                    std::sprintf(szID, "%08lX", (long unsigned int)obj.ID);

                    std::vector<std::vector<std::string> > result;
                    result = m_sql.safe_query("SELECT SubType FROM DeviceStatus WHERE (DeviceID='%q' AND HardwareID=='%d')", szID, m_HwdID);
                    if (result.size() != 0)
                    {
                        switchcmd.subtype = atoi(result[0][0].c_str());
                    }

                    switchcmd.id = (long unsigned int)obj.ID;
                    switchcmd.unitcode = 0;

                    if (obj.intValue() > 1)
                    {
                        switchcmd.cmnd = gswitch_sSetLevel;
                        switchcmd.level=obj.intValue();
                    }
                    else
                    {
                        switchcmd.cmnd = obj.intValue();
                        switchcmd.level=0;
                    }

                    m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&switchcmd, NULL, 3);
                }
            }
            catch(...)
            {
                crashCounter++;
                _log.Log(LOG_ERROR, "LK IHC: The connection to the IHC controller was lost %d times since Domoticz was started", crashCounter);
                ihcC->reset();
                firstTime = true;
            }
        }
        sleep_seconds(1);
    }

    _log.Log(LOG_STATUS,"LK IHC: Worker stopped...");
}

bool CLKIHC::UpdateBatteryAndRSSI()
{
    _log.Log(LOG_STATUS, "LK IHC: Updating battery and RSSI levels");
    TiXmlDocument const RFandRSSIinfo =  ihcC->getRF();
    TinyXPath::xpath_processor processor ( RFandRSSIinfo.RootElement(), "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:getDetectedDeviceList1/ns1:arrayItem");

    processor.u_compute_xpath_node_set(); // <-- this is important. It executes the Xpath expression
    if (processor.XNp_get_xpath_node(0)->FirstChild() != 0)
    {
        for (int i = 0; i < processor.u_compute_xpath_node_set(); i++)
        {
            TiXmlNode* const thisNode = processor.XNp_get_xpath_node(i);

            TiXmlElement * const res = thisNode->FirstChild()->ToElement();
            int const batteryLevel = boost::lexical_cast<int>(getValue(res, "/ns1:batteryLevel"));
            int const signalStrength = (int)(boost::lexical_cast<int>(getValue(res, "/ns1:signalStrength"))>>2);
            unsigned long const serialNumber   = boost::lexical_cast<unsigned long>(getValue(res, "/ns1:serialNumber"));

            m_sql.safe_query("UPDATE DeviceStatus SET BatteryLevel=%d, SignalLevel=%d WHERE (HardwareID==%d) AND (Options==%lld)",
                    (batteryLevel*100), signalStrength, m_HwdID, serialNumber);
        }
        return true;
    }
    return false;
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
                    case sSwitchIHCOutput:
                    {
                        /* Boolean value */
                        ResourceValue const t(general->id, general->cmnd == gswitch_sOn ? true : false);
                        result = ihcC->resourceUpdate(t);
                        break;
                    }

                    case sSwitchIHCInput:
                        return false;
                        break;

                    case sSwitchIHCDimmer:
                    {
                        /* Integer value */
                        uint8_t val = 0;
                        if (gswitch_sOff == general->cmnd)
                        { val = 0; }
                        else if (gswitch_sOn == general->cmnd)
                        { val = 100; }
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
                _log.Log(LOG_STATUS, "LK IHC: Resource update was successful");
            }
            else
            {
                _log.Log(LOG_STATUS, "LK IHC: Failed resource update");
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

    long unsigned int serialNumber = 0;
    std::string const devID_raw = device->ToElement()->Attribute("id");
    std::string devID = devID_raw.substr(3);

    char sid[10];
    sprintf(sid, "%08X", (static_cast<unsigned int>(std::strtoul(devID.c_str(), NULL, 16))));

    std::vector<std::vector<std::string> > result;
    result = m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%q')",
                              m_HwdID, sid);
    if (result.size() < 1)
    {
        #ifdef _DEBUG
            _log.Log(LOG_NORM, "LK IHC: Added device %d %s: %s", id, sid ,devname);
        #endif
        char buff[100];
        //TODO: Fix null location
        if (functionBlock)
        {
            snprintf(buff, sizeof(buff), "FB | %s | %s | %s",
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

            // If it's a wireless device, get the serial number so we can use it for the RSSI and battery level mappings
            if (device->Parent()->ToElement()->Attribute("serialnumber") != 0){
                std::string const serialNumber_raw = std::string(device->Parent()->ToElement()->Attribute("serialnumber")).substr(3);
                serialNumber = std::strtoul(serialNumber_raw.c_str(), NULL, 16);
            }
        }

        switch (deviceType)
        {
        case sSwitchIHCInput:
            m_sql.safe_query(
                "INSERT INTO DeviceStatus (HardwareID, DeviceID, Type, SubType, SwitchType, SignalLevel, BatteryLevel, Name, nValue, AddjValue, AddjValue2, sValue, Options) "
                "VALUES (%d,'%q',%d,%d,%d,12,255,'%q',0, %f, %f, ' ', '%lld')",
                m_HwdID, sid, pTypeGeneralSwitch, deviceType, STYPE_Contact, buff, 1.0, 1.0, serialNumber);
            break;

/*        case sSwitchIHCFBInput:
            m_sql.safe_query(
                "INSERT INTO DeviceStatus (HardwareID, DeviceID, Type, SubType, SwitchType, SignalLevel, BatteryLevel, Name, nValue, AddjValue, AddjValue2, sValue, Options) "
                "VALUES (%d,'%q',%d,%d,%d,12,255,'%q',0, %f, %f, ' ','%lld')",
                m_HwdID, sid, pTypeGeneralSwitch, deviceType, STYPE_PushOn, buff, 1.0, 1.0, serialNumber);
            break;
*/
        case sSwitchIHCDimmer:
            m_sql.safe_query(
                "INSERT INTO DeviceStatus (HardwareID, DeviceID, Type, SubType, SwitchType, SignalLevel, BatteryLevel, Name, nValue, sValue, Options) "
                "VALUES (%d,'%q',%d,%d,%d,12,255,'%q',0, ' ','%lld')",
                m_HwdID, sid, pTypeGeneralSwitch, deviceType, STYPE_Dimmer, buff, serialNumber);
            break;

        case sSwitchIHCOutput:
            m_sql.safe_query(
                "INSERT INTO DeviceStatus (HardwareID, DeviceID, Type, SubType, SwitchType, SignalLevel, BatteryLevel, Name, nValue, sValue, Options) "
                "VALUES (%d,'%q',%d,%d,%d,12,255,'%q',0, ' ','%lld')",
                m_HwdID, sid, pTypeGeneralSwitch, deviceType, STYPE_OnOff, buff, serialNumber);
            break;
        }
    }
}

void CLKIHC::iterateDevices(const TiXmlNode* deviceNode)
{

    if (strcmp(deviceNode->Value(), "airlink_dimming") == 0)
    {
        unsigned char const deviceType = sSwitchIHCDimmer;
        addDeviceIfNotExists(deviceNode, deviceType);
    }
    else if ((strcmp(deviceNode->Value(), "airlink_relay") == 0) || (strcmp(deviceNode->Value(), "dataline_output") == 0))
    {
        unsigned char const deviceType = sSwitchIHCOutput;
        addDeviceIfNotExists(deviceNode, deviceType);
    }
    else if ((strcmp(deviceNode->Value(), "airlink_input") == 0) || (strcmp(deviceNode->Value(), "dataline_input") == 0))
    {
        unsigned char const deviceType = sSwitchIHCInput;
        addDeviceIfNotExists(deviceNode, deviceType);
    }
    else if (strcmp(deviceNode->Value(), "inputs") == 0)
    {
        for (const TiXmlNode* node = deviceNode->FirstChild(); node; node = node->NextSibling())
        {
            if (strcmp(node->Value(), "resource_input") == 0)
            {
                unsigned char const deviceType = sSwitchIHCInput;
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
                unsigned char const deviceType = sSwitchIHCOutput;
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
        if (ihcC->CONNECTED == ihcC->connState)
        {// TODO: Wait for connection
            TiXmlDocument doc = ihcC->loadProject();

            TinyXPath::xpath_processor processor ( doc.RootElement(), "/utcs_project/groups/*/*[self::functionblock or self::product_dataline or self::product_airlink]");

            unsigned const numberOfDevices = processor.u_compute_xpath_node_set();

            for (int i = 0; i < numberOfDevices; i++)
            {
                TiXmlNode* thisNode = processor.XNp_get_xpath_node(i);
                iterateDevices(thisNode);
            }
            UpdateBatteryAndRSSI();
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
        _log.Log(LOG_ERROR, "LK IHC: Exception: '%s'", msg);
    }


    //m_mainworker.RestartHardware(idx);
}

}
}
