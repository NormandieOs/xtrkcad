/** \file gtkdraw-cairo.c
 * Basic drawing functions
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

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif
#include <unistd.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "gtkint.h"
#include "gdk/gdkkeysyms.h"

#define gtkAddHelpString( a, b ) wlibAddHelpString( a, b )

#define CENTERMARK_LENGTH (6)

static long drawVerbose = 0;

struct wDrawBitMap_t {
		int w;
		int h;
		int x;
		int y;
		const char * bits;
		GdkPixbuf * pixbuf;
		};

//struct wDraw_t {
		//WOBJ_COMMON
		//void * context;
		//wDrawActionCallBack_p action;
		//wDrawRedrawCallBack_p redraw;

		//GdkPixmap * pixmap;
		//GdkPixmap * pixmapBackup;

		//double dpi;

		//GdkGC * gc;
		//wDrawWidth lineWidth;
		//wDrawOpts opts;
		//wPos_t maxW;
		//wPos_t maxH;
		//unsigned long lastColor;
		//wBool_t lastColorInverted;
		//const char * helpStr;

		//wPos_t lastX;
		//wPos_t lastY;

		//wBool_t delayUpdate;
		//};

struct wDraw_t psPrint_d;

/*****************************************************************************
 *
 * MACROS
 *
 */

#define LBORDER (22)
#define RBORDER (6)
#define TBORDER (6)
#define BBORDER (20)

#define INMAPX(D,X)	(X)
#define INMAPY(D,Y)	(((D)->h-1) - (Y))
#define OUTMAPX(D,X)	(X)
#define OUTMAPY(D,Y)	(((D)->h-1) - (Y))


/*******************************************************************************
 *
 * Basic Drawing Functions
 *
*******************************************************************************/


static cairo_t* gtkDrawCreateCairoContext(
		wDraw_p bd,
		GdkPixbuf * pixbuf,
		cairo_surface_t * surface,
		wDrawWidth width,
		wDrawLineType_e lineType,
		wDrawColor color,
		wDrawOpts opts )
{
	cairo_t * cairo;
	cairo_surface_t * cairo_surface;

	if (pixbuf) {
		GdkWindow * window = gtk_widget_get_window(GTK_WIDGET(gtkMainW->gtkwin));
		//gdk_window_create_similar_surface (GdkWindow *window,cairo_content_t content,int width,int height);
		cairo_surface = gdk_window_create_similar_surface(window, CAIRO_CONTENT_COLOR, gdk_window_get_width(window),gdk_window_get_height(window));
		if (bd->surface) bd->surface = cairo_surface;
		cairo = cairo_create(cairo_surface);
	} else
		cairo = cairo_create(bd->surface);

	width = width ? abs(width) : 1;
	cairo_set_line_width(cairo, width);

	cairo_set_line_cap(cairo, CAIRO_LINE_CAP_BUTT);
	cairo_set_line_join(cairo, CAIRO_LINE_JOIN_MITER);

	switch(lineType)
	{
		case wDrawLineSolid:
		{
			cairo_set_dash(cairo, 0, 0, 0);
			break;
		}
		case wDrawLineDash:
		{
			double dashes[] = { 5, 3 };
			static int len_dashes  = sizeof(dashes) / sizeof(dashes[0]);
			cairo_set_dash(cairo, dashes, len_dashes, 0);
			break;
		}
	}
	GdkRGBA gcolor;

	cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
	if(opts & wDrawOptTemp)
	{
		cairo_set_source_rgba(cairo, 0, 0, 0, 0);
		cairo_set_operator(cairo, CAIRO_OPERATOR_OVER);
	}
	else
	{
        long tcolor = wlibGetColor(color, TRUE);
        gcolor.red = (tcolor&0x00FF0000)>>16;
        gcolor.green = (tcolor&0x0000FF00)>>8;
        gcolor.blue = (tcolor&0x000000FF);

        bd->lastColor = tcolor;

        cairo_set_source_rgba(cairo, gcolor.red, gcolor.green, gcolor.blue, 1.0);
        cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    }

	if (bd->clip_set) {
		cairo_rectangle(cairo,bd->rect.x,bd->rect.y,bd->rect.width,bd->rect.height);
		cairo_clip(cairo);
	}

	return cairo;
}

