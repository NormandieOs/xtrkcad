/*
 * $Header: /home/dmarkle/xtrkcad-fork-cvs/xtrkcad/app/bin/fileio.c,v 1.4 2006-03-26 12:02:50 m_fischer Exp $
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
#include <errno.h>
#endif
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#ifdef WINDOWS
#include <io.h>
#include <windows.h>
#else
#include <sys/stat.h>
#endif
#include <stdarg.h>
#include <locale.h>
#include <dirent.h>

#include "track.h"
#include "version.h"
#include "common.h"
#include "utility.h"
#include "draw.h"
#include "misc.h"
#include "compound.h"

/*#define TIME_READTRACKFILE*/

EXPORT const char * workingDir;
EXPORT const char * libDir;

static char * customPath = NULL;
static char * customPathBak = NULL;

EXPORT char curPathName[STR_LONG_SIZE];
EXPORT char * curFileName;
EXPORT char curDirName[STR_LONG_SIZE];

EXPORT char * clipBoardN;

EXPORT wBool_t executableOk = FALSE;

static int log_paramFile;

EXPORT void SetCurDir(
		const char * pathName,
		const char * fileName )
{
	memcpy( curDirName, pathName, fileName-pathName );
	curDirName[fileName-pathName-1] = '\0';
	wPrefSetString( "file", "directory", curDirName );
}

#ifdef WINDOWS
#define rename( F1, F2 ) Copyfile( F1, F2 )

static int Copyfile( char * fn1, char * fn2 )
{
	FILE *f1, *f2;
	size_t size;
	f1 = fopen( fn1, "r" );
	if ( f1 == NULL )
		return 0;
	f2 = fopen( fn2, "w" );
	if ( f2 == NULL ) {
		fclose( f1 );
		return -1;
	}
	while ( (size=fread( message, 1, sizeof message, f1 )) > 0 )
		fwrite( message, size, 1, f2 );
	fclose( f1 );
	fclose( f2 );
	return 0;
}
#endif

/****************************************************************************
 *
 * PARAM FILE INPUT
 *
 */

EXPORT FILE * paramFile = NULL;
EXPORT char paramFileName[STR_LONG_SIZE];
EXPORT wIndex_t paramLineNum = 0;
EXPORT char paramLine[STR_LONG_SIZE];
EXPORT char * curContents;
EXPORT char * curSubContents;
static long paramCheckSum;

#define PARAM_DEMO (-1)

typedef struct {
		char * name;
		readParam_t proc;
		} paramProc_t;
static dynArr_t paramProc_da;
#define paramProc(N) DYNARR_N( paramProc_t, paramProc_da, N )


EXPORT void Stripcr( char * line )
{
	char * cp;
	cp = line + strlen(line);
	if (cp == line)
		return;
	cp--;
	if (*cp == '\n')
		*cp-- = '\0';
	if (cp >= line && *cp == '\r')
		*cp = '\0';
}

EXPORT void ParamCheckSumLine( char * line )
{
	long mult=1;
	while ( *line )
		paramCheckSum += (((long)(*line++))&0xFF)*(mult++);
}

EXPORT char * GetNextLine( void )
{
	if (!paramFile) {
		paramLine[0] = '\0';
		return NULL;
	}
	if (fgets( paramLine, sizeof paramLine, paramFile ) == NULL) {
		AbortProg( "Permature EOF on %s", paramFileName );
	}
	Stripcr( paramLine );
	ParamCheckSumLine( paramLine );
	paramLineNum++;
	return paramLine;
}


EXPORT void InputError(
		char * msg,
		BOOL_T showLine,
		... )
{
	va_list ap;
	char * mp = message;
	mp += sprintf( message, "INPUT ERROR: %s:%d\n",
		paramFileName, paramLineNum );
	va_start( ap, showLine );
	mp += vsprintf( mp, msg, ap );
	va_end( ap );
	if (showLine) {
		*mp++ = '\n';
		strcpy( mp, paramLine );
	}
	strcat( mp, "\nDo you want to continue?" );
	if (!wNotice( message, "Continue", "Stop" )) {
		if ( paramFile ) fclose(paramFile);
		paramFile = NULL;
	}
}


EXPORT void SyntaxError(
		char * event,
		wIndex_t actual,
		wIndex_t expected )
{
	InputError( "%s scan returned %d (expected %d)",
		TRUE, event, actual, expected );
}


