/*
 * $Header: /home/dmarkle/xtrkcad-fork-cvs/xtrkcad/app/bin/misc2.c,v 1.1 2005-12-07 15:47:08 rc-flyer Exp $
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

#include <stdlib.h>
#include <stdio.h>
#ifndef WINDOWS
#include <unistd.h>
#include <dirent.h>
#endif
#include <malloc.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#include "track.h"
#include "common.h"
#include "utility.h"
#include "draw.h"
#include "misc.h"
#include "cjoin.h"
#include "compound.h"

EXPORT long units = 0;
EXPORT long checkPtInterval = 10;

EXPORT SCALEINX_T curScaleInx;
EXPORT DIST_T curScaleRatio;
EXPORT char * curScaleName;
EXPORT DIST_T trackGauge;
EXPORT char Title1[40] = "";
EXPORT char Title2[40] = "Title line 2";
EXPORT long labelScale = 8;
EXPORT long labelEnable = ((1<<0)|LABELENABLE_LENGTHS|LABELENABLE_ENDPT_ELEV|LABELENABLE_CARS);
EXPORT long labelWhen = 2;
EXPORT long colorLayers = 0;
EXPORT long hideSelectionWindow = 0;
EXPORT long angleSystem = 0;
EXPORT DIST_T minLength = 0.1;
EXPORT DIST_T connectDistance = 0.1;
EXPORT ANGLE_T connectAngle = 1.0;
EXPORT long twoRailScale = 16;
EXPORT long mapScale = 64;
EXPORT long liveMap = 0;
EXPORT long preSelect = 0;
EXPORT long listLabels = 7;
EXPORT long layoutLabels = 1;
EXPORT long descriptionFontSize = 72;
EXPORT long enableListPrices = 1;
EXPORT void ScaleLengthEnd(void);
static char minTrackRadiusPrefS[STR_SHORT_SIZE] = "minTrackRadius";

/****************************************************************************
 *
 * RPRINTF
 *
 */


#define RBUFF_SIZE (8192)
static char rbuff[RBUFF_SIZE+1];
static int roff;
static int rbuff_record = 0;

EXPORT void Rdump( FILE * outf )
{
	fprintf( outf, "Record Buffer:\n" );
	rbuff[RBUFF_SIZE] = '\0';
	fprintf( outf, rbuff+roff );
	rbuff[roff] = '\0';
	fprintf( outf, rbuff );
	memset( rbuff, 0, sizeof rbuff );
	roff = 0;
}


EXPORT void Rprintf(
		char * format,
		... )
{
	static char buff[STR_SIZE];
	char * cp;
	va_list ap;
	va_start( ap, format );
	vsprintf( buff, format, ap );
	va_end( ap );
	if (rbuff_record >= 1)
		lprintf( buff );
	for ( cp=buff; *cp; cp++ ) {
		rbuff[roff] = *cp;
		roff++;
		if (roff>=RBUFF_SIZE)
			roff=0;
	}
}

/****************************************************************************
 *
 * CHANGE NOTIFICATION
 *
 */


static changeNotificationCallBack_t changeNotificationCallBacks[20];
static int changeNotificationCallBackCnt = 0;
		
EXPORT void RegisterChangeNotification(
		changeNotificationCallBack_t action )
{
	changeNotificationCallBacks[changeNotificationCallBackCnt] = action;
	changeNotificationCallBackCnt++;
}


EXPORT void DoChangeNotification( long changes )
{
	int inx;
	for (inx=0;inx<changeNotificationCallBackCnt;inx++)
		changeNotificationCallBacks[inx](changes);
}


/****************************************************************************
 *
 * SCALE
 *
 */


#define SCALE_ANY	(-2)
#define SCALE_DEMO	(-1)

typedef struct {
		char * scale;
		DIST_T ratio;
		DIST_T gauge;
		DIST_T R[3];
		DIST_T X[3];
		DIST_T L[3];
		wIndex_t index;
		DIST_T length;
		BOOL_T tieDataValid;
		tieData_t tieData;
		} scaleInfo_t;
EXPORT typedef scaleInfo_t * scaleInfo_p;
static dynArr_t scaleInfo_da;
#define scaleInfo(N) DYNARR_N( scaleInfo_t, scaleInfo_da, N )
static tieData_t tieData_demo = {
		96.0/160.0,
		16.0/160.0,
		32.0/160.0 };

EXPORT SCALEINX_T curScaleInx = -1;
static scaleInfo_p curScale;
EXPORT long includeSameGaugeTurnouts = FALSE;
static SCALEINX_T demoScaleInx = -1;

EXPORT DIST_T GetScaleTrackGauge( SCALEINX_T si )
{
	return scaleInfo(si).gauge;
}

