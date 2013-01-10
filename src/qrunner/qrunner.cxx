// ----------------------------------------------------------------------------
//	qrunner.cxx
//
// Copyright (C) 2007-2009
//		Stelios Bounanos, M0GLD
//
// This file is part of fldigi.
//
// fldigi is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// fldigi is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// ----------------------------------------------------------------------------

#include <config.h>

#include <unistd.h>
#include <errno.h>
#if defined(__CYGWIN__)
#  include <sys/types.h>
#  include <sys/socket.h>
#elif defined(__MINGW32__)
#  include "compat.h"
#endif
#include <fcntl.h>

#include <FL/Fl.H>

#include "fqueue.h"
#include "qrunner.h"

//Remi's advice for FIFO full issue #define FIFO_SIZE 2048
#define FIFO_SIZE 8192

#ifndef __MINGW32__
#  define QRUNNER_EAGAIN() (errno == EAGAIN)
#else
#  define QRUNNER_EAGAIN() ((errno = WSAGetLastError()) == WSAEWOULDBLOCK)
#endif

qrunner::qrunner()
        : attached(false), inprog(false), drop_flag(false)
{
        fifo = new fqueue(FIFO_SIZE);
#ifndef __WOE32__
        if (pipe(pfd) == -1)
#else
	if (socketpair(PF_INET, SOCK_STREAM, 0, pfd) == -1)
#endif
                throw qexception(errno);
	set_cloexec(pfd[0], 1);
	set_cloexec(pfd[1], 1);
	if (set_nonblock(pfd[0], 1) == -1)
		throw qexception(errno);
#ifdef __WOE32__
	set_nodelay(pfd[1], 1);
#endif
}

qrunner::~qrunner()
{
        detach();
        close(pfd[0]);
        close(pfd[1]);
        delete fifo;
}

void qrunner::attach(void)
{
        Fl::add_fd(pfd[0], FL_READ, qrunner::execute, this);
        attached = true;
}
void qrunner::detach(void)
{
        attached = false;
        Fl::remove_fd(pfd[0], FL_READ);
}

static unsigned char rbuf[FIFO_SIZE];

void qrunner::execute(int fd, void *arg)
{
        qrunner *qr = reinterpret_cast<qrunner *>(arg);

	if (qr->inprog)
		return;
	qr->inprog = true;

	size_t n = QRUNNER_READ(fd, rbuf, FIFO_SIZE);
	switch (n) {
	case -1:
		if (!QRUNNER_EAGAIN())
			throw qexception(errno);
		// else fall through
	case 0:
		break;
	default:
		while (n--)
			qr->fifo->execute();
	}

	qr->inprog = false;
}

void qrunner::flush(void)
{
        execute(pfd[0], this);
}