EXPORT BOOL_T GetArgs(
		char * line,
		char * format,
		... )
{
	char * cp, * cq;
	int argNo;
	long * pl;
	int * pi;
	FLOAT_T *pf;
	coOrd p, *pp;
	char * ps;
	char ** qp;
	va_list ap;
	cp = line;
	va_start( ap, format );
	for (argNo=1;*format;argNo++,format++) {
		while (isspace(*cp)) cp++;
		if (!*cp && strchr( "XZYzc", *format ) == NULL ) {
			InputError( "Arg %d: EOL unexpected", TRUE, argNo );
			return FALSE;
		}
		switch (*format) {
		case '0':
			(void)strtol( cp, &cq, 10 );
			if (cp == cq) {
				InputError( "Arg %d: expected integer", TRUE, argNo );
				return FALSE;
			}
			cp = cq;
			break;
		case 'X':
			pi = va_arg( ap, int * );
			*pi = 0;
			break;
		case 'Z':
			pl = va_arg( ap, long * );
			*pl = 0;
			break;
		case 'Y':
			pf = va_arg( ap, FLOAT_T * );
			*pf = 0;
			break;
		case 'L':
			pi = va_arg( ap, int * );
			*pi = (int)strtol( cp, &cq, 10 );
			if (cp == cq) {
				InputError( "Arg %d: expected integer", TRUE, argNo );
				return FALSE;
			}
			cp = cq;
			break;
		case 'd':
			pi = va_arg( ap, int * );
			*pi = (int)strtol( cp, &cq, 10 );
			if (cp == cq) {
				InputError( "Arg %d: expected integer", TRUE, argNo );
				return FALSE;
			}
			cp = cq;
			break;
		case 'w':
			pf = va_arg( ap, FLOAT_T * );
			*pf = (FLOAT_T)strtol( cp, &cq, 10 );
			if (cp == cq) {
				InputError( "Arg %d: expected integer", TRUE, argNo );
				return FALSE;
			}
			if (*cq == '.')
				*pf = strtod( cp, &cq );
			else
				*pf /= mainD.dpi;
			cp = cq;
			break;
		case 'l':
			pl = va_arg( ap, long * );
			*pl = strtol( cp, &cq, 10 );
			if (cp == cq) {
				InputError( "Arg %d: expected integer", TRUE, argNo );
				return FALSE;
			}
			cp = cq;
			break;
		case 'f':
			pf = va_arg( ap, FLOAT_T * );
			*pf = strtod( cp, &cq );
			if (cp == cq) {
				InputError( "Arg %d: expected float", TRUE, argNo );
				return FALSE;
			}
			cp = cq;
			break;
		case 'z':
			pf = va_arg( ap, FLOAT_T * );
#ifdef LATER
			if ( paramVersion >= 9 ) {
				*pf = strtod( cp, &cq );
				if (cp == cq) {
					InputError( "Arg %d: expected float", TRUE, argNo );
					return FALSE;
				}
				cp = cq;
			} else {
				*pf = 0.0;
			}
#endif
			*pf = 0.0;
			break;
		case 'p':
			pp = va_arg( ap, coOrd * );
			p.x = strtod( cp, &cq );
			if (cp == cq) {
				InputError( "Arg %d: expected float", TRUE, argNo );
				return FALSE;
			}
			cp = cq;
			p.y = strtod( cp, &cq );
			if (cp == cq) {
				InputError( "Arg %d: expected float", TRUE, argNo );
				return FALSE;
			}
			cp = cq;
			*pp = p;
			break;
		case 's':
			ps = va_arg( ap, char * );
			while (isspace(*cp)) cp++;
			while (*cp && !isspace(*cp)) *ps++ = *cp++;
			*ps++ = '\0';
			break;
		case 'q':
			qp = va_arg( ap, char * * );
			if (*cp != '\"')
				/* Stupid windows */
				cq = strchr( cp, '\"' );
			else
				cq = cp;
			if (cq!=NULL) {
				cp = cq;
				ps = &message[0];
				cp++;
				while (*cp) {
					if ( (ps-message)>=sizeof message)
						AbortProg( "Quoted title argument too long" );
					if (*cp == '\"') {
						if (*++cp == '\"') {
							*ps++ = '\"';
						} else {
							*ps = '\0';
							cp++;
							break;
						}
					} else {
						*ps++ = *cp;
					}
					cp++;
				}
				*ps = '\0';
			} else {
				message[0] = '\0';
			}
			*qp = (char*)MyStrdup(message);
			break;
		case 'c':
			qp = va_arg( ap, char * * );
			while (isspace(*cp)) cp++;
			if (*cp)
				*qp = cp;
			else
				*qp = NULL;
			break;
		default:
			AbortProg( "getArgs: bad format char" );
		}
	}
	va_end( ap );
	return TRUE;
}

EXPORT wBool_t ParseRoomSize(
		char * s,
		coOrd * roomSizeRet )
{
	coOrd size;
	char *cp;

	size.x = strtod( s, &cp );
	if (cp != s) {
		s = cp;
		while (isspace(*s)) s++;
		if (*s == 'x' || *s == 'X') {
			size.y = strtod( ++s, &cp );
			if (cp != s) {
#ifdef LATER
				if (units == UNITS_METRIC) {
					size.x /= 2.54;
					size.y /= 2.54;
				}
#endif
				*roomSizeRet = size;
				return TRUE;
			}
		}
	}
	return FALSE;
}


EXPORT void AddParam(
		char * name,
		readParam_t proc )
{
	DYNARR_APPEND( paramProc_t, paramProc_da, 10 );
	paramProc(paramProc_da.cnt-1).name = name;
	paramProc(paramProc_da.cnt-1).proc = proc;
}


