#include <sys/time.h>
#include <cmath>
#include <string>

#include "network.h"
#include "socket.h"
#include "re.h"

using namespace std;

bool request_reply(const string& node, const string& service, const string& request, string& reply, double timeout)
{
	try {
		Socket s(Address(node.c_str(), service.c_str()));
		s.connect();
		s.set_nonblocking();
		s.set_timeout(timeout);

		if (s.send(request) != request.length() || s.recv(reply) == 0) {
		        reply = "Request timed out";
			return false;
		}
	}
	catch (const SocketException& e) {
		reply = e.what();
		return false;
	}

	return true;
}

bool fetch_http(const string& url, string& reply, double timeout)
{
	re_t http_re("http://([^/]+)(.+)", REG_EXTENDED | REG_ICASE);

	if (!http_re.match(url.c_str()) || http_re.nsub() != 3)
		return false;

	string request;
	request.append("GET ").append(http_re.submatch(2)).append(" HTTP/1.0\n")
	       .append("Host: ").append(http_re.submatch(1)).append("\nConnection: close\n\n");

	return request_reply(http_re.submatch(1), "http", request, reply, timeout);
}
