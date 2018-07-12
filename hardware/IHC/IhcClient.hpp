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
#include "../webserver/Base64.h"

class ihcClient
{

public:
	enum ConnectionState {
		DISCONNECTED,
		CONNECTING,
		CONNECTED
	} connectionState;

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
ihcClient(std::string const &ip, std::string const &username, std::string const &password)
{
    this->ip = ip;
    this->username = username;
    this->password = password;
    this->connectionState = DISCONNECTED;
}

std::string getUsername()
{ return username; }

void setUsername(std::string &username)
{ this->username = username; }

void setPassword(std::string &password)
{ this->password = password; }

std::string getIp()
{ return ip; }

void setIp(std::string &ip)
{ this->ip = ip; }

std::string getProjectFile()
{ return projectFile; }

void setProjectFile(std::string &path)
{ this->projectFile = path; }

std::string getDumpResourceInformationToFile()
{ return dumpResourceToFile; }

void setDumpResourcesInformationToFile(std::string const &value)
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
ResourceValue resourceQuery(int const &res)
{ return resourceInteractionService->resourceQuery(res); }
/*
void enableRuntimeValueNotifications(std::vector<int> resourceIdList)
{
    resourceInteractionService->enableRuntimeValueNotifications(resourceIdList);
}*/

void enableRuntimeValueNotifications2(std::vector<int> resourceIdList)
{ resourceInteractionService->enableRuntimeValueNotifications(resourceIdList); }

std::vector<boost::shared_ptr<ResourceValue> > waitResourceValueNotifications(int const timeout)
{ return resourceInteractionService->waitResourceValueNotifications(timeout); }

void reset()
{
	connectionState = DISCONNECTED;
}

void openConnection()
{
    connectionState = CONNECTING;

    authenticationService = new IhcAuthenticationService(ip);

    WSLoginResult* loginResult;
    try
    {

         loginResult= authenticationService->authenticate(username, password);
    }
    catch (const char* msg)
                {
            _log.Log(LOG_ERROR, "LK IHC: Error: '%s'", msg);
        throw std::runtime_error("asd3");
    }

    if (!loginResult->isLoginWasSuccessful())
    {
        connectionState = DISCONNECTED;

        if (loginResult->isLoginFailedDueToAccountInvalid())
        { std::cout << "account\n";
            throw "invalid username or password"; }

        if (loginResult->isLoginFailedDueToConnectionRestrictions())
        { std::cout << "connect\n";
            throw "connection restrictions"; }

        if (loginResult->isLoginFailedDueToInsufficientUserRights())
        { std::cout << "rights\n";
            throw "insufficient user rights"; }

        throw "unknown connection error";

    }


    connectionState = CONNECTED;

    resourceInteractionService = new IhcResourceInteractionService(ip);
    controllerService = new IhcControllerService(ip);
    controllerState = controllerService->getControllerState();
    //loadProject(); //Todo: insert again


}

public:

WSProjectInfo getProjectInfo()
{ return controllerService->getProjectInfo(); }

TiXmlDocument loadProject()
{
	std::cout << "Loading IHC /ELKO LS project file from controller...\n";
	TiXmlDocument doc = LoadProjectFileFromController();
	return doc;
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
std::string iso_8859_1_to_utf8(std::string str, size_t pos)
{
    std::string strOut;
    for (std::string::iterator it = str.begin(); it != str.end(); ++it)
    {
        uint8_t ch = *it;
        if (ch < 0x80) {
            strOut.push_back(ch);
        }
        else {
            strOut.push_back(0xc0 | ch >> 6);
            strOut.push_back(0x80 | (ch & 0x3f));
        }
    }
    return strOut;
}

TiXmlDocument LoadProjectFileFromController()
{
    WSProjectInfo projectInfo = getProjectInfo();
    int numberOfSegments = controllerService->getProjectNumberOfSegments();
    int segmentationSize = controllerService->getProjectSegmentationSize();

    std::vector<char> projectData;
    for (int i = 0; i < numberOfSegments; i++)
    {
#ifdef _DEBUG
        std::cout << "Downloading segment " << i << "..." << std::endl;
#endif
        WSFile data = controllerService->getProjectSegment(i, getProjectInfo().getProjectMajorRevision(), getProjectInfo().getProjectMinorRevision());

        std::vector<char> t = data.getData();
        projectData.insert(projectData.end(), t.begin(), t.end());

    }
#ifdef _DEBUG
    std::cout << "Final size: " << projectData.size() << std::endl;
#endif

    std::string str(projectData.begin(), projectData.end());
    std::string decoded = base64_decode(str);
    std::string extracted = decompress(decoded);

#ifdef _DEBUG
    std::cout << "Final size decoded: " << decoded.size() << std::endl;
    std::cout << "Final size extracted: " << extracted.size() << std::endl;

    std::ofstream out("project_file.vis");
    out << extracted;
    out.close();
#endif

    // Skip unsupported statements
    size_t pos = 0;
    while (true) {
        pos = extracted.find_first_of("<", pos);
        if (extracted[pos + 1] == '?' || // <?xml...
                extracted[pos + 1] == '!') { // <!DOCTYPE... or [<!ENTITY...
            // Skip this line
            pos = extracted.find_first_of("\n", pos);
        } else
            break;
    }

    extracted = iso_8859_1_to_utf8(extracted.substr(pos), pos);

    // Parse document as usual
    TiXmlDocument doc;
    doc.Parse(extracted.c_str());

    return doc;
}
public:

template <typename T> bool resourceUpdate(T value)
{
	return resourceInteractionService->resourceUpdate(value);
}

void enableRuntimeValueNotification(std::vector<int> resourceIdLis)
{
	resourceInteractionService->enableRuntimeValueNotifications(resourceIdLis);
}
};

#endif /* IHC_IHCCLIENT_HPP_ */