EXPORT BOOL_T ReadParams(
		long key,
		const char * dirName,
		const char * fileName )
{
	FILE * oldFile;
	char *cp;
	wIndex_t oldLineNum;
	wIndex_t pc;
	long oldCheckSum;
	long checkSum=0;
	BOOL_T checkSummed;
	long paramVersion = -1;
	char *oldLocale = NULL;

	if (dirName) {
		strcpy( paramFileName, dirName );
		strcat( paramFileName, FILE_SEP_CHAR );
		strcat( paramFileName, fileName );
	} else {
		strcpy( paramFileName, fileName );
	}
	paramLineNum = 0;
	curBarScale = -1;
	curContents = strdup( fileName );
	curSubContents = curContents;

LOG1( log_paramFile, ("ReadParam( %s )\n", fileName ) )
	
	oldLocale = setlocale(LC_ALL, "C");
	
	paramFile = fopen( paramFileName, "r" );
	if (paramFile == NULL) {
		NoticeMessage( MSG_OPEN_FAIL, "Continue", NULL, "Parameter", paramFileName, strerror(errno) );
		return FALSE;
	}
	paramCheckSum = key;
	paramLineNum = 0;
	checkSummed = FALSE;
	while ( paramFile && ( fgets(paramLine, 256, paramFile) ) != NULL ) {
		paramLineNum++;
		Stripcr( paramLine );
		if (strncmp( paramLine, "CHECKSUM ", 9 ) == 0) {
			checkSum = atol( paramLine+9 );
			checkSummed = TRUE;
			goto nextLine;
		}
		ParamCheckSumLine( paramLine );
		if (paramLine[0] == '#') {
			/* comment */
		} else if (paramLine[0] == 0) {
			/* empty paramLine */
		} else if (strncmp( paramLine, "INCLUDE ", 8 ) == 0) {
			cp = &paramLine[8];
			while (*cp && isspace(*cp)) cp++;
			if (!*cp) {
				InputError( "INCLUDE - no file name", TRUE );
				if (paramFile)fclose( paramFile );
				return FALSE;
			}
			oldFile = paramFile;
			oldLineNum = paramLineNum;
			oldCheckSum = paramCheckSum;
			ReadParams( key, dirName, cp );
			paramFile = oldFile;
			paramLineNum = oldLineNum;
			paramCheckSum = oldCheckSum;
			if (dirName) {
				strcpy( paramFileName, dirName );
				strcat( paramFileName, FILE_SEP_CHAR );
				strcat( paramFileName, fileName );
			} else {
				strcpy( paramFileName, fileName );
			}
		} else if (strncmp( paramLine, "CONTENTS ", 9) == 0 ) {
			curContents = MyStrdup( paramLine+9 );
			curSubContents = curContents;
		} else if (strncmp( paramLine, "SUBCONTENTS ", 12) == 0 ) {
			curSubContents = MyStrdup( paramLine+12 );
		} else if (strncmp( paramLine, "PARAM ", 6) == 0 ) {
			paramVersion = atol( paramLine+6 );
		} else {
			for (pc = 0; pc < paramProc_da.cnt; pc++ ) {
				if (strncmp( paramLine, paramProc(pc).name,
							 strlen(paramProc(pc).name)) == 0 ) {
					paramProc(pc).proc( paramLine );
					goto nextLine;
				}
			}
			InputError( "Unknown param line", TRUE );
		}
 nextLine:;
	}
	if ( key ) {
		if ( !checkSummed || checkSum != paramCheckSum ) {
			NoticeMessage( MSG_PROG_CORRUPTED, "Ok", NULL, paramFileName );
			return FALSE;
		}
	}
	if (paramFile)fclose( paramFile );
	
	if( oldLocale ) setlocale( LC_ALL, oldLocale );
	
	return TRUE;
}


static void ReadCustom( void )
{
	FILE * f;
	customPath =
		(char*)MyMalloc( strlen(workingDir) + 1 + strlen(sCustomF) + 1 );
	sprintf( customPath, "%s%s%s", workingDir, FILE_SEP_CHAR, sCustomF );
	customPathBak = MyStrdup( customPath );
	customPathBak[ strlen(customPathBak)-1 ] = '1';
	f = fopen( customPath, "r" );
	if ( f != NULL ) {
		fclose( f );
		curParamFileIndex = PARAM_CUSTOM;
		ReadParams( 0, workingDir, sCustomF );
	}
}


EXPORT FILE * OpenCustom( char * mode )
{
	FILE * ret = NULL;
	if (inPlayback)
		return NULL;
	if ( *mode == 'w' )
		rename( customPath, customPathBak );
	if (customPath) {
		ret = fopen( customPath, mode );
		if (ret == NULL) {
			NoticeMessage( MSG_OPEN_FAIL, "Continue", NULL, "Custom", customPath, strerror(errno) );
		}
	}
	return ret;
}


EXPORT char * PutTitle( char * cp )
{
	static char title[STR_SIZE];
	char * tp = title;
	while (*cp && (tp-title)<=(sizeof title)-3) {
		if (*cp == '\"') {
			*tp++ = '\"';
			*tp++ = '\"';
		} else {
			*tp++ = *cp;
		}
		cp++;
	}
	if ( *cp )
		NoticeMessage( "putTitle: title too long: %s", "Ok", NULL, title );
	*tp = '\0';
	return title;
}


void SetWindowTitle( void )
{
	if ( changed > 2 || inPlayback )
		return;
	sprintf( message, "%s%s - %s(%s)",
		(curFileName==NULL||curFileName[0]=='\0')?"Unnamed Trackplan":curFileName,
		changed>0?"*":"", sProdName, sVersion );
	wWinSetTitle( mainW, message );
}

/*****************************************************************************
 *
 * LOAD / SAVE TRACKS
 *
 */

static struct wFilSel_t * loadFile_fs;
static struct wFilSel_t * saveFile_fs;

static wWin_p checkPointingW;
static paramData_t checkPointingPLs[] = {
   {    PD_MESSAGE, "Check Pointing" } };
static paramGroup_t checkPointingPG = { "checkpoint", 0, checkPointingPLs, sizeof checkPointingPLs/sizeof checkPointingPLs[0] };