static cairo_t* gtkDrawDestroyCairoContext(cairo_t *cairo, cairo_surface_t *surface) {
	if (surface)
		cairo_surface_destroy(surface);
	cairo_destroy(cairo);
	return NULL;
}

 void wDrawDelayUpdate(
		wDraw_p bd,
		wBool_t delay )
{
	cairo_rectangle_int_t update_rect;

	if ( (!delay) && bd->delayUpdate ) {
		update_rect.x = 0;
		update_rect.y = 0;
		update_rect.width = bd->w;
		update_rect.height = bd->h;
		cairo_region_t * cairo_region = cairo_region_create_rectangle(&update_rect);
		gtk_widget_queue_draw_region(bd->widget, cairo_region);
		//gtk_widget_draw( bd->widget, &update_rect );
		cairo_region_destroy(cairo_region);
	}
	bd->delayUpdate = delay;
}


 void wDrawLine(
		wDraw_p bd,
		wPos_t x0, wPos_t y0,
		wPos_t x1, wPos_t y1,
		wDrawWidth width,
		wDrawLineType_e lineType,
		wDrawColor color,
		wDrawOpts opts )
{

	if ( bd == &psPrint_d ) {
		psPrintLine( x0, y0, x1, y1, width, lineType, color, opts );
		return;
	}
	x0 = INMAPX(bd,x0);
	y0 = INMAPY(bd,y0);
	x1 = INMAPX(bd,x1);
	y1 = INMAPY(bd,y1);

	cairo_t* cairo = gtkDrawCreateCairoContext(bd, NULL, NULL, width, lineType, color, opts);
	cairo_move_to(cairo, x0 + 0.5, y0 + 0.5);
	cairo_line_to(cairo, x1 + 0.5, y1 + 0.5);
	cairo_stroke(cairo);

	gtkDrawDestroyCairoContext(cairo, NULL);

}

/**
 * Draw an arc around a specified center
 *
 * \param bd IN ?
 * \param x0, y0 IN  center of arc
 * \param r IN radius
 * \param angle0, angle1 IN start and end angle
 * \param drawCenter draw marking for center
 * \param width line width
 * \param lineType
 * \param color color
 * \param opts ?
 */


 void wDrawArc(
		wDraw_p bd,
		wPos_t x0, wPos_t y0,
		wPos_t r,
		wAngle_t angle0,
		wAngle_t angle1,
		int drawCenter,
		wDrawWidth width,
		wDrawLineType_e lineType,
		wDrawColor color,
		wDrawOpts opts )
{
	int x, y, w, h;

	if ( bd == &psPrint_d ) {
		psPrintArc( x0, y0, r, angle0, angle1, drawCenter, width, lineType, color, opts );
		return;
	}

	if (r < 6.0/75.0) return;
	x = INMAPX(bd,x0-r);
	y = INMAPY(bd,y0+r);
	w = 2*r;
	h = 2*r;

	// now create the new arc
	cairo_t* cairo = gtkDrawCreateCairoContext(bd, NULL, NULL, width, lineType, color, opts);
	cairo_new_path(cairo);

	// its center point marker
	if(drawCenter)
	{
		// draw a small crosshair to mark the center of the curve
		cairo_move_to(cairo,  INMAPX(bd, x0 - (CENTERMARK_LENGTH / 2)), INMAPY(bd, y0 ));
		cairo_line_to(cairo, INMAPX(bd, x0 + (CENTERMARK_LENGTH / 2)), INMAPY(bd, y0 ));
		cairo_move_to(cairo, INMAPX(bd, x0), INMAPY(bd, y0 - (CENTERMARK_LENGTH / 2 )));
		cairo_line_to(cairo, INMAPX(bd, x0) , INMAPY(bd, y0  + (CENTERMARK_LENGTH / 2)));
		cairo_new_sub_path( cairo );
	}

	// draw the curve itself
	cairo_arc_negative(cairo, INMAPX(bd, x0), INMAPY(bd, y0), r, (angle0 - 90 + angle1) * (M_PI / 180.0), (angle0 - 90) * (M_PI / 180.0));
	cairo_stroke(cairo);

	gtkDrawDestroyCairoContext(cairo, NULL);

}

 void wDrawPoint(
		wDraw_p bd,
		wPos_t x0, wPos_t y0,
		wDrawColor color,
		wDrawOpts opts )
{

	if ( bd == &psPrint_d ) {
		/*psPrintArc( x0, y0, r, angle0, angle1, drawCenter, width, lineType, color, opts );*/
		return;
	}

	cairo_t* cairo = gtkDrawCreateCairoContext(bd, NULL, NULL, 0, wDrawLineSolid, color, opts);
	cairo_new_path(cairo);
	cairo_arc(cairo, INMAPX(bd, x0), INMAPY(bd, y0), 0.75, 0, 2 * M_PI);
	cairo_stroke(cairo);
	gtkDrawDestroyCairoContext(cairo, NULL);

}

