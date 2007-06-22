/*
 *    rttym.h  --  BAUDOT encoder/decoder
 *
 *    Copyright (C) 2001, 2002, 2003
 *      Tomi Manninen (oh2bns@sral.fi)
 *
 *    This file is part of gMFSK.
 *
 *    gMFSK is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    gMFSK is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with gMFSK; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define	RTTYM_LETS	0x100
#define	RTTYM_FIGS	0x200

extern int rttym_enc(unsigned char data);
extern int rttym_dec(int *mode, unsigned char data);

#ifdef __cplusplus
}
#endif /* __cplusplus */