static char * checkPtFileName1;
static char * checkPtFileName2;


static BOOL_T ReadTrackFile(
		const char * pathName,
		const char * fileName,
		BOOL_T full,
		BOOL_T noSetCurDir,
		BOOL_T complain )
{
	int count;
	coOrd roomSize;
	long scale;
	char * cp;
	char *oldLocale = NULL;
	
	oldLocale = setlocale( LC_ALL, "C" );

	paramFile = fopen( pathName, "r" );
	if (paramFile == NULL) {
		if ( complain )
			NoticeMessage( MSG_OPEN_FAIL, "Continue", NULL, "XTrkCad", pathName, strerror(errno) );
		return FALSE;
	}
	if (!noSetCurDir)
		SetCurDir( pathName, fileName );
	if (full) {
		strcpy( curPathName, pathName );
		curFileName = &curPathName[fileName-pathName];
		SetWindowTitle();
	}
	paramLineNum = 0;
	strcpy( paramFileName, fileName );

	InfoMessage("0");
	count = 0;
	while ( paramFile && ( fgets(paramLine, sizeof paramLine, paramFile) ) != NULL ) {
		count++;
		if (count%10 == 0) {
			InfoMessage( "%d", count );
			wFlush();
		}
		paramLineNum++;
		if (strlen(paramLine) == (sizeof paramLine) -1 &&
			paramLine[(sizeof paramLine)-1] != '\n') {
			InputError( "Line too long", TRUE );
		}
		Stripcr( paramLine );
		if (paramLine[0] == '#' ||
			paramLine[0] == '\n' ||
			paramLine[0] == '\0' ) {
			/* comment */
			continue;
		}

		if (ReadTrack( paramLine )) {

		} else if (strncmp( paramLine, "END", 3 ) == 0) {
			break;
		} else if (strncmp( paramLine, "VERSION ", 8 ) == 0) {
			paramVersion = strtol( paramLine+8, &cp, 10 );
			if (cp)
				while (*cp && isspace(*cp)) cp++;
			if ( paramVersion > iParamVersion ) {
				if (cp && *cp) {
					NoticeMessage( MSG_UPGRADE_VERSION1, "Ok", NULL, paramVersion, iParamVersion, sProdName, cp );
				} else {
					NoticeMessage( MSG_UPGRADE_VERSION2, "Ok", NULL, paramVersion, iParamVersion, sProdName );
				}
				break;
			}
			if ( paramVersion < iMinParamVersion ) {
				NoticeMessage( MSG_BAD_FILE_VERSION, "Ok", NULL, paramVersion, iMinParamVersion, sProdName );
				break;
			}
		} else if (!full) {
			InputError( "unknown command", TRUE );
		} else if (strncmp( paramLine, "TITLE1 ", 7 ) == 0) {
			strcpy( Title1, &paramLine[7] );
			/*wStringSetValue( title1PD.control, Title1 );*/
		} else if (strncmp( paramLine, "TITLE2 ", 7 ) == 0) {
			strcpy( Title2, &paramLine[7] );
			/*wStringSetValue( title2PD.control, Title2 );*/
		} else if (strncmp( paramLine, "ROOMSIZE", 8 ) == 0) {
			if ( ParseRoomSize( paramLine+8, &roomSize ) ) {
				SetRoomSize( roomSize );
				/*wFloatSetValue( roomSizeXPD.control, PutDim(roomSize.x) );*/
				/*wFloatSetValue( roomSizeYPD.control, PutDim(roomSize.y) );*/
			} else {
				InputError( "ROOMSIZE: bad value", TRUE );
			}
		} else if (strncmp( paramLine, "SCALE ", 6 ) == 0) {
			if ( !DoSetScale( paramLine+5 ) ) {
				InputError( "SCALE: bad value", TRUE );
			}
		} else if (strncmp( paramLine, "MAPSCALE ", 9 ) == 0) {
			scale = atol( paramLine+9 );
			if (scale > 1) {
				mapD.scale = mapScale = scale;
			}
		} else if (strncmp( paramLine, "LAYERS ", 7 ) == 0) {
			ReadLayers( paramLine+7 );
		} else {
			InputError( "unknown command", TRUE );
		}
	}
	if (paramFile)fclose(paramFile);

	if( oldLocale ) setlocale( LC_ALL, oldLocale );
	
	paramFile = NULL;
	InfoMessage( "%d", count );
	return TRUE;
}


EXPORT int LoadTracks(
		const char * pathName,
		const char * fileName,
		void * data)
{
#ifdef TIME_READTRACKFILE
	long time0, time1;
#endif
	if (pathName == NULL)
		return TRUE;
	paramVersion = -1;
	wSetCursor( wCursorWait );
	Reset();
	ClearTracks();
	ResetLayers();
	checkPtMark = changed = 0;
	UndoSuspend();
	useCurrentLayer = FALSE;
#ifdef TIME_READTRACKFILE
	time0 = wGetTimer();
#endif
	if (ReadTrackFile( pathName, fileName, TRUE, FALSE, TRUE )) {
		wMenuListAdd( fileList_ml, 0, fileName, MyStrdup(pathName) );
		ResolveIndex();
#ifdef TIME_READTRACKFILE
		time1 = wGetTimer();
		printf( "time= %ld ms \n", time1-time0 );
#endif
		RecomputeElevations();
		AttachTrains();
		DoChangeNotification( CHANGE_ALL );
		DoUpdateTitles();
	}
	UndoResume();
	/*DoRedraw();*/
	Reset();
	wSetCursor( wCursorNormal );
	return TRUE;
}