/*******************************************************************************
 *
 * Strings
 *
 ******************************************************************************/

 void wDrawString(
		wDraw_p bd,
		wPos_t x, wPos_t y,
		wAngle_t a,
		const char * s,
		wFont_p fp,
		wFontSize_t fs,
		wDrawColor color,
		wDrawOpts opts )
{
	PangoLayout *layout;
	int w;
	int h;
	gint ascent;
	gint descent;
	double angle = -M_PI * a / 180.0;

	if ( bd == &psPrint_d ) {
		psPrintString( x, y, a, (char *) s, fp, fs, color, opts );
		return;
	}

	x = INMAPX(bd,x);
	y = INMAPY(bd,y);

	/* draw text */
	cairo_t* cairo = gtkDrawCreateCairoContext(bd, NULL, NULL, 0, wDrawLineSolid, color, opts);

	cairo_save( cairo );
	cairo_translate( cairo, x, y );
	cairo_rotate( cairo, angle );

	layout = wlibFontCreatePangoLayout(bd->widget, cairo, fp, fs, s,
									  (int *) &w, (int *) &h,
									  (int *) &ascent, (int *) &descent);

	/* cairo does not support the old method of text removal by overwrite; force always write here and
           refresh on cancel event */
	GdkRGBA gcolor;
	long tcolor = wlibGetColor(color, TRUE);
	gcolor.red = (tcolor&0x00FF0000)>>16;
	gcolor.green = (tcolor&0x0000FF00)>>8;
	gcolor.blue = (tcolor&0x000000FF);

	cairo_set_source_rgba(cairo, gcolor.red, gcolor.green, gcolor.blue, 1.0);

	cairo_move_to( cairo, 0, -ascent );

	pango_cairo_show_layout(cairo, layout);
	wlibFontDestroyPangoLayout(layout);
	cairo_restore( cairo );
	gtkDrawDestroyCairoContext(cairo, NULL);

}

 void wDrawGetTextSize(
		wPos_t *w,
		wPos_t *h,
		wPos_t *d,
		wDraw_p bd,
		const char * s,
		wFont_p fp,
		wFontSize_t fs )
{
	int textWidth;
	int textHeight;
	int ascent;
	int descent;

	*w = 0;
	*h = 0;

	wlibFontDestroyPangoLayout(
		wlibFontCreatePangoLayout(bd->widget, NULL, fp, fs, s,
								 &textWidth, (int *) &textHeight,
								 (int *) &ascent, (int *) &descent));

	*w = (wPos_t) textWidth;
	*h = (wPos_t) ascent;
	*d = (wPos_t) descent;

	if (debugWindow >= 3)
		fprintf(stderr, "text metrics: w=%d, h=%d, d=%d\n", *w, *h, *d);
}


