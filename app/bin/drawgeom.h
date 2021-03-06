/** \file drawgeom.h
 *
 */
/*  XTrkCad - Model Railroad CAD
 *  Copyright (C) 2005 Dave Bullis
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

#ifndef HAVE_DRAWGEOM_H
#define HAVE_DRAWGEOM_H

#include "ccurve.h"
#include "common.h"
#include "draw.h"
#include "track.h"
#include "wlib.h"

#define OP_LINE			(0)
#define OP_DIMLINE		(1)
#define OP_BENCH		(2)
#define OP_TBLEDGE		(3)
#define OP_CURVE1		(4)
#define OP_CURVE2		(5)
#define OP_CURVE3		(6)
#define OP_CURVE4		(7)
#define OP_CIRCLE1		(8)
#define OP_CIRCLE2		(9)
#define OP_CIRCLE3		(10)
#define OP_BOX			(11)
#define OP_POLY			(12)
#define OP_FILLCIRCLE1	(13)
#define OP_FILLCIRCLE2	(14)
#define OP_FILLCIRCLE3	(15)
#define OP_FILLBOX		(16)
#define OP_FILLPOLY		(17)
#define OP_BEZLIN       (18)
#define OP_POLYLINE     (19)
#define OP_LAST			(OP_POLYLINE)

typedef struct {
				void (*message)( char *, ... );
				void (*Redraw)( void );
				drawCmd_t *D;
				long Op;
				wDrawColor Color;
				long line_Width;
				double width;
				ANGLE_T angle;
				double length;
				double radius;
				long benchOption;
				drawLineType_e lineType;
				int State;
				int index;
				curveData_t ArcData;
				ANGLE_T ArcAngle;
				int Started;
				BOOL_T Changed;
		} drawContext_t;

typedef enum {MOD_NONE, MOD_STARTED, MOD_SELECTED_PT, MOD_AFTER_PT,
		MOD_ORIGIN, MOD_AFTER_ORIG } ModState_e;

typedef struct {
				void (*message)( char *, ... );
				void (*Redraw)( void );
				drawCmd_t *D;
				double length;
				ANGLE_T rel_angle;
				double radius;
				ANGLE_T arc_angle;
				int last_inx;
				ANGLE_T abs_angle;
				double height;
				double width;
				int prev_inx;
				int max_inx;
				track_p trk;
				char type;
				coOrd orig;			//Origin Pos
				ANGLE_T angle;      //Origin Angle
				wIndex_t segCnt;
				trkSeg_p segPtr;
				wBool_t selected;
				wBool_t circle;
				ModState_e state;
				coOrd rel_center;
				coOrd rot_center;
				wBool_t rot_moved;
				coOrd translate_center;
				coOrd moved;
				coOrd arm;
				coOrd new_arm;
				ANGLE_T rot_angle;
				coOrd p0;
				coOrd p1;
				coOrd pm;
				coOrd pc;
				DIST_T disp;
				wBool_t rotate_state;
				wBool_t open;
				wBool_t filled;
				PolyType_e subtype;
		} drawModContext_t;

typedef enum {LENGTH_UPDATE, WIDTH_UPDATE} drawUpdateType_e;

extern drawContext_t * drawContext;
extern wDrawColor lineColor;
extern long lineWidth;

void DrawGeomOp( void * );
STATUS_T DrawGeomMouse( wAction_t, coOrd, drawContext_t *);
STATUS_T DrawGeomModify( wAction_t, coOrd, drawModContext_t * );
STATUS_T DrawGeomOriginMove(wAction_t, coOrd, drawModContext_t * );
#endif //HAVE_DRAWGEOM_H
