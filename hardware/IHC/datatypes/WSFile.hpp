/*
 * WSFile.hpp
 *
 *  Created on: 1. okt. 2016
 *      Author: Jonas Bo Jalling
 */

#ifndef IHC_DATATYPES_WSFILE_HPP_
#define IHC_DATATYPES_WSFILE_HPP_

class WSFile : public WSBaseDataType {
    private:
	std::vector<char> data;
    std::string filename;
public:
    WSFile() {
    }

    WSFile(std::vector<char> data, std::string filename)
    {
    	this->data = data;
    	this->filename = filename;
    }

    /**
     * Gets the data value for this WSFile.
     *
     * @return data
     */
    std::vector<char> getData() {
        return data;
    }

    /**
     * Sets the data value for this WSFile.
     *
     * @param data
     */
    void setData(std::vector<char> data) {
        this->data = data;
    }

    /**
     * Gets the filename value for this WSFile.
     *
     * @return filename
     */
    std::string getFilename() {
        return filename;
    }

    /**
     * Sets the filename value for this WSFile.
     *
     * @param filename
     */
    void setFilename(std::string filename) {
        this->filename = filename;
    }

    void encodeData(TiXmlDocument data)
    {
        filename = parseValue(data, "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:getIHCProjectSegment4/ns1:filename");
        std::string str =parseValue(data, "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:getIHCProjectSegment4/ns1:data");
        std::copy(str.begin(), str.end(), std::back_inserter(this->data));
    }

};
#endif /* IHC_DATATYPES_WSFILE_HPP_ */
