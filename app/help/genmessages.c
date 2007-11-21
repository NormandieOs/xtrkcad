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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef WINDOWS
	#if _MSC_VER >=1400
		#define strdup _strdup
	#endif
#endif


typedef struct helpMsg_t * helpMsg_p;
typedef struct helpMsg_t {
	char * key;
	char * title;
	char * help;
	} helpMsg_t;

helpMsg_t helpMsgs[200];
int helpMsgCnt = 0;

struct transTbl {
      char *inChar;
      char *outChar[];
};

/* ATTENTION: make sure that the characters are in the same order as the equivalent escape sequences below */

/* translation table for unicode sequences understood by Halibut */
struct transTbl toUnicode = {
       "�\0",
      { "\\u00B0",
		  "\\0"  }
};

/* translation table for escape sequences understood by C compiler */

struct transTbl toC = {
      "\\\"\0",
      { "\\\\",
      "\\\"",
		"\\0" }
};


char *
translateString( char *srcString, struct transTbl *trTbl )
{
	char *destString;
   char *cp;
   char *cp2;
   int bufLen = strlen( srcString ) + 1;
   char *idx;

   /* calculate the expected result length */
   for( cp = srcString; *cp; cp++ )
   	if( idx = strchr( trTbl->inChar, *cp ))         /* does character need translation ? */
      	bufLen += strlen( (trTbl->outChar)[idx - trTbl->inChar ] ) - 1; /* yes, extend buffer accordingly */

   /* allocate memory for result */
   destString = malloc( bufLen );

   if( destString ) {
		/* copy and translate characters as needed */
      cp2 = destString;
      for( cp = srcString; *cp; cp++ ) {
      	if( idx = strchr( trTbl->inChar, *cp )) {       /* does character need translation ? */
         	strcpy( cp2, (trTbl->outChar)[idx - trTbl->inChar ] );   /* yes, copy the escaped character sequence */
            cp2 += strlen((trTbl->outChar)[idx - trTbl->inChar ] );
         } else {
            *cp2++ = *cp;                       /* no, just copy the character */
         }
		}
   	/* terminate string */
   	*cp2 = '\0';
   } else {
   	/* memory allocation failed */
      exit(1);
   }

   return( destString );
}


int cmpHelpMsg( const void * a, const void * b )
{
	helpMsg_p aa = (helpMsg_p)a;
	helpMsg_p bb = (helpMsg_p)b;
	return strcmp( aa->title, bb->title );
}

void unescapeString( FILE * f, char * str ) 
{
	while (*str) {
		if (*str != '\\')
			fputc( *str, f );
		str++;
	}
}

/**
 * Generate the file in help source format ( ie. the BUT file )
 */

void dumpHelp( FILE *hlpsrcF )
{
	int inx;
	char *transStr;

	/* sort in alphabetical order */
	qsort( helpMsgs, helpMsgCnt, sizeof helpMsgs[0], cmpHelpMsg );

	/* write the start of the help source */
	fprintf( hlpsrcF, "\\A{messages} Error Messages\n\n" );

	/* now save all the help messages */
	for ( inx=0; inx<helpMsgCnt; inx++ ) {
	
		transStr = translateString( helpMsgs[inx].title, &toUnicode );
		fprintf( hlpsrcF, "\\H{%s} %s\n\n", helpMsgs[inx].key, transStr );
		free( transStr );
		
		transStr = translateString( helpMsgs[inx].help, &toUnicode );
		fprintf( hlpsrcF, "%s\n\n", transStr );
		free( transStr );		
#ifdef LATER
		fprintf( hlpsrcF, "\\H{" );
		unescapeString ( hlpsrcF, helpMsgs[inx].key );
		fprintf( hlpsrcF, "} " );

		unescapeString( hlpsrcF, helpMsgs[inx].title );
		fprintf( hlpsrcF, "\n\n" );

		unescapeString ( hlpsrcF, helpMsgs[inx].help );

		fprintf( hlpsrcF, "\n\n" );
#endif
	}
}


