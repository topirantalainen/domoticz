/*
 * IhcClient.hpp
 *
 *  Created on: 30. sep. 2016
 *      Author: Jonas Bo Jalling
 */

#ifndef IHC_IHCCLIENT_HPP_
#define IHC_IHCCLIENT_HPP_
#include <string>
#include <cstring>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <pthread.h>
#include "../../httpclient/HTTPClient.h"
#include "datatypes/WSDate.hpp"
#include "datatypes/WSUser.hpp"
#include "datatypes/WSUserGroup.hpp"
#include "datatypes/WSBaseDataType.hpp"
#include "datatypes/WSLoginResult.hpp"
#include "datatypes/WSResourceValue.hpp"
#include "datatypes/WSProjectInfo.hpp"
#include "datatypes/WSFile.hpp"
#include "datatypes/WSControllerState.hpp"
#include "IhcAuthenticationService.hpp"
#include "IhcResourceInteractionService.hpp"
#include "IhcControllerService.hpp"
	const char* B64chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	const int B64index [256] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
	   56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
	    7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,
	    0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
	   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };
class Thread
{
public:
	Thread();
	virtual ~Thread();

	int start();
	int join();
	int detach();
	pthread_t self();

	virtual void* run() = 0;

private:
	pthread_t  m_tid;
	int        m_running;
	int        m_detached;
};

static void* runThread(void* arg)
{
    return ((Thread*)arg)->run();
}

Thread::Thread() : m_tid(0), m_running(0), m_detached(0) {}

Thread::~Thread()
{
    if (m_running == 1 && m_detached == 0) {
        pthread_detach(m_tid);
    }
    if (m_running == 1) {
        pthread_cancel(m_tid);
    }
}

int Thread::start()
{
    int result = pthread_create(&m_tid, NULL, runThread, this);
    if (result == 0) {
        m_running = 1;
    }
    return result;
}

int Thread::join()
{
    int result = -1;
    if (m_running == 1) {
        result = pthread_join(m_tid, NULL);
        if (result == 0) {
            m_detached = 0;
        }
    }
    return result;
}

int Thread::detach()
{
    int result = -1;
    if (m_running == 1 && m_detached == 0) {
        result = pthread_detach(m_tid);
        if (result == 0) {
            m_detached = 1;
        }
    }
    return result;
}

pthread_t Thread::self() {
    return m_tid;
}



class ihcClient
{

public:
	enum ConnectionState {
		DISCONNECTED,
		CONNECTING,
		CONNECTED
	} connState;

private:
	std::string username;
	std::string password;
	std::string ip;
	std::string projectFile;
	std::string dumpResourceToFile;

	IhcAuthenticationService* authenticationService;
	IhcResourceInteractionService* resourceInteractionService;
	IhcControllerService* controllerService;

	WSControllerState controllerState;

public:
	ihcClient(std::string const ip, std::string const username, std::string const password)
	{
		this->ip = ip;
		this->username = username;
		this->password = password;
		this->connState = DISCONNECTED;
	}

	std::string getUsername()
	{ return username; }

	void setUsernae(std::string username)
	{ this->username = username; }

	std::string getPassword()
	{ return password; }

	void setPassword(std::string password)
	{ this->password = password; }

	std::string getIp()
	{ return ip; }

	void setIp(std::string ip)
	{ this->ip = ip; }

	std::string getProjectFile()
	{ return projectFile; }

	void setProjectFile(std::string path)
	{ this->projectFile = path; }

	std::string getDumpResourceInformationToFile()
	{ return dumpResourceToFile; }

	void setDumpResourcesInformationToFile(std::string const value)
	{ this->dumpResourceToFile = value; }

	/*ConnectionState ConnectionState getConnectionState()
	{
		return connState;
	}*/
/*
	void addEventListener(IhcEventListener listener)
	{
		eventListeners.add(listener);
	}

	void removeEventListener(IhcEventListener listener)
	{
		eventListener.remove(listener);
	}
*/
	/*
	std::unique_ptr<WSResourceValue> resourceQuery(int resoureId)
	{
		return resourceInteractionService->resourceQuery(resoureId);

	}*/
	ResourceValue resourceQuery(int const res)
	{
		std::cout << __func__ << std::endl;
		return resourceInteractionService->resourceQuery(res);
	}
/*
	void enableRuntimeValueNotifications(std::vector<int> resourceIdList)
	{
		resourceInteractionService->enableRuntimeValueNotifications(resourceIdList);
	}*/

	void enableRuntimeValueNotifications2(std::vector<int> resourceIdList)
	{
		std::cout << __func__ << std::endl;
		resourceInteractionService->enableRuntimeValueNotifications(resourceIdList);
	}

	std::vector<boost::shared_ptr<ResourceValue> > waitResourceValueNotifications(int const timeout)
	{
		std::cout << __func__ << std::endl;
		return resourceInteractionService->waitResourceValueNotifications(timeout);
	}