EXPORT DIST_T GetScaleRatio( SCALEINX_T si )
{
	return scaleInfo(si).ratio;
}

EXPORT char * GetScaleName( SCALEINX_T si )
{
	if ( si == -1 )
		return "DEMO";
	if ( si == SCALE_ANY )
		return "*";
	else if ( si < 0 || si >= scaleInfo_da.cnt )
		return "Unknown";
	else
		return scaleInfo(si).scale;
}

EXPORT void GetScaleEasementValues( DIST_T * R, DIST_T * L )
{
	wIndex_t i;
	for (i=0;i<3;i++) {
		*R++ = curScale->R[i];
		*L++ = curScale->L[i];
	}
}


EXPORT tieData_p GetScaleTieData( SCALEINX_T si )
{
	scaleInfo_p s;
	DIST_T defLength;

	if ( si == -1 )
		return &tieData_demo;
	else if ( si < 0 || si >= scaleInfo_da.cnt )
		return &tieData_demo;
	s = &scaleInfo(si);
	if ( !s->tieDataValid ) {
		sprintf( message, "tiedata-%s", s->scale );
		defLength = (96.0-54.0)/s->ratio+s->gauge;
		wPrefGetFloat( message, "length", &s->tieData.length, defLength );
		wPrefGetFloat( message, "width", &s->tieData.width, 16.0/s->ratio );
		wPrefGetFloat( message, "spacing", &s->tieData.spacing, 2*s->tieData.width );
	}
	return &scaleInfo(si).tieData;
}

EXPORT SCALEINX_T LookupScale( const char * name )
{
	wIndex_t si;
	DIST_T gauge;
	if ( strcmp( name, "*" ) == 0 )
		return SCALE_ANY;
	for ( si=0; si<scaleInfo_da.cnt; si++ ) {
		if (strcmp( scaleInfo(si).scale, name ) == 0)
			return si;
	}
	if ( isdigit(name[0]) ) {
		gauge = atof( name );
		for ( si=0; si<scaleInfo_da.cnt; si++ ) {
			if (scaleInfo(si).gauge == gauge)
				return si;
		}
	}
	NoticeMessage( MSG_BAD_SCALE_NAME, "Ok", NULL, name, sProdNameLower ); 
	si = scaleInfo_da.cnt;
	DYNARR_APPEND( scaleInfo_t, scaleInfo_da, 10 );
	scaleInfo(si) = scaleInfo(0);
	scaleInfo(si).scale = MyStrdup( name );
	return si;
}


EXPORT BOOL_T CompatibleScale(
	BOOL_T isTurnout,
	SCALEINX_T scale1,
	SCALEINX_T scale2 )
{
	if ( scale1 == scale2 )
		return TRUE;
	if ( scale1 == SCALE_DEMO || scale2 == SCALE_DEMO )
		return FALSE;
	if ( scale1 == demoScaleInx || scale2 == demoScaleInx )
		return FALSE;
	if ( isTurnout ) {
		if ( includeSameGaugeTurnouts &&
			 scaleInfo(scale1).gauge == scaleInfo(scale2).gauge )
			return TRUE;
	} else {
		if ( scale1 == SCALE_ANY )
			return TRUE;
		if ( scaleInfo(scale1).ratio == scaleInfo(scale2).ratio )
			return TRUE;
	}
	return FALSE;
}


static void SetScale(
		SCALEINX_T newScaleInx )
{
	if ( curScaleInx >= 0 )
		wPrefSetFloat( "misc", minTrackRadiusPrefS, minTrackRadius );
	if (newScaleInx < 0 && newScaleInx >= scaleInfo_da.cnt) {
		NoticeMessage( MSG_BAD_SCALE_INDEX, "Ok", NULL, (int)newScaleInx );
		return;
	}
	curScaleInx = (SCALEINX_T)newScaleInx;
	curScale = &scaleInfo(curScaleInx);
	trackGauge = curScale->gauge;
	curScaleRatio = curScale->ratio;
	curScaleName = curScale->scale;
	wPrefSetString( "misc", "scale", curScaleName );
	sprintf( minTrackRadiusPrefS, "minTrackRadius-%s", curScaleName );
	wPrefGetFloat( "misc", minTrackRadiusPrefS, &minTrackRadius, curScale->R[0] );
}


