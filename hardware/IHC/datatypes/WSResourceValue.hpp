/*
 * WSResourceValue.hpp
 *
 *  Created on: 30. sep. 2016
 *      Author: Jonas Bo Jalling
 */

#ifndef IHC_WSRESOURCEVALUE_HPP_
#define IHC_WSRESOURCEVALUE_HPP_
#include <string>
#include <boost/variant.hpp>


struct RangedInteger {
    int min;
    int max;
    int value;

    explicit RangedInteger(int value, int min = 0, int max = 100)
      : min(min)
      , max(max)
      , value(value)
    {}
};

/**/

struct ToStringVisitor : boost::static_visitor<std::string> {
	int ID;

	ToStringVisitor(int const ID)
	: ID(ID)
	{}

    std::string operator()(bool const value) const{
    	std::string const val = (value) ? "true" : "false";
    	std::string const out = "Object " + boost::to_string(ID) + ", value \"" + val + "\"";
    	return out;
    }

    std::string operator()(RangedInteger const value) const{
    	std::string const out = "Object " + boost::to_string(ID) + ", value \"" + boost::lexical_cast<std::string>(value.value) + "\"";
        return out;
    }

    template<typename T>
    std::string operator()(T const&) const
    {
    	std::string const out = "Object " + boost::to_string(ID) + ", Unknown type";
        return out;
    }
};

struct XmlVisitor : boost::static_visitor<std::string> {
	int ID;

	XmlVisitor(int const ID)
	: ID(ID)
	{}

	std::string operator()(bool const value) const
	{
		std::string status;
		status = (value) ? "true" : "false";
		std::string query = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
		query += "<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">";
		query +=  "<soap:Body><setResourceValue1 xmlns=\"utcs\">";
		query +=  "  <value xmlns:q1=\"utcs.values\" xsi:type=\"q1:WSBooleanValue\"><q1:value>" + status + "</q1:value>";
		query +=  "  </value><resourceID>" + boost::to_string(ID) + "</resourceID><isValueRuntime>true</isValueRuntime>";
		query +=  " </setResourceValue1></soap:Body></soap:Envelope>";
return query;

		//std::cout << std::to_string(ID);
		//return "bool"; // TODO: XML
	}

	std::string operator()(RangedInteger const value) const
	{
		std::string query = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
			query += "<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">";
			query += "<soap:Body><setResourceValue1 xmlns=\"utcs\">";
			query += "  <value xmlns:q1=\"utcs.values\" xsi:type=\"q1:WSIntegerValue\">";
			query += "   <q1:maximumValue>" + boost::to_string(value.max) + "</q1:maximumValue><q1:minimumValue>" + boost::to_string(value.min) + "</q1:minimumValue>";
			query += "   <q1:integer>" + boost::to_string(value.value) + "</q1:integer></value><resourceID>" + boost::to_string(ID) + "</resourceID>";
			query += "  <isValueRuntime>true</isValueRuntime></setResourceValue1></soap:Body>";
			query += "</soap:Envelope>";
		/*std::string query = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
		        			query += "<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">";
		        			query +=  "<soap:Body><setResourceValue1 xmlns=\"utcs\">";
		        			query +=  "  <value xmlns:q1=\"utcs.values\" xsi:type=\"q1:WSBooleanValue\"><q1:value>" + std::to_string( value.value) + "</q1:value>";
		        			query +=  "  </value><resourceID>" + std::to_string(ID) + "</resourceID><isValueRuntime>true</isValueRuntime>";
		      			query +=  " </setResourceValue1></soap:Body></soap:Envelope>";*/
		        			return query;
		//return "integer"; // TODO: XML
	}

	template<typename T>
	    std::string operator()(T const&) const
	    {
	        return "default";
	    }
};





struct Null {};

struct ResourceValue {
    int ID;
    boost::variant<Null, bool, RangedInteger> value;

    explicit ResourceValue(int ID)
      : ID(ID)
    {}

    template<typename T>
    ResourceValue(int ID, T const& val)
      : ID(ID)
      , value(val)
    {}


    std::string toString() const
    {
        return boost::apply_visitor(ToStringVisitor(ID), value);
//        return boost::apply_visitor(ToStringVisitor{}, value);
    }

    std::string toXml( ) const
    {



    	return boost::apply_visitor( XmlVisitor(ID), value);
    }

};
/*
ResourceValue res(132456, RangedInteger(42));

res.value = RangedInteger(42);
res.value = true;

*/













class WSResourceValue
{
private:
	int resourceID;

public:
	virtual ~WSResourceValue(){};

	WSResourceValue (int resourceID = 0)
	: resourceID(resourceID)
	{}

	int getResourceID() const
	{ return resourceID; }

	void setResourceID(int resID)
	{ this->resourceID = resID; }

    virtual std::string toString() const = 0;

};

/**
 * <p>
 * C++ class for WSBooleanValue complex type.
 *
 * <p>
 * The following schema fragment specifies the expected content contained within
 * this class.
 *
 * <pre>
 * &lt;complexType name="WSBooleanValue">
 *   &lt;complexContent>
 *     &lt;extension base="{utcs.values}WSResourceValue">
 *       &lt;sequence>
 *         &lt;element name="value" type="{http://www.w3.org/2001/XMLSchema}boolean"/>
 *       &lt;/sequence>
 *     &lt;/extension>
 *   &lt;/complexContent>
 * &lt;/complexType>
 * </pre>
 *
 *
 */

// class WSBooleanValue : public  WSResourceValue {
// private:
//     bool value;
//
// public:
//     /**
// 	 * Gets the value of the value property.
// 	 *
// 	 */
//     bool isValue() const
//     { return value; }
//
//     /**
//      * Sets the value of the value property.
//      *
//      */
//     void setValue(bool val)
//     { this->value = val; }
//
//     std::string toString() const override
//     {
//         return (value) ? "true" : "false";
//     }
// };
//
// class WSIntegerValue : public WSResourceValue {
// private:
//     int integer;
//     int minValue;
//     int maxValue;
//
// public:
//     WSIntegerValue(int id = 0, int value = 0)
//       : WSResourceValue(id), integer(value)
//     {}
//
//     int getInteger () const
//     { return integer; }
//
//     void setInteger(int val)
//     { this->integer = val; }
//
//     int getMinValue() const
//     { return minValue; }
//
//     int getMaxValue() const
//     { return maxValue; }
//
//     void setMinValue(int val)
//     { this->minValue = val; }
//
//     int setMaxValue(int val)
//     { this->maxValue = val; }
//
//     std::string toString() const override
//     {
//         return boost::lexical_cast<std::string>(integer);
//     }
//
// };

#endif /* IHC_WSRESOURCEVALUE_HPP_ */
