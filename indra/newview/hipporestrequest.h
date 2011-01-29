#ifndef __HIPPO_REST_REQUEST_H__
#define __HIPPO_REST_REQUEST_H__


#include <string>


class HippoRestRequest
{
	public:
		static int getBlocking(const std::string &url, std::string *result);
	
};


#endif
