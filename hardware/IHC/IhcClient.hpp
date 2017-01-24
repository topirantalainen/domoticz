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
ihcClient(std::string const &ip, std::string const &username, std::string const &password)
{
    this->ip = ip;
    this->username = username;
    this->password = password;
    this->connState = DISCONNECTED;
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

void openConnection()
{
    connState = CONNECTING;

    authenticationService = new IhcAuthenticationService(ip);
    WSLoginResult* loginResult = authenticationService->authenticate(username, password);
    if (!loginResult->isLoginWasSuccessful())
    {
        connState = DISCONNECTED;

        if (loginResult->isLoginFailedDueToAccountInvalid())
        { throw "invalid username or password"; }

        if (loginResult->isLoginFailedDueToConnectionRestrictions())
        { throw "connection restrictions"; }

        if (loginResult->isLoginFailedDueToInsufficientUserRights())
        { throw "insufficient user rights"; }

        throw "unknown connection error";

    }

    connState = CONNECTED;

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
    if (!projectFile.empty())
    {
        //TODO: Load from file
        //std::cout << "Loading IHC /ELKO LS project file from path " << projectFile << "\n";

    }
    else
    {
        std::cout << "Loading IHC /ELKO LS project file from controller...\n";
        TiXmlDocument doc = LoadProjectFileFromController();
        return doc;

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
void iso_8859_1_to_utf8(std::string &str, size_t pos)
{
    std::string strOut;
    for (std::string::iterator it = str.substr(pos).begin(); it != str.end(); ++it)
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
    str = strOut;
}

TiXmlDocument LoadProjectFileFromController()
{
    WSProjectInfo projectInfo = getProjectInfo();
    int numberOfSegments = controllerService->getProjectNumberOfSegments();
    int segmentationSize = controllerService->getProjectSegmentationSize();

    std::vector<char> asd;

    for (int i = 0; i < numberOfSegments; i++)
    {
#ifdef _DEBUG
        std::cout << "Downloading segment " << i << "..." << std::endl;
#endif
        WSFile data = controllerService->getProjectSegment(i, getProjectInfo().getProjectMajorRevision(), getProjectInfo().getProjectMinorRevision());

        std::vector<char> t = data.getData();
        asd.insert(asd.end(), t.begin(), t.end());

    }
#ifdef _DEBUG
    std::cout << "Final size: " << asd.size() << std::endl;
#endif

    std::string str(asd.begin(), asd.end());
    std::string decoded;
    decoded = b64decode(str);
    std::string extracted;
    extracted = decompress(decoded);
#ifdef _DEBUG
    std::cout << "Final size decoded: " << decoded.size() << std::endl;
    std::cout << "Final size extracted: " << extracted.size() << std::endl;

    std::ofstream out("output.txt");
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

    iso_8859_1_to_utf8(extracted, pos);

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