/*******************************************************************************
 *
 * Basic Drawing Functions
 *
*******************************************************************************/

 void wDrawFilledRectangle(
		wDraw_p bd,
		wPos_t x,
		wPos_t y,
		wPos_t w,
		wPos_t h,
		wDrawColor color,
		wDrawOpts opt )
{

	if ( bd == &psPrint_d ) {
		psPrintFillRectangle( x, y, w, h, color, opt );
		return;
	}

	x = INMAPX(bd,x);
	y = INMAPY(bd,y)-h;

	cairo_t* cairo = gtkDrawCreateCairoContext(bd, NULL, NULL, 0, wDrawLineSolid, color, opt);

	cairo_move_to(cairo, x, y);
	cairo_rel_line_to(cairo, w, 0);
	cairo_rel_line_to(cairo, 0, h);
	cairo_rel_line_to(cairo, -w, 0);
	cairo_rel_line_to(cairo, 0, -h);
	cairo_set_source_rgb(cairo, 0,0,0);
	cairo_set_operator(cairo, CAIRO_OPERATOR_DIFFERENCE);
	cairo_fill(cairo);
	GdkRGBA gcolor;
	long tcolor = wlibGetColor(color, TRUE);
	gcolor.red = (tcolor&0x00FF0000)>>16;
	gcolor.green = (tcolor&0x0000FF00)>>8;
	gcolor.blue = (tcolor&0x000000FF);
	cairo_set_source_rgba(cairo, gcolor.red, gcolor.green, gcolor.blue, 1.0);
	cairo_set_operator(cairo, CAIRO_OPERATOR_OVER);
	cairo_move_to(cairo, x, y);
    cairo_rel_line_to(cairo, w, 0);
    cairo_rel_line_to(cairo, 0, h);
    cairo_rel_line_to(cairo, -w, 0);
    cairo_rel_line_to(cairo, 0, -h);
	cairo_stroke(cairo);
	cairo_set_operator(cairo, CAIRO_OPERATOR_OVER);
	cairo_set_source_rgba(cairo, gcolor.red, gcolor.green, gcolor.blue, 0.3);
	cairo_move_to(cairo, x, y);
	cairo_rel_line_to(cairo, w, 0);
	cairo_rel_line_to(cairo, 0, h);
	cairo_rel_line_to(cairo, -w, 0);
	cairo_rel_line_to(cairo, 0, -h);
	cairo_fill(cairo);

	gtkDrawDestroyCairoContext(cairo, NULL);
	gtk_widget_queue_draw(GTK_WIDGET(bd->widget));

}

 void wDrawFilledPolygon(
		wDraw_p bd,
		wPos_t p[][2],
		int cnt,
		wDrawColor color,
		wDrawOpts opt )
{
	static int maxCnt = 0;
	static GdkPoint *points;
	int i;

	if ( bd == &psPrint_d ) {
		psPrintFillPolygon( p, cnt, color, opt );
		return;
	}

		if (cnt > maxCnt) {
		if (points == NULL)
			points = (GdkPoint*)malloc( cnt*sizeof *points );
		else
			points = (GdkPoint*)realloc( points, cnt*sizeof *points );
		if (points == NULL)
			abort();
		maxCnt = cnt;
	}
    for (i=0; i<cnt; i++) {
    	points[i].x = INMAPX(bd,p[i][0]);
    	points[i].y = INMAPY(bd,p[i][1]);
	}

	cairo_t* cairo = gtkDrawCreateCairoContext(bd, NULL, NULL, 0, wDrawLineSolid, color, opt);
	for(i = 0; i < cnt; ++i)
	{
		if(i)
			cairo_line_to(cairo, points[i].x, points[i].y);
		else
			cairo_move_to(cairo, points[i].x, points[i].y);
	}
	cairo_fill(cairo);
	gtkDrawDestroyCairoContext(cairo, NULL);

}

 void wDrawFilledCircle(
		wDraw_p bd,
		wPos_t x0,
		wPos_t y0,
		wPos_t r,
		wDrawColor color,
		wDrawOpts opt )
{
	int x, y, w, h;

	if ( bd == &psPrint_d ) {
		psPrintFillCircle( x0, y0, r, color, opt );
		return;
	}

	x = INMAPX(bd,x0-r);
	y = INMAPY(bd,y0+r);
	w = 2*r;
	h = 2*r;

	cairo_t* cairo = gtkDrawCreateCairoContext(bd, NULL, NULL, 0, wDrawLineSolid, color, opt);
	cairo_arc(cairo, INMAPX(bd, x0), INMAPY(bd, y0), r, 0, 2 * M_PI);
	cairo_fill(cairo);
	gtkDrawDestroyCairoContext(cairo, NULL);

}


 void wDrawClear(
		wDraw_p bd )
{

	cairo_t* cairo = gtkDrawCreateCairoContext(bd, NULL, NULL, 0, wDrawLineSolid, wDrawColorWhite, 0);
	cairo_move_to(cairo, 0, 0);
	cairo_rel_line_to(cairo, bd->w, 0);
	cairo_rel_line_to(cairo, 0, bd->h);
	cairo_rel_line_to(cairo, -bd->w, 0);
	cairo_fill(cairo);
	gtkDrawDestroyCairoContext(cairo, NULL);

}

 void * wDrawGetContext(
		wDraw_p bd )
{
	return bd->context;
}