EXPORT void DoFileList(
		int index,
		char * label,
		void * data )
{
	char * fileName, * pathName = (char*)data;
	fileName = strrchr( pathName, FILE_SEP_CHAR[0] );
	if (fileName == NULL)
		fileName = pathName;
	else
		fileName++;
	LoadTracks( pathName, fileName, NULL );
}


static BOOL_T DoSaveTracks(
		const char * fileName )
{
	FILE * f;
	time_t clock;
	BOOL_T rc = TRUE;
	char *oldLocale = NULL;
	
	oldLocale = setlocale( LC_ALL, "C" );
	
	f = fopen( fileName, "w" );
	if (f==NULL) {
		NoticeMessage( MSG_OPEN_FAIL, "Continue", NULL, "Track", fileName, strerror(errno) );
		
		if( oldLocale ) setlocale( LC_ALL, oldLocale );
		
		return FALSE;
	}
	wSetCursor( wCursorWait );
	time(&clock);
	rc &= fprintf(f,"#%s Version: %s, Date: %s\n", sProdName, sVersion, ctime(&clock) )>0;
	rc &= fprintf(f, "VERSION %d %s\n", iParamVersion, PARAMVERSIONVERSION )>0;
	Stripcr( Title1 );
	Stripcr( Title2 );
	rc &= fprintf(f, "TITLE1 %s\n", Title1 )>0;
	rc &= fprintf(f, "TITLE2 %s\n", Title2 )>0;
	rc &= fprintf(f, "MAPSCALE %ld\n", (long)mapD.scale )>0;
	rc &= fprintf(f, "ROOMSIZE %0.1f x %0.1f\n", mapD.size.x, mapD.size.y )>0;
	rc &= fprintf(f, "SCALE %s\n", curScaleName )>0;
	rc &= WriteLayers( f );
	rc &= WriteMainNote( f );
	rc &= WriteTracks( f );
	rc &= fprintf(f, "END\n")>0;
	if ( !rc )
		NoticeMessage( MSG_WRITE_FAILURE, "Ok", NULL, strerror(errno), fileName );
	fclose(f);
	if( oldLocale ) setlocale( LC_ALL, oldLocale );

	checkPtMark = changed;
	wSetCursor( wCursorNormal );
	return rc;
}


static doSaveCallBack_p doAfterSave;

static int SaveTracks(
		const char * pathName,
		const char * fileName,
		void * data )
{
	if (pathName == NULL)
		return TRUE;
	SetCurDir( pathName, fileName );
	DoSaveTracks( pathName );
	wMenuListAdd( fileList_ml, 0, fileName, MyStrdup(pathName) );
	checkPtMark = changed = 0;
	strcpy( curPathName, pathName );
	curFileName = &curPathName[fileName-pathName];
	if (doAfterSave)
		doAfterSave();
	doAfterSave = NULL;
	return TRUE;
}


EXPORT void DoSave( doSaveCallBack_p after )
{
	doAfterSave = after;
	if (curPathName[0] == 0) {
		if (saveFile_fs == NULL)
			saveFile_fs = wFilSelCreate( mainW, FS_SAVE, 0, "Save Tracks",
				sSourceFilePattern, SaveTracks, NULL );
		wFilSelect( saveFile_fs, curDirName );
	} else {
		SaveTracks( curPathName, curFileName, NULL );
	}
	SetWindowTitle();
}

EXPORT void DoSaveAs( doSaveCallBack_p after )
{
	doAfterSave = after;
	if (saveFile_fs == NULL)
		saveFile_fs = wFilSelCreate( mainW, FS_SAVE, 0, "Save Tracks",
			sSourceFilePattern, SaveTracks, NULL );
	wFilSelect( saveFile_fs, curDirName );
	SetWindowTitle();
}

EXPORT void DoLoad( void )
{
	loadFile_fs = wFilSelCreate( mainW, FS_LOAD, 0, "Open Tracks",
		sSourceFilePattern, LoadTracks, NULL );
	wFilSelect( loadFile_fs, curDirName );
}


EXPORT void DoCheckPoint( void )
{
	int rc; 
	
	if (checkPointingW == NULL) {
		ParamRegister( &checkPointingPG );
		checkPointingW = ParamCreateDialog( &checkPointingPG, MakeWindowTitle("Check Pointing"), NULL, NULL, NULL, FALSE, NULL, F_TOP|F_CENTER, NULL );
	}
	rename( checkPtFileName1, checkPtFileName2 );
	wShow( checkPointingW );
	rc = DoSaveTracks( checkPtFileName1 );
	
	/* could the check point file be written ok? */
	if( rc ) {
		/* yes, delete the backup copy of the checkpoint file */
		remove( checkPtFileName2 );
	} else {
		/* no, rename the backup copy back to the checkpoint file name */
		rename( checkPtFileName2, checkPtFileName1 );		
	}		
	wHide( checkPointingW );
}

/* \brief Remove all temporary files before exiting
 *
 * \param none
 * \return none
 *
 * When the program terminates normally through the exit choice, files 
 * that are created temporarily are removed: xtrkcad.ckp
 * 
 */
 
EXPORT void CleanupFiles( void )
{
	if( checkPtFileName1 )
		remove( checkPtFileName1 );
}	