int main( int argc, char * argv[] )
{
	FILE * hdrF;
	FILE *inF;
	FILE *outF;
	
	char buff[256];
	char * cp;
	int inFileIdx;
	enum {m_init, m_title, m_alt, m_help } mode = m_init;
	char msgName[256];
	char msgAlt[256];
	char msgTitle[1024];
	char msgTitle1[1024];
	char msgHelp[4096];
#ifndef FLAGS	
	int flags; 
#endif
	/* check argument count */
	if ( argc < 3 || argc > 4 ) {
		fprintf( stderr, "Usage: %s [-i18n] INFILE OUTFILE\n\n", argv[0] );
		fprintf( stderr, "       -i18n is used to generate a include file with gettext support.\n\n" );
		exit(1);
	}
	
	/* check options */
	if( argc == 4 ) {
		if( !strcmp(argv[ 1 ], "-i18n")){
			fprintf( stderr, "Option is not supported yet!\n" );
			exit( 1 );
		}	
		/* inFileIdx = 2;  skip over option argument */
	} else {
		inFileIdx = 1;	/* first argument is input file */
	}	
	
	/* open the file for reading */
	inF = fopen( argv[ inFileIdx ], "r" );
	if( !inF ) {
			fprintf( stderr, "Could not open %s for reading!\n", argv[ inFileIdx ] );
			exit( 1 );
	}		
			
	/* open the include file to generate */		
	hdrF = fopen( "messages.h", "w" );
	if( !hdrF ) {
			fprintf( stderr, "Could not open messages.h for writing!\n" );
			exit( 1 );
	}		
	
	/* open the help file to generate */
	outF = fopen( argv[ inFileIdx + 1 ], "w" );
	if( !inF ) {
			fprintf( stderr, "Could not open %s for writing!\n", argv[ inFileIdx ] );
			exit( 1 );
	}		
							
	while ( fgets( buff, sizeof buff, inF ) ) {
	
		/* skip comment lines */
		if ( buff[0] == '#' )
			continue;
			
		/* remove trailing newline */	
		cp = buff+strlen(buff)-1;
		if (*cp=='\n') *cp = 0;
		
		
		if ( strncmp( buff, "MESSAGE ", 8 ) == 0 ) {

			/* skip any spaces */
			cp = strchr( buff+8, ' ' );
			if (cp)
				while (*cp == ' ') *cp++ = 0;
#ifndef FLAGS		
			if ( cp && *cp ) {
				flags = atoi(cp);
			} 
#endif			
			/* save the name of the message */
			strcpy( msgName, buff + 8 );
			msgAlt[0] = 0;
			msgTitle[0] = 0;
			msgTitle1[0] = 0;
			msgHelp[0] = 0;
			mode = m_title;
		} else if ( strncmp( buff, "ALT", 3 ) == 0 ) {
			mode = m_alt;
		} else if ( strncmp( buff, "HELP", 4 ) == 0 ) {
			mode = m_help;
		} else if ( strncmp( buff, "END", 3 ) == 0 ) {
			/* the whole message has been read */
			if (msgHelp[0]==0) {
				/* no help text is included */
				fprintf( hdrF, "#define %s \"%s\"\n", msgName, msgTitle );
			} else if (msgAlt[0]) {
				/* a help text and an alternate description are included */
				fprintf( hdrF, "#define %s \"%s\\t%s\\t%s\"\n", msgName, msgName, msgAlt, msgTitle );
			} else {
				/* a help text but no alternate description are included */
				fprintf( hdrF, "#define %s \"%s\\t%s\"\n", msgName, msgName, msgTitle );
			}
			
			/* save the help text for later use */
			if (msgHelp[0]) {
				helpMsgs[helpMsgCnt].key = strdup(msgName);
				if ( msgAlt[0] )
				    helpMsgs[helpMsgCnt].title = strdup(msgAlt);
				else
				    helpMsgs[helpMsgCnt].title = strdup(msgTitle);
				helpMsgs[helpMsgCnt].help = strdup(msgHelp);
				helpMsgCnt++;
			}
			mode = 0;
		} else {
			/* are we currently reading the message text? */
			if (mode == m_title) {
				/* yes, is the message text split over two lines ? */
				if (msgTitle[0]) {
					/* if yes, keep the first part as the short text */			
					if (msgAlt[0] == 0) {
						strcpy( msgAlt, msgTitle );
						strcat( msgAlt, "..." );
					}
					/* add a newline to the first part */
					strcat( msgTitle, "\\n" );
				}
				/* now save the buffer into the message title */
				strcat( msgTitle, buff );
			} else if (mode == m_alt) {
				/* an alternate text was explicitly specified, save */
				strcpy( msgAlt, buff );
			} else if (mode == m_help) {
				/* we are reading the help text, save in buffer */
				strcat( msgHelp, buff );
				strcat( msgHelp, "\n" );
			}
		}
	}
	dumpHelp( outF );

	fclose( hdrF );
	fclose( inF );
	fclose( outF );
	
	printf( "%d messages\n", helpMsgCnt );
	return 0;
}
