#include <config.h>

#include <sys/time.h>
#include <cmath>
#include <string>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#include <FL/Fl.H>

#include "network.h"
#include "socket.h"
#include "re.h"
#include "timeops.h"
#include "gettext.h"

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

struct fetch_data_t {
	string url;
	string reply;
	double timeout;
	bool result;
	bool finished;
	pthread_t thread;
};

static void* fetch_url_thread(void* data)
{
	fetch_data_t* p = reinterpret_cast<fetch_data_t*>(data);
	p->result = fetch_http(p->url, p->reply, p->timeout);
	p->finished = true;

	return NULL;
}

static void fetch_url_cleanup(void* data)
{
	fetch_data_t* p = reinterpret_cast<fetch_data_t*>(data);
	if (p->finished)
		delete p;
	else
		Fl::repeat_timeout(1.0, fetch_url_cleanup, data);
}

// Like fetch_http, but calls Fl::wait and busy() until the time out has elapsed,
// done() before returning
bool fetch_http_gui(const string& url, string& reply, double timeout,
		    void(*busy)(void*), void* arg1, void(*done)(void*), void* arg2)
{
	fetch_data_t* data = new fetch_data_t;
	data->url = url;
	data->timeout = timeout;
	data->finished = false;

	struct timespec ts[2];
	clock_gettime(CLOCK_MONOTONIC, &ts[0]);

	if (pthread_create(&data->thread, 0, fetch_url_thread, data)) {
		reply = strerror(errno);
		delete data;
		return false;
	}
	while (!data->finished && Fl::wait(timeout) >= 0.0) {
		if (Fl::event() == FL_KEYUP && Fl::event_key() == FL_Escape)
			break;
		clock_gettime(CLOCK_MONOTONIC, &ts[1]);
		ts[1] -= ts[0];
		if (timeout - ts[1].tv_sec + ts[1].tv_nsec / 1e9 <= 0.0)
			break;
		if (busy)
			busy(arg1);
	}
	if (done)
		done(arg2);

	bool ret;
	if (data->finished) {
		reply = data->reply;
		ret = data->result;
		delete data;
	}
	else {
		Fl::add_timeout(1.0, fetch_url_cleanup, data);
		reply = (timeout >= 0.0 ? _("Aborted") : _("Timed out"));
		ret = false;
	}

	return ret;
}
