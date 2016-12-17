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

    //Start worker thread

    m_bIsStarted=true;
    sOnConnected(this);
    ihcC = new ihcClient(m_IPAddress, m_UserName, m_Password);
    ihcC->openConnection();
    std::cout << __func__ << std::endl;
    m_thread = boost::shared_ptr<boost::thread>(new boost::thread(boost::bind(&CLKIHC::Do_Work, this)));
    //GetDevices();
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
	std::cout << __func__ << std::endl;
    _log.Log(LOG_STATUS,"LK IHC: Worker started...");

    char ID[40];
    sprintf(ID, "%lu", (long unsigned int)0x13645e); // 1270878
    //m_sql.UpdateValue(1    , ID, 0, pTypeIHCWireless, sTypeRelay, 10, 255, 0, asd);
    std::string devname;
    //m_sql.UpdateValue(m_HwdID, ID   , 0, pTypeGeneralSwitch, sSwitchIHCAirRelay, 10, 255, 0, "Normal", devname);
    sleep_seconds(5);
    std::vector<int> resourceIdLis;
    resourceIdLis.push_back(1270878);
    ihcC->enableRuntimeValueNotification();

    int sec_counter = 0;

    while (!m_stoprequested)
    {
        sleep_seconds(1);
        sec_counter++;

        std::vector<boost::shared_ptr<ResourceValue> > ress;
        ress = ihcC->waitResourceValueNotifications(RESOURCE_NOTIFICATION_TIMEOUT_S);

        for (std::vector<boost::shared_ptr<ResourceValue> >::iterator itt = ress.begin(); itt != ress.end(); ++itt)
        {
            ResourceValue & obj = *(*itt);/*
			_log.Log(LOG_NORM, "LKIHC Plugin: Object changed status (%s).", obj.toString().c_str());
			sprintf(ID, "%08X", (unsigned int)0x13645e); // 1270878
			m_sql.safe_query("UPDATE DeviceStatus SET nValue=%d WHERE(HardwareID == %d) AND (DeviceID == '%08X')",
						obj.intValue(), m_HwdID, obj.ID);
			_log.Log(LOG_NORM, "UPDATE DeviceStatus SET nValue=%d WHERE(HardwareID == %d) AND (DeviceID == '%08X')",
					obj.intValue(), m_HwdID, obj.ID);*/

            {


                int nvalue = obj.intValue();
                bool tIsOn = (nvalue != 0);
                int lastLevel = 0;
                int value = obj.intValue();
                //if ((bIsOn != tIsOn) || (value != lastLevel))
                {
                    int cmd = light1_sOn;
                    int level = 100;
                    if (!tIsOn) {
                        cmd = IHC_Off;
                        level = 0;
                    }
                    _tGeneralSwitch ycmd;
                    //ycmd.len = sizeof(pTypeGeneralSwitch) - 1;
                    //ycmd.type = pTypeGeneralSwitch;
                    //ycmd.subtype = sSwitchIHCAirRelay;
                    std::cout << "ID: " << obj.ID << "\n";
                    /*char szID[10];
                    std::sprintf(szID, "%08lX", (long unsigned int)obj.ID);*/

                    ycmd.id =  /*static_cast<long unsigned int>*/(obj.ID);//obj.ID;
                    ycmd.unitcode = 0;
                    //ycmd.battery_level = 10;
                    std::cout << "new value : " << obj.intValue() << std::endl;
                    ycmd.cmnd = obj.intValue();
                    ////ycmd.level=0;
                    //ycmd.rssi = 12;
                    //sDecodeRXMessage(this, (const unsigned char *)&ycmd, NULL, 255);
                    m_mainworker.PushAndWaitRxMessage(this, (const unsigned char *)&ycmd, NULL, -1);
                }
            }

            std::cout << obj.toString() << std::endl;//(*itt)->id;
        }


        //	if (ress.size() > 0)

        if (sec_counter % 2 == 0) {
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
        _log.Log(LOG_STATUS, "Got relay with ID: ");
        const _tGeneralSwitch *general = reinterpret_cast<const _tGeneralSwitch*>(pdata);

        std::cout << general->id << std::endl;

        ResourceValue const t(general->id, general->cmnd == gswitch_sOn ? true : false);
        if (ihcC->resourceUpdate(t))
        {

            _log.Log(LOG_STATUS, "Resource update was successful");
        }
        else
            _log.Log(LOG_STATUS, "Failed resource update");
    }
    else
    {
        std::cout << "Got relay with ID: " << std::endl;
        const _tGeneralSwitch *general = reinterpret_cast<const _tGeneralSwitch*>(pdata);

        std::cout << general->id << std::endl;

        ResourceValue const t(general->id, RangedInteger(general->level));
        if (ihcC->resourceUpdate(t))
        {
            _log.Log(LOG_STATUS, "Resource update was successful");
        }
        else
            _log.Log(LOG_STATUS, "Failed resource update");
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

void CLKIHC::AddDevice(const std::string nodeID, const std::string &Name, const std::string type, const std::string location)
{
    std::cout << __func__ << std::endl;
    std::vector<std::vector<std::string> > result;
    /*
	//Check if exists
	result=m_sql.safe_query("SELECT ID FROM DeviceStatus WHERE (HardwareID==%d) AND (DeviceID=='%d')",
		m_HwdID, type);
	if (result.size()>0)
		return; //Already exists*/
    std::string devname = "hej";
    char ID[40];
    //sprintf(ID, "%08u", nodeID);
    std::cout << ID << std::endl;
    m_sql.UpdateValue(m_HwdID, nodeID.c_str() , 0, pTypeGeneralSwitch, sSwitchIHCAirRelay, 10, 255, 0, "Normal", devname);
    /*

	m_sql.safe_query("INSERT INTO WOLNodes (HardwareID, Name, MacAddress) VALUES (%d,'%q','%q')",
		m_HwdID, Name.c_str(), MACAddress.c_str());

	result=m_sql.safe_query("SELECT ID FROM WOLNodes WHERE (HardwareID==%d) AND (Name=='%q') AND (MacAddress=='%q')",
		m_HwdID, Name.c_str(), MACAddress.c_str());
	if (result.size()<1)
		return;

	int ID=atoi(result[0][0].c_str());

	char szID[40];
	sprintf(szID,"%X%02X%02X%02X", 0, 0, (ID&0xFF00)>>8, ID&0xFF);

	//Also add a light (push) device
	m_sql.safe_query(
		"INSERT INTO DeviceStatus (HardwareID, DeviceID, Unit, Type, SubType, SwitchType, Used, SignalLevel, BatteryLevel, Name, nValue, sValue) "
		"VALUES (%d,'%q',%d,%d,%d,%d,1, 12,255,'%q',1,' ')",
		m_HwdID, szID, int(1), pTypeLighting2, sTypeAC, int(STYPE_PushOn), Name.c_str());*/
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

void CWebServer::Cmd_LKIHCAddNode(WebEmSession & session, const request& req, Json::Value &root)
{

    std::cout << "add node\n";
    if (session.rights != 2)
    {
        //No admin user, and not allowed to be here
        return;
    }

    std::string hwid = request::findValue(&req, "idx");
    std::string nodeid = request::findValue(&req, "nodeid");
    std::string nodename = request::findValue(&req, "nodename");
    std::string nodetype = request::findValue(&req, "nodetype");
    std::string nodelocation = request::findValue(&req, "nodelocation");
    std::cout << hwid << ":" << nodeid << ":" << nodename << ":" << nodetype << ":" << nodelocation << std::endl;
    /*if (
			(hwid == "") ||
			(name == "") ||
			(mac == "")
			)
			return;*/
    int iHardwareID = atoi(hwid.c_str());
    CDomoticzHardwareBase *pBaseHardware = m_mainworker.GetHardware(iHardwareID);
    /*if (pBaseHardware == NULL)
			return;
		if (pBaseHardware->HwdType != HTYPE_WOL)
			return;*/
    CLKIHC *pHardware = reinterpret_cast<CLKIHC*>(pBaseHardware);

    root["status"] = "OK";
    root["title"] = "WOLAddNode";
    pHardware->AddDevice(nodeid, nodename, nodetype,nodelocation);
}
}
}