/*******************************************************************************
 *
 * Bit Maps
 *
*******************************************************************************/


 wDrawBitMap_p wDrawBitMapCreate(
		wDraw_p bd,
		int w,
		int h,
		int x,
		int y,
		const char * fbits )
{
	wDrawBitMap_p bm;

	bm = (wDrawBitMap_p)malloc( sizeof *bm );
	bm->w = w;
	bm->h = h;
	/*bm->pixmap = gtkMakeIcon( NULL, fbits, w, h, wDrawColorBlack, &bm->mask );*/
	bm->bits = fbits;
	bm->x = x;
	bm->y = y;
	return bm;
}


 void wDrawBitMap(
		wDraw_p bd,
		wDrawBitMap_p bm,
		wPos_t x, wPos_t y,
		wDrawColor color,
		wDrawOpts opts )
{
	GdkRectangle update_rect;
    
	int i, j, wb;
	wPos_t xx, yy;
	wControl_p b;
	GdkPixbuf * gdk_pixbuf, * cairo_pixbuf;
	cairo_surface_t * surface = NULL;

	x = INMAPX( bd, x-bm->x );
	y = INMAPY( bd, y-bm->y )-bm->h;
	wb = (bm->w+7)/8;


	cairo_t* cairo = gtkDrawCreateCairoContext(bd, NULL, NULL, 0, wDrawLineSolid, color, opts);
	cairo_pixbuf = bd->pixbuf;


	for ( i=0; i<bm->w; i++ )
		for ( j=0; j<bm->h; j++ )
			if ( bm->bits[ j*wb+(i>>3) ] & (1<<(i&07)) ) {
				xx = x+i;
				yy = y+j;
				if ( 0 <= xx && xx < bd->w &&
					 0 <= yy && yy < bd->h ) {
					gdk_pixbuf = bd->pixbuf;
					b = (wControl_p)bd;
				} else if ( (opts&wDrawOptNoClip) != 0 ) {
					xx += bd->realX;
					yy += bd->realY;
					b = wlibGetControlFromPos( bd->parent, xx, yy );
					if ( b ) {
						if ( b->type == B_DRAW )
							gdk_pixbuf = ((wDraw_p)b)->pixbuf;
						else
							gdk_pixbuf = gdk_pixbuf_get_from_window(gtk_widget_get_window(bd->widget),xx,yy, bd->w, bd->h);
						xx -= b->realX;
						yy -= b->realY;
					} else {
						gdk_pixbuf = gdk_pixbuf_get_from_window(gtk_widget_get_window(bd->widget),xx,yy, bd->w, bd->h);
					}
				} else {
					continue;
				}
				if (gdk_pixbuf != cairo_pixbuf) {
					cairo_destroy(cairo);
					cairo = gtkDrawCreateCairoContext(bd, gdk_pixbuf, surface, 0, wDrawLineSolid, color, opts);
					cairo_pixbuf = gdk_pixbuf;
				}
				cairo_rectangle(cairo, xx-0.5, yy-0.5, 1, 1);
				cairo_fill(cairo);
				if ( b && b->type == B_DRAW ) {
				    gtk_widget_queue_draw_area( b->widget, xx-1, yy-1, 3, 3 );
				}
			}
	gtkDrawDestroyCairoContext(cairo,surface);
}


/*******************************************************************************
 *
 * Event Handlers
 *
*******************************************************************************/



 void wDrawSaveImage(
		wDraw_p bd )
{
	if ( bd->pixbufBackup ) {
		g_object_unref( bd->pixbufBackup );
	}
	bd->pixbufBackup = gdk_pixbuf_get_from_surface( bd->surface, 0, 0, bd->w, bd->h );

}


 void wDrawRestoreImage(
		wDraw_p bd )
{
	if ( bd->pixbufBackup ) {

		cairo_t * cr;
		cr = cairo_create(bd->surface);
		gdk_cairo_set_source_pixbuf(cr, bd->pixbufBackup, 0, 0);
		cairo_paint(cr);
		cairo_destroy(cr);

		cr = NULL;

		if ( bd->delayUpdate || bd->widget == NULL ) return;
		gtk_widget_queue_draw_area( bd->widget, 0, 0, bd->w, bd->h );
	}
}


 void wDrawSetSize(
		wDraw_p bd,
		wPos_t w,
		wPos_t h )
{
	wBool_t repaint;
	if (bd == NULL) {
		fprintf(stderr,"resizeDraw: no client data\n");
		return;
	}

	/* Negative values crashes the program */
	if (w < 0 || h < 0)
		return;

	repaint = (w != bd->w || h != bd->h);
	bd->w = w;
	bd->h = h;
	gtk_widget_set_size_request( bd->widget, w, h );
	if (repaint)
	{
		if (bd->surface)
			cairo_surface_destroy(bd->surface);
		bd->surface = gdk_window_create_similar_surface(gtk_widget_get_window (bd->widget), CAIRO_CONTENT_COLOR, w, h);

		wDrawClear( bd );
		/*bd->redraw( bd, bd->context, w, h );*/
	}
	/*wRedraw( bd );*/
}


 void wDrawGetSize(
		wDraw_p bd,
		wPos_t *w,
		wPos_t *h )
{
	if (bd->widget)
		wlibControlGetSize( (wControl_p)bd );
	*w = bd->w-2;
	*h = bd->h-2;
}

/**
 * Return the resolution of a device in dpi
 *
 * \param d IN the device
 * \return    the resolution in dpi
 */

 double wDrawGetDPI(
		wDraw_p d )
{
	//if (d == &psPrint_d)
		//return 1440.0;
	//else
		return d->dpi;
}


 double wDrawGetMaxRadius(
		wDraw_p d )
{
	if (d == &psPrint_d)
		return 10e9;
	else
		return 32767.0;
}


 void wDrawClip(
		wDraw_p d,
		wPos_t x,
		wPos_t y,
		wPos_t w,
		wPos_t h )
{
	d->rect.width = w;
	d->rect.height = h;
	d->rect.x = INMAPX( d, x );
	d->rect.y = INMAPY( d, y ) - d->rect.height;
	d->clip_set = TRUE;

}


