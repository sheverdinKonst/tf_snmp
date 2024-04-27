/* -----------------------------------------------------------------------------
 * SNMP implementation for Contiki
 *
 * Copyright (C) 2010 Siarhei Kuryla
 *
 * This program is part of free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

/**
 * \file
 *         Key tools.
 * \author
 *         Siarhei Kuryla <kurilo@gmail.com>
 */

#ifndef __KEYTOOLS_H__
#define	__KEYTOOLS_H__

#include "snmpd-types.h"
#include "snmpd-conf.h"

#if ENABLE_SNMPv3

void password_to_key_md5(u8t *pwd, u8t pwd_len, u8t *engineID, u8t engineLength, u8t *key);

u8t* getAuthKul();
u8t* getPrivKul();

void setAuthKul(u8t *key);
void setPrivKul(u8t *key);
#endif

#endif	/* __KEYTOOLS_H__ */