/* \brief Check for existance of checkpoint file 
 *
 * \param none
 * \return TRUE if exists, FALSE otherwise
 *
 * Existance of a checkpoint file means that XTrkCAD was not properly 
 * terminated.
 * 
 */

EXPORT int ExistsCheckpoint( void )
{
	int len;
	char *pattern = sCheckPointF;
	char *search;
	
	struct stat fileStat;

	len = strlen( workingDir ) + 1 + strlen( sCheckPointF ) + 1;
	checkPtFileName1 = (char*)MyMalloc(len);
	sprintf( checkPtFileName1, "%s%s%s", workingDir, FILE_SEP_CHAR, sCheckPointF );
	checkPtFileName2 = (char*)MyMalloc(len);
	sprintf( checkPtFileName2, "%s%s%s", workingDir, FILE_SEP_CHAR, sCheckPoint1F );	

	len = strlen( workingDir ) + 1 + strlen( pattern ) + 1;
	search = (char*)MyMalloc(len);
	sprintf( search, "%s%s%s", workingDir, FILE_SEP_CHAR, pattern );

	if( !stat( search, &fileStat ) ) {
		MyFree( search );
		return TRUE;
	} else {
		MyFree( search );
		return FALSE;
	}	


#ifdef LATER
	DIR *dir;

	dir = opendir( search );
	MyFree( search );
	
	if( dir )	{
		closedir( dir );
		return TRUE;
	} else {
		return FALSE;
	}	
#endif	

}

/* \brief Load checkpoint file 
 *
 * \param none
 * \return TRUE if exists, FALSE otherwise
 *
 */

EXPORT int LoadCheckpoint( void )
{
	int len;
	char *search;
	
	paramVersion = -1;
	wSetCursor( wCursorWait );

	len = strlen( workingDir ) + 1 + strlen( sCheckPointF ) + 1;
	search = (char*)MyMalloc(len);
	sprintf( search, "%s%s%s", workingDir, FILE_SEP_CHAR, sCheckPointF );

	if (ReadTrackFile( search, search + strlen(search) - strlen( sCheckPointF ), TRUE, TRUE, TRUE )) {
		ResolveIndex();

		RecomputeElevations();
		AttachTrains();
		DoChangeNotification( CHANGE_ALL );
		DoUpdateTitles();
	}

	Reset();
	wSetCursor( wCursorNormal );

	strcpy( curPathName, "" );
	curFileName = curPathName;
	SetWindowTitle();
	changed = TRUE;
	MyFree( search );
	return TRUE;
}

/*****************************************************************************
 *
 * IMPORT / EXPORT
 *
 */

static struct wFilSel_t * exportFile_fs;
static struct wFilSel_t * importFile_fs;
static struct wFilSel_t * exportDXFFile_fs;


static int ImportTracks(
		const char * pathName,
		const char * fileName,
		void * data )
{
	long paramVersionOld = paramVersion;

	if (pathName == NULL)
		return TRUE;
	paramVersion = -1;
	wSetCursor( wCursorWait );
	Reset();
	SetAllTrackSelect( FALSE );
	ImportStart();
	UndoStart( "Import Tracks", "importTracks" );
	useCurrentLayer = TRUE;
	ReadTrackFile( pathName, fileName, FALSE, FALSE, TRUE );
	ImportEnd();
	/*DoRedraw();*/
	EnableCommands();
	wSetCursor( wCursorNormal );
	paramVersion = paramVersionOld;
	importMove = TRUE;
	DoCommandB( (void*)selectCmdInx );
	SelectRecount();
	return TRUE;
}


EXPORT void DoImport( void )
{
	if (importFile_fs == NULL)
		importFile_fs = wFilSelCreate( mainW, FS_LOAD, 0, "Import Tracks",
			sImportFilePattern, ImportTracks, NULL );

	wFilSelect( importFile_fs, curDirName );
}

static int DoExportTracks(
		const char * pathName,
		const char * fileName,
		void * data )
{
	FILE * f;
	time_t clock;
	if (pathName == NULL)
		return TRUE;
	SetCurDir( pathName, fileName );
	f = fopen( pathName, "w" );
	if (f==NULL) {
		NoticeMessage( MSG_OPEN_FAIL, "Continue", NULL, "Export", fileName, strerror(errno) );
		return FALSE;
	}
	wSetCursor( wCursorWait );
	time(&clock);
	fprintf(f,"#%s Version: %s, Date: %s\n", sProdName, sVersion, ctime(&clock) );
	fprintf(f, "VERSION %d %s\n", iParamVersion, PARAMVERSIONVERSION );
	ExportTracks( f );
	fprintf(f, "END\n");
	fclose(f);
	Reset();
	wSetCursor( wCursorNormal );
	UpdateAllElevations();
	return TRUE;
}


EXPORT void DoExport( void )
{
	if (selectedTrackCount <= 0) {
		ErrorMessage( MSG_NO_SELECTED_TRK );
		return;
	}
	if (exportFile_fs == NULL)
		exportFile_fs = wFilSelCreate( mainW, FS_SAVE, 0, "Export Tracks",
				sImportFilePattern, DoExportTracks, NULL );

	wFilSelect( exportFile_fs, curDirName );
}