static gboolean draw_event(
		GtkWidget *widget,
		cairo_t * cr,
		wDraw_p bd)
{
	cairo_set_source_surface (cr, bd->surface, 0, 0);
	cairo_paint (cr);
	return FALSE;
}

static void
clear_surface (cairo_surface_t * surface)
{
  cairo_t *cr;

  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_destroy (cr);
}


static gboolean draw_configure_event(
	GtkWidget *widget,
		GdkEventConfigure *event,
		wDraw_p bd)
{
	if (bd->surface)
	    cairo_surface_destroy (bd->surface);

	  bd->surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
	                                               CAIRO_CONTENT_COLOR,
	                                               gtk_widget_get_allocated_width (widget),
	                                               gtk_widget_get_allocated_height (widget));

	  /* Initialize the surface to white */
	  clear_surface (bd->surface);

	  /* We've handled the configure event, no need for further processing. */
	  return TRUE;
}

static const char * actionNames[] = { "None", "Move", "LDown", "LDrag", "LUp", "RDown", "RDrag", "RUp", "Text", "ExtKey", "WUp", "WDown" };

/**
 * Handler for scroll events, ie mouse wheel activity
 */

static gint draw_scroll_event(
		GtkWidget *widget,
		GdkEventScroll *event,
		wDraw_p bd)
{
	wAction_t action;

	switch( event->direction ) {
	case GDK_SCROLL_UP:
		action = wActionWheelUp;
		break;
	case GDK_SCROLL_DOWN:
		action = wActionWheelDown;
		break;
	default:
		action = 0;
		break;
	}

	if (action != 0) {
		if (drawVerbose >= 2)
			printf( "%s[%dx%d]\n", actionNames[action], bd->lastX, bd->lastY );
		bd->action( bd, bd->context, action, bd->lastX, bd->lastY );
	}

	return FALSE;
}



static gint draw_leave_event(
		GtkWidget *widget,
		GdkEvent * event )
{
	wlibHelpHideBalloon();
	return FALSE;
}


/**
 * Handler for mouse button clicks.
 */

static gint draw_button_event(
		GtkWidget *widget,
		GdkEventButton *event,
		wDraw_p bd )
{
	wAction_t action = 0;
	if (bd->action == NULL)
		return TRUE;

	bd->lastX = OUTMAPX(bd, event->x);
	bd->lastY = OUTMAPY(bd, event->y);

	switch ( event->button ) {
	case 1: /* left mouse button */
		action = event->type==GDK_BUTTON_PRESS?wActionLDown:wActionLUp;
		/*bd->action( bd, bd->context, event->type==GDK_BUTTON_PRESS?wActionLDown:wActionLUp, bd->lastX, bd->lastY );*/
		break;
	case 3: /* right mouse button */
		action = event->type==GDK_BUTTON_PRESS?wActionRDown:wActionRUp;
		/*bd->action( bd, bd->context, event->type==GDK_BUTTON_PRESS?wActionRDown:wActionRUp, bd->lastX, bd->lastY );*/
		break;
	}
	if (action != 0) {
		if (drawVerbose >= 2)
			printf( "%s[%dx%d]\n", actionNames[action], bd->lastX, bd->lastY );
		bd->action( bd, bd->context, action, bd->lastX, bd->lastY );
	}
	if (!(bd->option & BD_NOFOCUS))
		gtk_widget_grab_focus( bd->widget );
	return FALSE;
}

static gint draw_motion_event(
		GtkWidget *widget,
		GdkEventMotion *event,
		wDraw_p bd )
{
	int x, y;
	GdkModifierType state;
	wAction_t action;

	if (bd->action == NULL)
		return TRUE;

	if (event->is_hint) {
		gdk_window_get_device_position (event->window, event->device, &x, &y, &state);
	} else {
		x = event->x;
		y = event->y;
		state = event->state;
	}

	if (state & GDK_BUTTON1_MASK) {
		action = wActionLDrag;
	} else if (state & GDK_BUTTON3_MASK) {
		action = wActionRDrag;
	} else {
		action = wActionMove;
	}
	bd->lastX = OUTMAPX(bd, x);
	bd->lastY = OUTMAPY(bd, y);
	if (drawVerbose >= 2)
		printf( "%lx: %s[%dx%d] %s\n", (long)bd, actionNames[action], bd->lastX, bd->lastY, event->is_hint?"<Hint>":"<>" );
	bd->action( bd, bd->context, action, bd->lastX, bd->lastY );
	if (!(bd->option & BD_NOFOCUS))
		gtk_widget_grab_focus( bd->widget );
	return FALSE;
}