	void openConnection()
	{
		connState = CONNECTING;
		std::cout << __func__ << std::endl;
		authenticationService = new IhcAuthenticationService(ip);
		WSLoginResult* loginResult = authenticationService->authenticate(username, password, "treeview");
		std::cout << __func__ << std::endl;
		if (!loginResult->isLoginWasSuccessful())
		{
			//Todo: Fill out reasons why connection was not established
			throw "cant connect/authenticate\n";
			connState = DISCONNECTED;
			return;
		}

		std::cout << "Connection established\n";
		connState = CONNECTED;

		resourceInteractionService = new IhcResourceInteractionService(ip);
		controllerService = new IhcControllerService(ip);
		controllerState = controllerService->getControllerState();
		//loadProject(); //Todo: insert again
		//startIhcListeners();

		//controllerService->getRF();
	}

//private:
//	void startIhcListeners()
//	{
//		std::cout << __func__ << std::endl;
//		std::cout << "startIhcListeners" << std::endl;
//		IhcResourceValueNotificationListener * resourceValueNotificationListener = new IhcResourceValueNotificationListener(*this);
//		resourceValueNotificationListener->start();
//
//	}
//
//	class IhcResourceValueNotificationListener : public Thread
//	{
//		bool interrupted = false;
//		ihcClient &outer;
//	public:
//
//		IhcResourceValueNotificationListener(ihcClient &o): outer(o){};
//
//		void setInterrupted(bool interrupted)
//		{
//			std::cout << __func__ << std::endl;
//			this->interrupted = interrupted;
//		}
//
//		void* run()
//		{
//			std::cout << __func__ << std::endl;
//			std::cout << "IHC resource value listener started" << std::endl;
//
//
//			// as long as no interrupt is requested, continue running
//			while (!interrupted)
//			{
//				waitResourceNotifications();
//			}
//
//			std::cout <<"IHC Listener stopped" << std::endl;
//		}
//
//	private:
//		void waitResourceNotifications()
//		{
//			std::cout << __func__ << std::endl;
//			//try
//			//{
//			std::cout << "-Start---------------------------------------------------------------------------------------------------------------" << std::endl;
//				std::cout << "Wait new resource value notifications from controller" << std::endl;
//				std::vector<std::unique_ptr<ResourceValue>> resourceValueList = outer.waitResourceValueNotifications2(10);
//				std::cout << resourceValueList.size() << " new notifications received from controller" << std::endl;
//
//				for( auto&& pointer : resourceValueList ) {
//				    std::cout << pointer->toString()<< "\n";
//				}
//				/*
//				for (auto i = std::next(resourceValueList.begin()); i != resourceValueList.end(); ++i)
//				        std::cout << ", " << **i.toString();*/
//				/*
//				for (std::vector<std::unique_ptr<ResourceValue>>::iterator it = resourceValueList.begin(); it != resourceValueList.end(); ++it)
//				{
//					//query += "<xsd:arrayItem>" + std::to_string(*it) + "</xsd:arrayItem>";
//					std::cout << &it->toString() << std::endl;
//				}*/
//
//
//				std::cout << "-End---------------------------------------------------------------------------------------------------------------" << std::endl;
//				//IhcStatusUpdateEvent event = new IhcStatusUpdateEvent();
//
//				/*List<? extends WSResourceValue> resourceValueList = waitResourceValueNotifications(10);
//				/logger.debug("{} new notifications received from controller", resourceValueList.size());
//				IhcStatusUpdateEvent event = new IhcStatusUpdateEvent(this);
//				for (int i = 0; i < resourceValueList.size(); i++)
//				{
//					try
//					{
//						Iterator<IhcEventListener> iterator = eventListeners.iterator();
//						while (iterator.hasNext())
//						{
//							iterator.next().resourceValueUpdateReceived(event, resourceValueList.get(i));
//						}
//					}
//					catch (Exception e)
//					{
//						logger.error("Event listener invoking error", e);
//					}
//				}
//			}
//			catch (SocketTimeoutException e)
//			{
//				logger.trace("Notifications timeout - no new notifications");
//			} catch (IhcExecption e)
//			{
//				logger.error("New notifications wait failed...", e);
//				sendErrorEvent(e);
//				mysleep(1000L);
//			}*/
//				mysleep(2);
//		}
//
//		void mysleep(int sec)
//		{
//			std::cout << __func__ << std::endl;
//			/*try
//			{*/
//			for (int i = 0; i < sec; i++)
//			{
//				std::cout << i << std::endl;
//				sleep(1);
//			}
//			/*} catch (InterruptedException e) {
//				interrupted = true;
//			}*/
//		}
//	};

public:

	WSProjectInfo getProjectInfo()
	{
		std::cout << __func__ << std::endl;
		return controllerService->getProjectInfo();
	}

