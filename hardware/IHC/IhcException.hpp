/*
 * IhcException.hpp
 *
 *  Created on: 7. okt. 2016
 *      Author: Jonas Bo Jalling
 */

#ifndef IHC_IHCEXCEPTION_HPP_
#define IHC_IHCEXCEPTION_HPP_

// using standard exceptions
#include <iostream>
#include <exception>

class myexception: public exception
{
  virtual const char* what() const throw()
  {
    return "My exception happened";
  }
} myex;

int main () {
  try
  {
    throw myex;
  }
  catch (exception& e)
  {
    cout << e.what() << '\n';
  }
  return 0;
}


#endif /* IHC_IHCEXCEPTION_HPP_ */
