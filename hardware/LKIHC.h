#pragma once

#include "DomoticzHardware.h"
#include <iostream>
#include <map>

class CLKIHC : public CDomoticzHardwareBase
{
	enum _eHueLightType
	{
		HLTYPE_NORMAL,
		HLTYPE_DIM,
		HLTYPE_RGBW,
		HLTYPE_SCENE
	};
	struct _tHueLight
	{
		bool on;
		int level;
		int sat;
		int hue;
	};
	struct _tHueGroup
	{
		_tHueLight gstate;
		std::vector<int> lights;
	};
	struct _tHueScene
	{
		std::string id;
		std::string name;
		std::string lastupdated;
	};
public:
	CLKIHC(const int ID, const std::string &IPAddress, const unsigned short Port, const std::string &Username, const std::string &Password);
	~CLKIHC(void);
	bool WriteToHardware(const char *pdata, const unsigned char length);
	static std::string RegisterUser(const std::string &IPAddress, const unsigned short Port, const std::string &username);
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
	bool GetDevices();
};