EXPORT BOOL_T DoSetScale(
		const char * newScale )
{
	SCALEINX_T scale;
	char * cp;
	int i30 = 29;
 
	curScaleInx = 0;
	i30++;
	if ( newScale != NULL ) {
		cp = CAST_AWAY_CONST newScale+strlen(newScale)-1;
		while ( *cp=='\n' || *cp==' ' || *cp=='\t' ) cp--; 
		cp[1] = '\0';
		while (isspace(*newScale)) newScale++;
		for (scale = 0; scale<scaleInfo_da.cnt; scale++) {
			if (strcasecmp( scaleInfo(scale).scale, newScale ) == 0) {
				curScaleInx = scale;
				break;
			}
		}
	}
	DoChangeNotification( CHANGE_SCALE );
	return TRUE;
}


static BOOL_T AddScale(
		char * line )
{
	wIndex_t i;
	BOOL_T rc;
	DIST_T R[3], X[3], L[3];
	DIST_T ratio, gauge;
	char scale[40];
	scaleInfo_p s;

	if ( (rc=sscanf( line, "SCALE %[^,]," SCANF_FLOAT_FORMAT "," SCANF_FLOAT_FORMAT "",
				scale, &ratio, &gauge )) != 3) {
		SyntaxError( "SCALE", rc, 3 );
		return FALSE;
	}
	for (i=0;i<3;i++) {
		line = GetNextLine();
		if ( (rc=sscanf( line, "" SCANF_FLOAT_FORMAT "," SCANF_FLOAT_FORMAT "," SCANF_FLOAT_FORMAT "",
				&R[i], &X[i], &L[i] )) != 3 ) {
			SyntaxError( "SCALE easement", rc, 3 );
			return FALSE;
		}
	}

	DYNARR_APPEND( scaleInfo_t, scaleInfo_da, 10 );
	s = &scaleInfo(scaleInfo_da.cnt-1);
	s->scale = MyStrdup( scale );
	s->ratio = ratio;
	s->gauge = gauge;
	s->index = -1;
	for (i=0; i<3; i++) {
		s->R[i] = R[i]/ratio;
		s->X[i] = X[i]/ratio;
		s->L[i] = L[i]/ratio;
	}
	s->tieDataValid = FALSE;
	if ( strcmp( scale, "DEMO" ) == 0 )
		demoScaleInx = scaleInfo_da.cnt-1;
	return TRUE;
}


EXPORT void ScaleLengthIncrement(
		SCALEINX_T scale,
		DIST_T length )
{
	char * cp;
	int len;
	if (scaleInfo(scale).length == 0.0) {
		if (units == UNITS_METRIC)
			cp = "999.99m SCALE Flex Track";
		else
			cp = "999' 11\" SCALE Flex Track";
		len = strlen( cp )+1;
		if (len > enumerateMaxDescLen)
			enumerateMaxDescLen = len;
	}
	scaleInfo(scale).length += length;
}

EXPORT void ScaleLengthEnd( void )
{
	wIndex_t si;
	int count;
	DIST_T length;
	char tmp[STR_SIZE];
	FLOAT_T flexLen;
	long flexUnit;
	FLOAT_T flexCost;
	for (si=0; si<scaleInfo_da.cnt; si++) {
		sprintf( tmp, "price list %s", scaleInfo(si).scale );
		wPrefGetFloat( tmp, "flex length", &flexLen, 0.0 );
		wPrefGetInteger( tmp, "flex unit", &flexUnit, 0 );
		wPrefGetFloat( tmp, "flex cost", &flexCost, 0.0 );
		tmp[0] = '\0';
		if ((length=scaleInfo(si).length) != 0) {
			sprintf( tmp, "%s %s Flex Track", FormatDistance(length), scaleInfo(si).scale );
			for (count = strlen(tmp); count<enumerateMaxDescLen; count++)
				tmp[count] = ' ';
			tmp[enumerateMaxDescLen] = '\0';
			count = 0;
			if (flexLen > 0.0) {
				count = (int)ceil( length / (flexLen/(flexUnit?2.54:1.00)));
			}
			EnumerateList( count, flexCost, tmp );
		}
		scaleInfo(si).length = 0;
	}
}


EXPORT void LoadScaleList( wList_p scaleList )
{
	wIndex_t inx;
	for (inx=0; inx<scaleInfo_da.cnt-(extraButtons?0:1); inx++) {
		scaleInfo(inx).index =
				wListAddValue( scaleList, scaleInfo(inx).scale, NULL, (void*)inx );
	}
}


static void ScaleChange( long changes )
{
	if (changes & CHANGE_SCALE) {
		SetScale( curScaleInx );
	}
}

/*****************************************************************************
 *
 * 
 *
 */

EXPORT void Misc2Init( void )
{
	AddParam( "SCALE ", AddScale );
	wPrefGetInteger( "draw", "label-when", &labelWhen, labelWhen );
	RegisterChangeNotification( ScaleChange );
	wPrefGetInteger( "misc", "include same gauge turnouts", &includeSameGaugeTurnouts, 1 );
}