static FILE * dxfF;
static void DxfLine(
		drawCmd_p d,
		coOrd p0,
		coOrd p1,
		wDrawWidth width,
		wDrawColor color )
{
	fprintf(dxfF, "  0\nLINE\n" );
	fprintf(dxfF, "  8\n%s%d\n", sProdNameUpper, curTrackLayer+1 );
	fprintf(dxfF, "  10\n%0.6f\n  20\n%0.6f\n  11\n%0.6f\n  21\n%0.6f\n",
		p0.x, p0.y, p1.x, p1.y );
	fprintf(dxfF, "  6\n%s\n", (d->options&DC_DASH)?"DASHED":"CONTINUOUS" );
}

static void DxfArc(
		drawCmd_p d,
		coOrd p,
		DIST_T r,
		ANGLE_T angle0,
		ANGLE_T angle1,
		BOOL_T drawCenter,
		wDrawWidth width,
		wDrawColor color )
{
	angle0 = NormalizeAngle(90.0-(angle0+angle1));
	if (angle1 >= 360.0) {
		fprintf(dxfF, "  0\nCIRCLE\n" );
		fprintf(dxfF, "  10\n%0.6f\n  20\n%0.6f\n  40\n%0.6f\n",
				p.x, p.y, r );
	} else {
		fprintf(dxfF, "  0\nARC\n" );
		fprintf(dxfF, "  10\n%0.6f\n  20\n%0.6f\n  40\n%0.6f\n  50\n%0.6f\n  51\n%0.6f\n",
				p.x, p.y, r, angle0, angle0+angle1 );
	}
	fprintf(dxfF, "  8\n%s%d\n", sProdNameUpper, curTrackLayer+1 );
	fprintf(dxfF, "  6\n%s\n", (d->options&DC_DASH)?"DASHED":"CONTINUOUS" );
}

static void DxfString(
		drawCmd_p d,
		coOrd p,
		ANGLE_T a,
		char * s,
		wFont_p fp,
		FONTSIZE_T fontSize,
		wDrawColor color )
{
	fprintf(dxfF, "  0\nTEXT\n" );
	fprintf(dxfF, "  1\n%s\n", s );
	fprintf(dxfF, "  8\n%s%d\n", sProdNameUpper, curTrackLayer+1 );
	fprintf(dxfF, "  10\n%0.6f\n  20\n%0.6f\n", p.x, p.y );
	fprintf(dxfF, "  40\n%0.6f\n", fontSize/72.0 );
}

static void DxfBitMap(
		drawCmd_p d,
		coOrd p,
		wDrawBitMap_p bm,
		wDrawColor color )
{
}

static void DxfFillPoly(
		drawCmd_p d,
		int cnt,
		coOrd * pts,
		wDrawColor color )
{
	int inx;
	for (inx=1; inx<cnt; inx++) {
		DxfLine( d, pts[inx-1], pts[inx], 0, color );
	}
	DxfLine( d, pts[cnt-1], pts[0], 0, color );
}

static void DxfFillCircle( drawCmd_p d, coOrd center, DIST_T radius, wDrawColor color )
{
	DxfArc( d, center, radius, 0.0, 360, FALSE, 0, color );
}


static drawFuncs_t dxfDrawFuncs = {
		0,
		DxfLine,
		DxfArc,
		DxfString,
		DxfBitMap,
		DxfFillPoly,
		DxfFillCircle };

static drawCmd_t dxfD = {
		NULL, &dxfDrawFuncs, 0, 1.0, 0.0, {0.0,0.0}, {0.0,0.0}, Pix2CoOrd, CoOrd2Pix, 100.0 };

static int DoExportDXFTracks(
		const char * pathName,
		const char * fileName,
		void * data )
{
	time_t clock;
	if (pathName == NULL)
		return TRUE;
	SetCurDir( pathName, fileName );
	dxfF = fopen( pathName, "w" );
	if (dxfF==NULL) {
		NoticeMessage( MSG_OPEN_FAIL, "Continue", NULL, "DXF", fileName, strerror(errno) );
		return FALSE;
	}
	wSetCursor( wCursorWait );
	time(&clock);
	fprintf(dxfF,"\
  0\nSECTION\n\
  2\nHEADER\n\
  9\n$ACADVER\n  1\nAC1009\n\
  9\n$EXTMIN\n  10\n%0.6f\n  20\n%0.6f\n\
  9\n$EXTMAX\n  10\n%0.6f\n  20\n%0.6f\n\
  9\n$TEXTSTYLE\n  7\nSTANDARD\n\
  0\nENDSEC\n\
  0\nSECTION\n\
  2\nTABLES\n\
  0\nTABLE\n\
  2\nLTYPE\n\
  0\nLTYPE\n  2\nCONTINUOUS\n  70\n0\n\
  3\nSolid line\n\
  72\n65\n  73\n0\n  40\n0\n\
  0\nLTYPE\n  2\nDASHED\n  70\n0\n\
  3\n__ __ __ __ __ __ __ __ __ __ __ __ __ __ __\n\
  72\n65\n  73\n2\n  40\n0.15\n  49\n0.1\n  49\n-0.05\n\
  0\nLTYPE\n  2\nDOT\n  70\n0\n\
  3\n...............................................\n\
  72\n65\n  73\n2\n  40\n0.1\n  49\n0\n  49\n-0.05\n\
  0\nENDTAB\n\
  0\nTABLE\n\
  2\nLAYER\n\
  70\n0\n\
  0\nLAYER\n  2\n%s1\n  6\nCONTINUOUS\n  62\n7\n  70\n0\n\
  0\nLAYER\n  2\n%s2\n  6\nCONTINUOUS\n  62\n7\n  70\n0\n\
  0\nLAYER\n  2\n%s3\n  6\nCONTINUOUS\n  62\n7\n  70\n0\n\
  0\nLAYER\n  2\n%s4\n  6\nCONTINUOUS\n  62\n7\n  70\n0\n\
  0\nLAYER\n  2\n%s5\n  6\nCONTINUOUS\n  62\n7\n  70\n0\n\
  0\nLAYER\n  2\n%s6\n  6\nCONTINUOUS\n  62\n7\n  70\n0\n\
  0\nLAYER\n  2\n%s7\n  6\nCONTINUOUS\n  62\n7\n  70\n0\n\
  0\nLAYER\n  2\n%s8\n  6\nCONTINUOUS\n  62\n7\n  70\n0\n\
  0\nLAYER\n  2\n%s9\n  6\nCONTINUOUS\n  62\n7\n  70\n0\n\
  0\nLAYER\n  2\n%s10\n  6\nCONTINUOUS\n  62\n7\n  70\n0\n\
  0\nENDTAB\n\
  0\nENDSEC\n\
  0\nSECTION\n\
  2\nENTITIES\n\
",
		0.0, 0.0, mapD.size.x, mapD.size.y,
		sProdNameUpper, sProdNameUpper, sProdNameUpper, sProdNameUpper, sProdNameUpper,
		sProdNameUpper, sProdNameUpper, sProdNameUpper, sProdNameUpper, sProdNameUpper );
	DrawSelectedTracks( &dxfD );
	fprintf(dxfF,"  0\nENDSEC\n");
	fprintf(dxfF,"  0\nEOF\n");
	fclose(dxfF);
	Reset();
	wSetCursor( wCursorNormal );
	return TRUE;
}


