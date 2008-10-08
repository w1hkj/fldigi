#ifndef NETWORK_H_
#define NETWORK_H_

#include <string>

bool request_reply(const std::string& node, const std::string& service,
		   const std::string& request, std::string& reply, double timeout = 0.0);
bool fetch_http(const std::string& url, std::string& reply, double timeout = 0.0);

#endif // NETWORK_H_
