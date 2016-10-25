/*
 * WSLoginResult.hpp
 *
 *  Created on: 30. sep. 2016
 *      Author: Jonas Bo Jalling
 */

#ifndef IHC_WSLOGINRESULT_HPP_
#define IHC_WSLOGINRESULT_HPP_

class WSLoginResult : public WSBaseDataType
{
private:
    WSUser loggedInUser;
    bool loginWasSuccessful;
    bool loginFailedDueToConnectionRestrictions;
    bool loginFailedDueToInsufficientUserRights;
    bool loginFailedDueToAccountInvalid;

public:
    WSLoginResult() { }

    /**
     * Gets the value of the loggedInUser property.
     *
     * @return
     *         possible object is
     *         {@link WSUser }
     *
     */
    WSUser getLoggedInUser()
    { return loggedInUser; }

    /**
     * Sets the value of the loggedInUser property.
     *
     * @param value
     *            allowed object is
     *            {@link WSUser }
     *
     */
    void setLoggedInUser(WSUser value)
    { this->loggedInUser = value; }

    /**
     * Gets the value of the loginWasSuccessful property.
     *
     */
    bool isLoginWasSuccessful()
    { return loginWasSuccessful; }

    /**
     * Sets the value of the loginWasSuccessful property.
     *
     */
    void setLoginWasSuccessful(bool value)
    { this->loginWasSuccessful = value; }

    /**
     * Gets the value of the loginFailedDueToConnectionRestrictions property.
     *
     */
    bool isLoginFailedDueToConnectionRestrictions()
    { return loginFailedDueToConnectionRestrictions; }

    /**
     * Sets the value of the loginFailedDueToConnectionRestrictions property.
     *
     */
    void setLoginFailedDueToConnectionRestrictions(bool value)
    { this->loginFailedDueToConnectionRestrictions = value; }

    /**
     * Gets the value of the loginFailedDueToInsufficientUserRights property.
     *
     */
    bool isLoginFailedDueToInsufficientUserRights()
    { return loginFailedDueToInsufficientUserRights; }

    /**
     * Sets the value of the loginFailedDueToInsufficientUserRights property.
     *
     */
    void setLoginFailedDueToInsufficientUserRights(bool value)
    { this->loginFailedDueToInsufficientUserRights = value; }

    /**
     * Gets the value of the loginFailedDueToAccountInvalid property.
     *
     */
    bool isLoginFailedDueToAccountInvalid()
    { return loginFailedDueToAccountInvalid; }

    /**
     * Sets the value of the loginFailedDueToAccountInvalid property.
     *
     */
    void setLoginFailedDueToAccountInvalid(bool value)
    { this->loginFailedDueToAccountInvalid = value; }

    //@Override
    void encodeData(TiXmlDocument data)
    {

        loginWasSuccessful = parseValueToBoolean(data,
                "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:authenticate2/ns1:loginWasSuccessful");

        loginFailedDueToConnectionRestrictions = parseValueToBoolean(data,
                "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:authenticate2/ns1:loginFailedDueToConnectionRestrictions");

        loginFailedDueToInsufficientUserRights = parseValueToBoolean(data,
                "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:authenticate2/ns1:loginFailedDueToInsufficientUserRights");

        loginFailedDueToAccountInvalid = parseValueToBoolean(data,
                "/SOAP-ENV:Envelope/SOAP-ENV:Body/ns1:authenticate2/ns1:loginFailedDueToAccountInvalid");
    }

};

#endif /* IHC_WSLOGINRESULT_HPP_ */