static gint draw_char_event(
		GtkWidget * widget,
		GdkEventKey *event,
		wDraw_p bd )
{
	guint key = event->keyval;
	wAccelKey_e extKey = wAccelKey_None;
	switch (key) {
	case GDK_KEY_Escape:	key = 0x1B; break;
	case GDK_KEY_Return:	key = 0x0D; break;
	case GDK_KEY_Linefeed:	key = 0x0A; break;
	case GDK_KEY_Tab:	key = 0x09; break;
	case GDK_KEY_BackSpace:	key = 0x08; break;
	case GDK_KEY_Delete:    extKey = wAccelKey_Del; break;
	case GDK_KEY_Insert:    extKey = wAccelKey_Ins; break;
	case GDK_KEY_Home:      extKey = wAccelKey_Home; break;
	case GDK_KEY_End:       extKey = wAccelKey_End; break;
	case GDK_KEY_Page_Up:   extKey = wAccelKey_Pgup; break;
	case GDK_KEY_Page_Down: extKey = wAccelKey_Pgdn; break;
	case GDK_KEY_Up:        extKey = wAccelKey_Up; break;
	case GDK_KEY_Down:      extKey = wAccelKey_Down; break;
	case GDK_KEY_Right:     extKey = wAccelKey_Right; break;
	case GDK_KEY_Left:      extKey = wAccelKey_Left; break;
	case GDK_KEY_F1:        extKey = wAccelKey_F1; break;
	case GDK_KEY_F2:        extKey = wAccelKey_F2; break;
	case GDK_KEY_F3:        extKey = wAccelKey_F3; break;
	case GDK_KEY_F4:        extKey = wAccelKey_F4; break;
	case GDK_KEY_F5:        extKey = wAccelKey_F5; break;
	case GDK_KEY_F6:        extKey = wAccelKey_F6; break;
	case GDK_KEY_F7:        extKey = wAccelKey_F7; break;
	case GDK_KEY_F8:        extKey = wAccelKey_F8; break;
	case GDK_KEY_F9:        extKey = wAccelKey_F9; break;
	case GDK_KEY_F10:       extKey = wAccelKey_F10; break;
	case GDK_KEY_F11:       extKey = wAccelKey_F11; break;
	case GDK_KEY_F12:       extKey = wAccelKey_F12; break;
	default: ;
	}

	if (extKey != wAccelKey_None) {
		if ( wlibFindAccelKey( event ) == NULL ) {
			bd->action( bd, bd->context, wActionExtKey + ((int)extKey<<8), bd->lastX, bd->lastY );
		}
		return FALSE;
	} else if (key <= 0xFF && (event->state&(GDK_CONTROL_MASK|GDK_MOD1_MASK)) == 0 && bd->action) {
		bd->action( bd, bd->context, wActionText+(key<<8), bd->lastX, bd->lastY );
		return FALSE;
	} else {
		return FALSE;
	}
}


/*******************************************************************************
 *
 * Create
 *
*******************************************************************************/



