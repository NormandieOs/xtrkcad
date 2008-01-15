/*  XTrkCad - Model Railroad CAD
 *  Copyright (C) 2007 Mikko Nissinen
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef XTC_USE_GETTEXT
/* Use gettext */
#include <libintl.h>
#include <string.h>

#define _(String)             ((String && strlen(String) > 0) \
                              ? gettext(String) : String)
#define gettext_noop(String)  String
#define N_(String)            gettext_noop(String)
#else
/* Don't use gettext */
#define _(String)             String
#define gettext_noop(String)  String
#define N_(String)            String
#endif /* XTC_USE_GETTEXT */

void InitGettext( void );