void DoExportDXF( void )
{
	if (selectedTrackCount <= 0) {
		ErrorMessage( MSG_NO_SELECTED_TRK );
		return;
	}
	if (exportDXFFile_fs == NULL)
		exportDXFFile_fs = wFilSelCreate( mainW, FS_SAVE, 0, "Export to DXF",
				sDXFFilePattern, DoExportDXFTracks, NULL );

	wFilSelect( exportDXFFile_fs, curDirName );
}

EXPORT BOOL_T EditCopy( void )
{
	FILE * f;
	time_t clock;
	if (selectedTrackCount <= 0) {
		ErrorMessage( MSG_NO_SELECTED_TRK );
		return FALSE;
	}
	f = fopen( clipBoardN, "w" );
	if (f == NULL) {
		NoticeMessage( MSG_OPEN_FAIL, "Continue", NULL, "Clipboard", clipBoardN, strerror(errno) );
		return FALSE;
	}
	time(&clock);
	fprintf(f,"#%s Version: %s, Date: %s\n", sProdName, sVersion, ctime(&clock) );
	fprintf(f, "VERSION %d %s\n", iParamVersion, PARAMVERSIONVERSION );
	ExportTracks(f);
	fprintf(f, "END\n");
	fclose(f);
	return TRUE;
}


EXPORT BOOL_T EditCut( void )
{
	if (!EditCopy())
		return FALSE;
	SelectDelete();
	return TRUE;
}


EXPORT BOOL_T EditPaste( void )
{
	BOOL_T rc = TRUE;
	wSetCursor( wCursorWait );
	Reset();
	SetAllTrackSelect( FALSE );
	ImportStart();
	UndoStart( "Paste", "paste" );
	useCurrentLayer = TRUE;
	if ( !ReadTrackFile( clipBoardN, sClipboardF, FALSE, TRUE, FALSE ) ) {
		NoticeMessage( MSG_CANT_PASTE, "Continue", NULL );
		rc = FALSE;
	}
	ImportEnd();
	/*DoRedraw();*/
	EnableCommands();
	wSetCursor( wCursorNormal );
	importMove = TRUE;
	DoCommandB( (void*)selectCmdInx );
	SelectRecount();
	UpdateAllElevations();
	return rc;
}

/*****************************************************************************
 *
 * INITIALIZATION
 *
 */

EXPORT void FileInit( void )
{
	const char * pref;

	if ( (libDir = wGetAppLibDir()) == NULL ) {
		abort();
	}
	if ( (workingDir = wGetAppWorkDir()) == NULL )
		AbortProg( "wGetAppWorkDir()" );

	pref = wPrefGetString( "file", "directory" );
	if (pref != NULL) {
		strcpy( curDirName, pref );
	} else {
		sprintf( curDirName, "%s%sexamples", libDir, FILE_SEP_CHAR );
	}
}

EXPORT BOOL_T ParamFileInit( void )
{
	curParamFileIndex = PARAM_DEMO;
	log_paramFile = LogFindIndex( "paramFile" );
	if ( ReadParams( lParamKey, libDir, sParamQF ) == FALSE )
		return FALSE;

	curParamFileIndex = PARAM_CUSTOM;
	if (lParamKey == 0) {
		ReadParamFiles();
		ReadCustom();
	}

	curPathName[0] = '\0';

	clipBoardN = (char*)MyMalloc( strlen(workingDir) + 1 + strlen(sClipboardF) + 1 );
	sprintf( clipBoardN, "%s%s%s", workingDir, FILE_SEP_CHAR, sClipboardF );
	return TRUE;

}
