#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include <map>

class CLKIHC : public CDomoticzHardwareBase
{
public:
	CLKIHC(const int ID, const std::string &IPAddress, const unsigned short Port, const std::string &Username, const std::string &Password);
	~CLKIHC(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	void GetDevicesFromController();

private:
	std::string m_IPAddress;
	unsigned short m_Port;
	std::string m_UserName;
	std::string m_Password;
	volatile bool m_stoprequested;
	boost::shared_ptr<boost::thread> m_thread;

	void Init();
	bool StartHardware();
	bool StopHardware();
	void Do_Work();
    bool AddSwitchIfNotExits(const int &id, const char* devname, const bool &isDimmer);
};