int XW = 0;
int XH = 0;
int xw, xh, cw, ch;

 wDraw_p wDrawCreate(
		wWin_p	parent,
		wPos_t	x,
		wPos_t	y,
		const char 	* helpStr,
		long	option,
		wPos_t	width,
		wPos_t	height,
		void	* context,
		wDrawRedrawCallBack_p redraw,
		wDrawActionCallBack_p action )
{
	wDraw_p bd;

	bd = (wDraw_p)wlibAlloc( parent,  B_DRAW, x, y, NULL, sizeof *bd, NULL );
	bd->option = option;
	bd->context = context;
	bd->redraw = redraw;
	bd->action = action;
	wlibComputePos( (wControl_p)bd );

	if (option&BO_USETEMPLATE) {
		char name[256];
		sprintf(name,"%s",helpStr);
		bd->widget = wlibWidgetFromId( parent, name );
		if (bd->widget) bd->fromTemplate = TRUE;
	}
	if (!bd->widget)
		bd->widget = gtk_drawing_area_new();

	gtk_widget_set_size_request( GTK_WIDGET(bd->widget), width, height );
	g_signal_connect ((bd->widget), "draw",
						   G_CALLBACK(draw_event), bd);
	g_signal_connect ((bd->widget),"configure_event",
						   G_CALLBACK(draw_configure_event), bd);
	g_signal_connect ((bd->widget), "motion_notify_event",
						   G_CALLBACK( draw_motion_event), bd);
	g_signal_connect ((bd->widget), "button_press_event",
						   G_CALLBACK( draw_button_event), bd);
	g_signal_connect ((bd->widget), "button_release_event",
						   G_CALLBACK( draw_button_event), bd);
	g_signal_connect ((bd->widget), "scroll_event",
						   G_CALLBACK( draw_scroll_event), bd);
	g_signal_connect_after ((bd->widget), "key_press_event",
						   G_CALLBACK( draw_char_event), bd);
	g_signal_connect ((bd->widget), "leave_notify_event",
						   G_CALLBACK( draw_leave_event), bd);
	gtk_widget_set_can_focus(bd->widget,!(option & BD_NOFOCUS));
	//if (!(option & BD_NOFOCUS))
	//	GTK_WIDGET_SET_FLAGS(GTK_WIDGET(bd->widget), GTK_CAN_FOCUS);
	gtk_widget_set_events (bd->widget,
							  GDK_LEAVE_NOTIFY_MASK
							  | GDK_BUTTON_PRESS_MASK
							  | GDK_BUTTON_RELEASE_MASK
/*							  | GDK_SCROLL_MASK */
							  | GDK_POINTER_MOTION_MASK
							  | GDK_POINTER_MOTION_HINT_MASK
							  | GDK_KEY_PRESS_MASK
							  | GDK_KEY_RELEASE_MASK );
	bd->lastColor = -1;
	bd->dpi = 75;
	bd->maxW = bd->w = width;
	bd->maxH = bd->h = height;

	if (option&BO_CONTROLGRID) {
		g_object_ref(bd->widget);
		bd->useGrid = TRUE;
	} else if (!bd->fromTemplate) {
		gtk_fixed_put( GTK_FIXED(parent->widget), bd->widget, bd->realX, bd->realY );
		wlibControlGetSize( (wControl_p)bd );
	}
	gtk_widget_realize( bd->widget );
	bd->surface = gdk_window_create_similar_surface(gtk_widget_get_window(bd->widget), CAIRO_CONTENT_COLOR, width, height);
	//bd->gc = gdk_gc_new( parent->gtkwin->window );
	//gdk_gc_copy( bd->gc, parent->gtkwin->style->base_gc[GTK_STATE_NORMAL] );
{
	GdkCursor * cursor;
	cursor = gdk_cursor_new_for_display ( gdk_display_get_default(), GDK_TCROSS );
	gdk_window_set_cursor ( gtk_widget_get_window(GTK_WIDGET(bd->widget)), cursor);
	g_object_unref (cursor);
}
#ifdef LATER
	if (labelStr)
		bd->labelW = gtkAddLabel( (wControl_p)bd, labelStr );
#endif
	gtk_widget_show( bd->widget );
	wlibAddButton( (wControl_p)bd );
	gtkAddHelpString( bd->widget, helpStr );

	return bd;
}

/*******************************************************************************
 *
 * BitMaps
 *
*******************************************************************************/

wDraw_p wBitMapCreate(          wPos_t w, wPos_t h, int arg )
{
	wDraw_p bd;

	bd = (wDraw_p)wlibAlloc( gtkMainW,  B_DRAW, 0, 0, NULL, sizeof *bd, NULL );

	bd->lastColor = -1;
	bd->dpi = 75;
	bd->maxW = bd->w = w;
	bd->maxH = bd->h = h;
	bd->clip_set = FALSE;

	bd->pixbuf = gdk_pixbuf_get_from_window( gtk_widget_get_window(GTK_WIDGET(gtkMainW->gtkwin)), 0, 0, w, h );
	if ( bd->pixbuf == NULL ) {
		wNoticeEx( NT_ERROR, "CreateBitMap: pixmap_new failed", "Ok", NULL );
		return FALSE;
	}
	//bd->gc = gdk_gc_new( gtkMainW->gtkwin->window );
	//if ( bd->gc == NULL ) {
	//	wNoticeEx( NT_ERROR, "CreateBitMap: gc_new failed", "Ok", NULL );
	//	return FALSE;
	//}
	//gdk_gc_copy( bd->gc, gtkMainW->gtkwin->style->base_gc[GTK_STATE_NORMAL] );

	wDrawClear( bd );
	return bd;
}


wBool_t wBitMapDelete(          wDraw_p d )
{
	g_object_unref( d->pixbuf );
	d->pixbuf = NULL;
	d->clip_set = FALSE;
	return TRUE;
}
