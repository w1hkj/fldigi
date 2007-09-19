// ----------------------------------------------------------------------------
//	qrunner.cxx
//
// Copyright (C) 2007
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

#include <unistd.h>
// #include <fcntl.h>
#include <errno.h>
// #include <cstring>

#include <FL/Fl.h>

#include "fqueue.h"
#include "qrunner.h"

qrunner::qrunner(size_t npri_)
        : npri(npri_), attached(false)
{
        fifo = new fqueue(2048, npri);
        if (pipe(pfd) == -1)
                throw qexception(errno);

        // int flags;
        // if ((flags = fcntl(pfd[0], F_GETFL)) == -1)
        //         throw qexception(errno);
        // if (fcntl(pfd[0], F_SETFL, flags | O_NONBLOCK) == -1)
        //         throw qexception(errno);
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

void qrunner::execute(int fd, void *arg)
{
        qrunner *qr = static_cast<qrunner *>(arg);

        char c;
        while (qr->fifo->execute()) {
                if (read(fd, &c, 1) == -1 /*&& errno != EWOULDBLOCK*/)
                        throw qexception(errno);
        }
}


qrunner *cbq[NUM_THREADS];
