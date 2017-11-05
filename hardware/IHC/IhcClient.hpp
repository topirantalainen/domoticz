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
#include "zlib.h"

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
	//std::string dumpResourceToFile;

	IhcAuthenticationService* authenticationService;
	IhcResourceInteractionService* resourceInteractionService;
	IhcControllerService* controllerService;
	IhcWireless* wire;
	WSControllerState controllerState;

public:
ihcClient(std::string const &ip, std::string const &username, std::string const &password)
{
    this->ip = ip;
    this->username = username;
    this->password = password;
    this->connState = DISCONNECTED;
}

ResourceValue resourceQuery(int const &res)
{ return resourceInteractionService->resourceQuery(res); }

void enableRuntimeValueNotifications2(std::vector<int> resourceIdList)
{ resourceInteractionService->enableRuntimeValueNotifications(resourceIdList); }

std::vector<boost::shared_ptr<ResourceValue> > waitResourceValueNotifications(int const timeout)
{ return resourceInteractionService->waitResourceValueNotifications(timeout); }

void reset()
{
	connState = DISCONNECTED;
}

void openConnection()
{
    connState = CONNECTING;

    authenticationService = new IhcAuthenticationService(ip);

    WSLoginResult* loginResult;
    try
    {
         loginResult= authenticationService->authenticate(username, password);
    }
    catch (...)
    {
        throw std::runtime_error("asd3");
    }

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
    wire = new IhcWireless(ip);
    //loadProject(); //Todo: insert again


}

public:
TiXmlDocument getRF()
{
	return wire->getRF();
}

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
        TiXmlDocument const doc = LoadProjectFileFromController();
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

/* From: https://panthema.net/2007/0328-ZLibString.html */
std::string decompress_string(const std::string& str)
{
    z_stream zs;                        // z_stream is zlib's control structure
    memset(&zs, 0, sizeof(zs));

    if (inflateInit2(&zs, 16+MAX_WBITS) != Z_OK)
        throw(std::runtime_error("inflateInit failed while decompressing."));

    zs.next_in = (Bytef*)str.data();
    zs.avail_in = str.size();

    int ret;
    char outbuffer[32768];
    std::string outstring;

    // get the decompressed bytes blockwise using repeated calls to inflate
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = inflate(&zs, 0);

        if (outstring.size() < zs.total_out) {
            outstring.append(outbuffer,
                             zs.total_out - outstring.size());
        }

    } while (ret == Z_OK);

    inflateEnd(&zs);

    if (ret != Z_STREAM_END) {          // an error occurred that was not EOF
        std::ostringstream oss;
        oss << "Exception during zlib decompression: (" << ret << ") "
            << zs.msg;
        throw(std::runtime_error(oss.str()));
    }

    return outstring;
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
    WSProjectInfo const projectInfo = getProjectInfo();

    std::vector<char> projectData;
    for (int i = 0; i < controllerService->getProjectNumberOfSegments(); i++)
    {
#ifdef _DEBUG
        std::cout << "Downloading segment " << i << "..." << std::endl;
#endif
        WSFile data = controllerService->getProjectSegment(i, getProjectInfo().getProjectMajorRevision(), getProjectInfo().getProjectMinorRevision());

        std::vector<char> const t = data.getData();
        projectData.insert(projectData.end(), t.begin(), t.end());

    }
#ifdef _DEBUG
    std::cout << "Final size: " << projectData.size() << std::endl;
#endif

    std::string const str(projectData.begin(), projectData.end());

    std::string decompressed_project = decompress_string(b64decode(str));

#ifdef _DEBUG
    std::cout << "Final size decoded: " << decoded.size() << std::endl;
    std::cout << "Final size extracted: " << decompressed_project.size() << std::endl;

    std::ofstream out("output.txt");
    out << decompressed_project;
    out.close();
#endif
    // Skip unsupported statements
    size_t pos = 0;
    while (true) {
        pos = decompressed_project.find_first_of("<", pos);
        if (decompressed_project[pos + 1] == '?' || // <?xml...
                decompressed_project[pos + 1] == '!') { // <!DOCTYPE... or [<!ENTITY...
            // Skip this line
            pos = decompressed_project.find_first_of("\n", pos);
        } else
            break;
    }

    decompressed_project = iso_8859_1_to_utf8(decompressed_project.substr(pos), pos);

    // Parse document as usual
    TiXmlDocument doc;
    doc.Parse(decompressed_project.c_str());

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
