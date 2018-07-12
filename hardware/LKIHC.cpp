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

/*
 * if (firstRun)
 * {
 *     connect to controller and get project info
 *     if (project info has changed)
 *     {
 *         update database with new items
 *     }
 * }
 *
 * if (watched devices > 0)
 * {
 *     connect to controller
 *     updated watchlist
 *
 *     wait for changes
 *     if changes
 *     {
 *         handle changes
 *     }
 *
 * }
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
//#include "IHC/IhcClient.hpp"
#include <string.h>
#include <exception>
#include <cstdlib>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#define _IHCDEBUG 1
class ihcController
{
public:
	enum ConnectionState {
			DISCONNECTED,
			CONNECTING,
			CONNECTED
		} connectionState;

	ihcController(std::string const &ip, std::string const &username, std::string const &password)
		{
		    this->ip = ip;
		    this->username = username;
		    this->password = password;
		    this->connectionState = DISCONNECTED;
		}
private:
	std::string username;
	std::string password;
	std::string ip;
};
#define RESOURCE_NOTIFICATION_TIMEOUT_S 5
std::vector<int> activeResourceIdList;
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

ihcController* ihcC;
void CLKIHC::Init()
{

}
bool CLKIHC::StartHardware()
{

    /*Start worker thread*/
    m_bIsStarted=true;
    sOnConnected(this);
    ihcC = new ihcController(m_IPAddress, m_UserName, m_Password);

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
#ifdef _IHCDEBUG
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);
#endif
    _log.Log(LOG_STATUS,"LK IHC: Worker started...");

    bool firstTime = true;
    int sec_counter = 28;

    while (!m_stoprequested)
    {
		if (m_stoprequested)
			break;
		sec_counter++;
		if (sec_counter % 10 == 0) {
		m_LastHeartbeat = mytime(NULL);
		}

		if (ihcC->CONNECTED != ihcC->connectionState)
		{
			try
			{
				//if (0 == sec_counter % 30)
					//ihcC->openConnection();
			}
            catch (const char* msg)
            {
            	_log.Log(LOG_ERROR, "LK IHC: Error: '%s'", msg);
				//ihcC->reset();
				firstTime = true;
			}
		}

		sleep_seconds(1);
    }

    _log.Log(LOG_STATUS,"LK IHC: Worker stopped...");
}

bool CLKIHC::WriteToHardware(const char *pdata, const unsigned char length)
{
#ifdef _IHCDEBUG
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);
#endif
	return false;
}

void CLKIHC::addDeviceIfNotExists(const TiXmlNode* device, const unsigned char deviceType, bool functionBlock)
{
#ifdef _IHCDEBUG
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);
#endif
}

void CLKIHC::iterateDevices(const TiXmlNode* deviceNode)
{
#ifdef _IHCDEBUG
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);
#endif
}

void CLKIHC::GetDevicesFromController()
{
#ifdef _IHCDEBUG
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);
#endif
}

//Webserver helpers
namespace http {
namespace server {

void CWebServer::GetIHCProjectFromController(WebEmSession & session, const request& req, std::string & redirect_uri)
{
#ifdef _IHCDEBUG
	_log.Log(LOG_STATUS,"IHC_TRACE: %s", __FUNCTION__);
#endif
    redirect_uri = "/index.html";
    if (session.rights != 2)
    {
        //No admin user, and not allowed to be here
        return;
    }

    std::string const idx = request::findValue(&req, "idx");
    if (idx == "") {
        return;
    }

    int const iHardwareID = atoi(idx.c_str());
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

}

}
}