	void loadProject()
	{
		std::cout << __func__ << std::endl;

		if (!projectFile.empty())
		{
			std::cout << "Loading IHC /ELKO LS project file from path " << projectFile << "\n";
/*
			try {
				enumDictionary = IhcProjectFile.parseProject(projectFile, dumpResourcesToFile);
			} catch (IhcExecption e) {
				logger.error("Project file loading error", e);
			}*/
		}
		else
		{
			std::cout << "Loading IHC /ELKO LS project file from controller...\n";
			TiXmlDocument doc = LoadProjectFileFromController();
			/*try
			 {

				Document doc = LoadProjectFileFromController();
				IhcProjectFile.parseProject(doc, dumpResourcesToFile);
			} catch (IhcExecption e)
			 {
				throw new IhcExecption("Project file loading error", e);
			}*/
		}
	}




	std::string b64decode(const void* data, const size_t len)
	{
	    unsigned char* p = (unsigned char*)data;
	    int pad = len > 0 && (len % 4 || p[len - 1] == '=');
	    const size_t L = ((len + 3) / 4 - pad) * 4;
	    std::string str(L / 4 * 3 + pad, '\0');

	    for (size_t i = 0, j = 0; i < L; i += 4)
	    {
	        int n = B64index[p[i]] << 18 | B64index[p[i + 1]] << 12 | B64index[p[i + 2]] << 6 | B64index[p[i + 3]];
	        str[j++] = n >> 16;
	        str[j++] = n >> 8 & 0xFF;
	        str[j++] = n & 0xFF;
	    }
	    if (pad)
	    {
	        int n = B64index[p[L]] << 18 | B64index[p[L + 1]] << 12;
	        str[str.size() - 1] = n >> 16;

	        if (len > L + 2 && p[L + 2] != '=')
	        {
	            n |= B64index[p[L + 2]] << 6;
	            str.push_back(n >> 8 & 0xFF);
	        }
	    }
	    return str;
	}

	std::string b64decode(const std::string& str64)
	{
	    return b64decode(str64.c_str(), str64.size());
	}

	static std::string decompress(const std::string& data)
	{
		namespace bio = boost::iostreams;

		std::stringstream compressed(data);
		std::stringstream decompressed;

		bio::filtering_streambuf<bio::input> out;
		out.push(bio::gzip_decompressor());
		out.push(compressed);
		bio::copy(out, decompressed);

		return decompressed.str();
	}
private:
	TiXmlDocument LoadProjectFileFromController()
	{
		std::cout << __func__ << std::endl;
		WSProjectInfo projectInfo = getProjectInfo();
		int numberOfSegments = controllerService->getProjectNumberOfSegments();
		int segmentationSize = controllerService->getProjectSegmentationSize();

		std::vector<char> asd;
		std::cout << " size of asd is now: " << asd.size() << std::endl;

		for (int i = 0; i < numberOfSegments; i++)
		{
			std::cout << "Downloading segment " << i << "..." << std::endl;
			WSFile data = controllerService->getProjectSegment(i, getProjectInfo().getProjectMajorRevision(), getProjectInfo().getProjectMinorRevision());

			std::vector<char> t = data.getData();
			asd.insert(asd.end(), t.begin(), t.end());

			std::cout << " size of aaaasd is now: " << asd.size() << std::endl;


		}
		std::cout << "Final size: " << asd.size() << std::endl;

		std::string str(asd.begin(), asd.end());
		std::string decoded;
		decoded = b64decode(str);
std::string extracted;
		extracted = decompress(decoded);
		std::cout << "Final size decoded: " << decoded.size() << std::endl;
		std::cout << "Final size extracted: " << extracted.size() << std::endl;

		    std::ofstream out("output.txt");
		    out << extracted;
		    out.close();

		TiXmlDocument doc;
		doc.Parse(extracted.c_str());
		return doc;

		/*std::string sResult;

		sResult = sendQuery(url, emptyQuery, "getIHCProjectNumberOfSegments");

		doc.Parse(sResult.c_str());
		doc.Print();*/
		//projectInfo.encodeData(doc);

		//return projectInfo;
	}
public:
	/*template <typename T> bool resourceUpdate(T value)
	{
		return resourceInteractionService->resourceUpdate(value);
	}*/

	template <typename T> bool resourceUpdate(T value)
		{
		std::cout << __func__ << std::endl;
			return resourceInteractionService->resourceUpdate(value);
		}
void enableRuntimeValueNotification(void)
{
	std::cout << __func__ << std::endl;
	std::vector<int> resourceIdLis;
		resourceIdLis.push_back(1270878);

		resourceInteractionService->enableRuntimeValueNotifications(resourceIdLis);
}
/*
std::vector<boost::shared_ptr<ResourceValue> > waitResourceValueNotifications(int const timeout)
	{
		std::cout << __func__ << std::endl;
		return resourceInteractionService->waitResourceValueNotifications(timeout);
	}
*/

};

#endif /* IHC_IHCCLIENT_HPP_ */
