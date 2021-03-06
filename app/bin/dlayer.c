/** \file dlayer.c
 * Functions and dialogs for handling layers.
 */

/*  XTrkCad - Model Railroad CAD
 *  Copyright (C) 2005 Dave Bullis and (C) 2007 Martin Fischer
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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WINDOWS
#include "include/dirent.h"
#else
#include <dirent.h>
#endif


#include "custom.h"
#include "paths.h"
#include "dynstring.h"
#include "fileio.h"
#include "i18n.h"
#include "layout.h"
#include "messages.h"
#include "param.h"
#include "track.h"
#include "include/partcatalog.h"
#include "include/stringxtc.h"

/*****************************************************************************
 *
 * LAYERS
 *
 */

#define NUM_BUTTONS		(99)
#define LAYERPREF_FROZEN  (1)
#define LAYERPREF_ONMAP	  (2)
#define LAYERPREF_VISIBLE (4)
#define LAYERPREF_MODULE  (8)
#define LAYERPREF_NOBUTTON (16)
#define LAYERPREF_SECTION ("Layers")
#define LAYERPREF_NAME 	"name"
#define LAYERPREF_COLOR "color"
#define LAYERPREF_FLAGS "flags"
#define LAYERPREF_LIST "list"
#define LAYERPREF_SETTINGS "settings"

unsigned int curLayer;
long layerCount = 10;
static long newLayerCount = 10;
static unsigned int layerCurrent = NUM_LAYERS;


static BOOL_T layoutLayerChanged = FALSE;

static wIcon_p show_layer_bmps[NUM_BUTTONS];
static wButton_p layer_btns[NUM_BUTTONS];	/**< layer buttons on toolbar */

/** Layer selector on toolbar */
static wList_p setLayerL;

/** Describe the properties of a layer */
typedef struct {
    char name[STR_SHORT_SIZE];			/**< Layer name */
    wDrawColor color;					/**< layer color, is an index into a color table */
    BOOL_T useColor;					/**< Use Layer color */
    BOOL_T frozen;						/**< Frozen flag */
    BOOL_T visible;						/**< visible flag */
    BOOL_T onMap;						/**< is layer shown map */
    BOOL_T module;						/**< is layer a module (all or nothing) */
    BOOL_T button_off;					/**< hide button */
    long objCount;						/**< number of objects on layer */
    dynArr_t layerLinkList;				/**< other layers that show/hide with this one */
    char settingsName[STR_SHORT_SIZE];  /**< name of settings file to load when this is current */
} layer_t;

static layer_t layers[NUM_LAYERS];
static layer_t *layers_save = NULL;

static Catalog * settingsCatalog;


static int oldColorMap[][3] = {
    { 255, 255, 255 },		/* White */
    {   0,   0,   0 },      /* Black */
    { 255,   0,   0 },      /* Red */
    {   0, 255,   0 },      /* Green */
    {   0,   0, 255 },      /* Blue */
    { 255, 255,   0 },      /* Yellow */
    { 255,   0, 255 },      /* Purple */
    {   0, 255, 255 },      /* Aqua */
    { 128,   0,   0 },      /* Dk. Red */
    {   0, 128,   0 },      /* Dk. Green */
    {   0,   0, 128 },      /* Dk. Blue */
    { 128, 128,   0 },      /* Dk. Yellow */
    { 128,   0, 128 },      /* Dk. Purple */
    {   0, 128, 128 },      /* Dk. Aqua */
    {  65, 105, 225 },      /* Royal Blue */
    {   0, 191, 255 },      /* DeepSkyBlue */
    { 125, 206, 250 },      /* LightSkyBlue */
    {  70, 130, 180 },      /* Steel Blue */
    { 176, 224, 230 },      /* Powder Blue */
    { 127, 255, 212 },      /* Aquamarine */
    {  46, 139,  87 },      /* SeaGreen */
    { 152, 251, 152 },      /* PaleGreen */
    { 124, 252,   0 },      /* LawnGreen */
    {  50, 205,  50 },      /* LimeGreen */
    {  34, 139,  34 },      /* ForestGreen */
    { 255, 215,   0 },      /* Gold */
    { 188, 143, 143 },      /* RosyBrown */
    { 139, 69, 19 },        /* SaddleBrown */
    { 245, 245, 220 },      /* Beige */
    { 210, 180, 140 },      /* Tan */
    { 210, 105, 30 },       /* Chocolate */
    { 165, 42, 42 },        /* Brown */
    { 255, 165, 0 },        /* Orange */
    { 255, 127, 80 },       /* Coral */
    { 255, 99, 71 },        /* Tomato */
    { 255, 105, 180 },      /* HotPink */
    { 255, 192, 203 },      /* Pink */
    { 176, 48, 96 },        /* Maroon */
    { 238, 130, 238 },      /* Violet */
    { 160, 32, 240 },       /* Purple */
    {  16,  16,  16 },      /* Gray */
    {  32,  32,  32 },      /* Gray */
    {  48,  48,  48 },      /* Gray */
    {  64,  64,  64 },      /* Gray */
    {  80,  80,  80 },      /* Gray */
    {  96,  96,  96 },      /* Gray */
    { 112, 112, 122 },      /* Gray */
    { 128, 128, 128 },      /* Gray */
    { 144, 144, 144 },      /* Gray */
    { 160, 160, 160 },      /* Gray */
    { 176, 176, 176 },      /* Gray */
    { 192, 192, 192 },      /* Gray */
    { 208, 208, 208 },      /* Gray */
    { 224, 224, 224 },      /* Gray */
    { 240, 240, 240 },      /* Gray */
    {   0,   0,   0 }       /* BlackPixel */
};

static void DoLayerOp(void * data);
static void UpdateLayerDlg(void);

static void InitializeLayers(void LayerInitFunc(void), int newCurrLayer);
static void LayerPrefSave(void);
static void LayerPrefLoad(void);

int IsLayerValid(unsigned int layer)
{
    return (layer <= NUM_LAYERS);
}

BOOL_T GetLayerVisible(unsigned int layer)
{
    if (!IsLayerValid(layer)) {
        return TRUE;
    } else {
        return layers[layer].visible;
    }
}

BOOL_T GetLayerHidden(unsigned int layer)
{
	if (!IsLayerValid(layer)) {
		return TRUE;
	} else {
		return layers[layer].button_off;
	}
}


BOOL_T GetLayerFrozen(unsigned int layer)
{
    if (!IsLayerValid(layer)) {
        return TRUE;
    } else {
        return layers[layer].frozen;
    }
}


BOOL_T GetLayerOnMap(unsigned int layer)
{
    if (!IsLayerValid(layer)) {
        return TRUE;
    } else {
        return layers[layer].onMap;
    }
}

BOOL_T GetLayerModule(unsigned int layer)
{
    if (!IsLayerValid(layer)) {
        return TRUE;
    } else {
        return layers[layer].module;
    }
}

void SetLayerModule(unsigned int layer, BOOL_T module)
{
	if (IsLayerValid(layer)) {
		layers[layer].module = module;
	}
}


char * GetLayerName(unsigned int layer)
{
    if (!IsLayerValid(layer)) {
        return NULL;
    } else {
        return layers[layer].name;
    }
}

void SetLayerName(unsigned int layer, char* name) {
	if (IsLayerValid(layer)) {
		strcpy(layers[layer].name,name);
	}
}

BOOL_T GetLayerUseColor(unsigned int layer) {
	return layers[layer].useColor;
}

wDrawColor GetLayerColor(unsigned int layer)
{
    return layers[layer].color;
}

static void RedrawLayer( unsigned int l, BOOL_T draw )
{
	DoRedraw(); // RedrawLayer
}


static void FlipLayer(unsigned int layer)
{
    wBool_t visible;

    if (!IsLayerValid(layer)) {
        return;
    }

    if (layer == curLayer && layers[layer].visible) {
    	if (!layers[layer].button_off)
    		wButtonSetBusy(layer_btns[layer], layers[layer].visible);
        NoticeMessage(MSG_LAYER_HIDE, _("Ok"), NULL);
        return;
    }

    RedrawLayer(layer, FALSE);
    visible = !layers[layer].visible;
    layers[layer].visible = visible;

    if (layer<NUM_BUTTONS) {
    	if (!layers[layer].button_off) {
    		wButtonSetBusy(layer_btns[layer], visible != 0);
    		wButtonSetLabel(layer_btns[layer], (char *)show_layer_bmps[layer]);
    	}
    }

    /* Set visible on related layers other than current */
     for (int i=0;i<layers[layer].layerLinkList.cnt;i++) {
        int l = DYNARR_N(int,layers[layer].layerLinkList,i)-1;
        if ((l != curLayer) && (l >=0) && (l < NUM_LAYERS)) {
			layers[l].visible = layers[layer].visible;
			if (!layers[l].button_off)
				wButtonSetBusy(layer_btns[l], layers[l].visible);
		}
	}

    RedrawLayer(layer, TRUE);
}

static char lastSettings[STR_SHORT_SIZE];
void SetCurrLayer(wIndex_t inx, const char * name, wIndex_t op,
                         void * listContext, void * arg)
{
    unsigned int newLayer = (unsigned int)inx;

    if (layers[newLayer].frozen) {
        NoticeMessage(MSG_LAYER_SEL_FROZEN, _("Ok"), NULL);
        wListSetIndex(setLayerL, curLayer);
        return;
    }

    char *array[1];
    if (!layers[inx].settingsName[0] || strcmp(layers[inx].settingsName," ")==0) {
    	if (lastSettings[0]) {
    		DoSettingsRead(1,NULL, NULL);
    	}
    	lastSettings[0] = '\0';
    } else {
    	if (strcmp(layers[inx].settingsName,lastSettings)!=0) {
    		if (!lastSettings[0]) wPrefFlush("");  // Save Last Settings for no settings file
    		array[0] = layers[inx].settingsName;
    		DoSettingsRead(1,array, NULL);
    	}
    	strcpy(lastSettings,layers[inx].settingsName);
    }

    curLayer = newLayer;

    if (!IsLayerValid(curLayer)) {
        curLayer = 0;
    }

    if (!layers[curLayer].visible) {
        FlipLayer(inx);
    }

    /* Set visible on related layers other than current */
	 for (int i=0;i<layers[curLayer].layerLinkList.cnt;i++) {
		int l = DYNARR_N(int,layers[curLayer].layerLinkList,i)-1;
		if (l != curLayer && l >=0 && l < NUM_LAYERS) {
			layers[l].visible = layers[curLayer].visible;
			if (!layers[l].button_off)
				wButtonSetBusy(layer_btns[l], layers[l].visible);
		}
	}

    if (recordF) {
        fprintf(recordF, "SETCURRLAYER %d\n", inx);
    }
}

static void PlaybackCurrLayer(char * line)
{
    wIndex_t layer;
    layer = atoi(line);
    wListSetIndex(setLayerL, layer);
    SetCurrLayer(layer, NULL, 0, NULL, NULL);
}

/**
 * Change the color of a layer.
 *
 * \param inx IN layer to change
 * \param color IN new color
 */

static void SetLayerColor(unsigned int inx, wDrawColor color)
{
    if (color != layers[inx].color) {
        if (inx < NUM_BUTTONS) {
            wIconSetColor(show_layer_bmps[inx], color);
            wButtonSetLabel(layer_btns[inx], (char*)show_layer_bmps[inx]);
        }

        layers[inx].color = color;
        layoutLayerChanged = TRUE;
    }
}

static void SetLayerHideButton(unsigned int inx, wBool_t hide) {
	if (hide != layers[inx].button_off) {
		if (inx < NUM_BUTTONS) {
			wControlShow((wControl_p)layer_btns[inx],!hide);
			if (!hide) wButtonSetBusy(layer_btns[inx], layers[inx].visible);
		}
		layers[inx].button_off = hide;
		layoutLayerChanged = TRUE;
	}
}

char *
FormatLayerName(unsigned int layerNumber)
{
    DynString string;// = NaS;
    char *result;
    DynStringMalloc(&string, 0);
    DynStringPrintf(&string,
                    "%2d %c %s",
                    layerNumber + 1,
                    (layers[layerNumber].objCount > 0 ? '+' : '-'),
                    layers[layerNumber].name);
    result = strdup(DynStringToCStr(&string));
    DynStringFree(&string);
    return result;
}


#include "bitmaps/l1.xbm"
#include "bitmaps/l2.xbm"
#include "bitmaps/l3.xbm"
#include "bitmaps/l4.xbm"
#include "bitmaps/l5.xbm"
#include "bitmaps/l6.xbm"
#include "bitmaps/l7.xbm"
#include "bitmaps/l8.xbm"
#include "bitmaps/l9.xbm"
#include "bitmaps/l10.xbm"
#include "bitmaps/l11.xbm"
#include "bitmaps/l12.xbm"
#include "bitmaps/l13.xbm"
#include "bitmaps/l14.xbm"
#include "bitmaps/l15.xbm"
#include "bitmaps/l16.xbm"
#include "bitmaps/l17.xbm"
#include "bitmaps/l18.xbm"
#include "bitmaps/l19.xbm"
#include "bitmaps/l20.xbm"
#include "bitmaps/l21.xbm"
#include "bitmaps/l22.xbm"
#include "bitmaps/l23.xbm"
#include "bitmaps/l24.xbm"
#include "bitmaps/l25.xbm"
#include "bitmaps/l26.xbm"
#include "bitmaps/l27.xbm"
#include "bitmaps/l28.xbm"
#include "bitmaps/l29.xbm"
#include "bitmaps/l30.xbm"
#include "bitmaps/l31.xbm"
#include "bitmaps/l32.xbm"
#include "bitmaps/l33.xbm"
#include "bitmaps/l34.xbm"
#include "bitmaps/l35.xbm"
#include "bitmaps/l36.xbm"
#include "bitmaps/l37.xbm"
#include "bitmaps/l38.xbm"
#include "bitmaps/l39.xbm"
#include "bitmaps/l40.xbm"
#include "bitmaps/l41.xbm"
#include "bitmaps/l42.xbm"
#include "bitmaps/l43.xbm"
#include "bitmaps/l44.xbm"
#include "bitmaps/l45.xbm"
#include "bitmaps/l46.xbm"
#include "bitmaps/l47.xbm"
#include "bitmaps/l48.xbm"
#include "bitmaps/l49.xbm"
#include "bitmaps/l50.xbm"
#include "bitmaps/l51.xbm"
#include "bitmaps/l52.xbm"
#include "bitmaps/l53.xbm"
#include "bitmaps/l54.xbm"
#include "bitmaps/l55.xbm"
#include "bitmaps/l56.xbm"
#include "bitmaps/l57.xbm"
#include "bitmaps/l58.xbm"
#include "bitmaps/l59.xbm"
#include "bitmaps/l60.xbm"
#include "bitmaps/l61.xbm"
#include "bitmaps/l62.xbm"
#include "bitmaps/l63.xbm"
#include "bitmaps/l64.xbm"
#include "bitmaps/l65.xbm"
#include "bitmaps/l66.xbm"
#include "bitmaps/l67.xbm"
#include "bitmaps/l68.xbm"
#include "bitmaps/l69.xbm"
#include "bitmaps/l70.xbm"
#include "bitmaps/l71.xbm"
#include "bitmaps/l72.xbm"
#include "bitmaps/l73.xbm"
#include "bitmaps/l74.xbm"
#include "bitmaps/l75.xbm"
#include "bitmaps/l76.xbm"
#include "bitmaps/l77.xbm"
#include "bitmaps/l78.xbm"
#include "bitmaps/l79.xbm"
#include "bitmaps/l80.xbm"
#include "bitmaps/l81.xbm"
#include "bitmaps/l82.xbm"
#include "bitmaps/l83.xbm"
#include "bitmaps/l84.xbm"
#include "bitmaps/l85.xbm"
#include "bitmaps/l86.xbm"
#include "bitmaps/l87.xbm"
#include "bitmaps/l88.xbm"
#include "bitmaps/l89.xbm"
#include "bitmaps/l90.xbm"
#include "bitmaps/l91.xbm"
#include "bitmaps/l92.xbm"
#include "bitmaps/l93.xbm"
#include "bitmaps/l94.xbm"
#include "bitmaps/l95.xbm"
#include "bitmaps/l96.xbm"
#include "bitmaps/l97.xbm"
#include "bitmaps/l98.xbm"
#include "bitmaps/l99.xbm"


static char * show_layer_bits[NUM_BUTTONS] = {
    l1_bits, l2_bits, l3_bits, l4_bits, l5_bits, l6_bits, l7_bits, l8_bits, l9_bits, l10_bits,
    l11_bits, l12_bits, l13_bits, l14_bits, l15_bits, l16_bits, l17_bits, l18_bits, l19_bits, l20_bits,
    l21_bits, l22_bits, l23_bits, l24_bits, l25_bits, l26_bits, l27_bits, l28_bits, l29_bits, l30_bits,
    l31_bits, l32_bits, l33_bits, l34_bits, l35_bits, l36_bits, l37_bits, l38_bits, l39_bits, l40_bits,
    l41_bits, l42_bits, l43_bits, l44_bits, l45_bits, l46_bits, l47_bits, l48_bits, l49_bits, l50_bits,
    l51_bits, l52_bits, l53_bits, l54_bits, l55_bits, l56_bits, l57_bits, l58_bits, l59_bits, l60_bits,
    l61_bits, l62_bits, l63_bits, l64_bits, l65_bits, l66_bits, l67_bits, l68_bits, l69_bits, l70_bits,
    l71_bits, l72_bits, l73_bits, l74_bits, l75_bits, l76_bits, l77_bits, l78_bits, l79_bits, l80_bits,
    l81_bits, l82_bits, l83_bits, l84_bits, l85_bits, l86_bits, l87_bits, l88_bits, l89_bits, l90_bits,
    l91_bits, l92_bits, l93_bits, l94_bits, l95_bits, l96_bits, l97_bits, l98_bits, l99_bits,
};


static  long layerRawColorTab[] = {
    wRGB(0,  0,255),        /* blue */
    wRGB(0,  0,128),        /* dk blue */
    wRGB(0,128,  0),        /* dk green */
    wRGB(255,255,  0),      /* yellow */
    wRGB(0,255,  0),        /* green */
    wRGB(0,255,255),        /* lt cyan */
    wRGB(128,  0,  0),      /* brown */
    wRGB(128,  0,128),      /* purple */
    wRGB(128,128,  0),      /* green-brown */
    wRGB(255,  0,255)
};     /* lt-purple */
static  wDrawColor layerColorTab[COUNT(layerRawColorTab)];


static wWin_p layerW;
static char layerName[STR_SHORT_SIZE];
static char layerLinkList[STR_LONG_SIZE];
static char settingsName[STR_SHORT_SIZE];
static wDrawColor layerColor;
static long layerUseColor = TRUE;
static long layerVisible = TRUE;
static long layerFrozen = FALSE;
static long layerOnMap = TRUE;
static long layerModule = FALSE;
static long layerNoButton = FALSE;
static void LayerOk(void *);
static BOOL_T layerRedrawMap = FALSE;

#define ENUMLAYER_RELOAD (1)
#define ENUMLAYER_SAVE	(2)
#define ENUMLAYER_CLEAR (3)

static char *visibleLabels[] = { "", NULL };
static char *frozenLabels[] = { "", NULL };
static char *onMapLabels[] = { "", NULL };
static char *moduleLabels[] = { "", NULL };
static char *layerColorLabels[] = { "", NULL };
static paramIntegerRange_t i0_20 = { 0, NUM_BUTTONS };
static paramListData_t layerUiListData = { 10, 370, 0 };

static paramData_t layerPLs[] = {
#define I_LIST	(0)
    { PD_DROPLIST, NULL, "layer", PDO_LISTINDEX, (void*)250, N_("Select Layer:") },
#define I_NAME	(1)
    { PD_STRING, layerName, "name", PDO_NOPREF|PDO_STRINGLIMITLENGTH, (void*)(250-54), N_("Name"), 0, 0, sizeof(layerName) },
#define I_COLOR	(2)
    { PD_COLORLIST, &layerColor, "color", PDO_NOPREF, NULL, N_("Color") },
#define I_USE_COLOR (3)
	{ PD_TOGGLE, &layerUseColor, "layercolor", PDO_NOPREF|PDO_DLGHORZ, layerColorLabels, N_("Use Color"), BC_HORZ|BC_NOBORDER },
#define I_VIS	(4)
    { PD_TOGGLE, &layerVisible, "visible", PDO_NOPREF, visibleLabels, N_("Visible"), BC_HORZ|BC_NOBORDER },
#define I_FRZ	(5)
    { PD_TOGGLE, &layerFrozen, "frozen", PDO_NOPREF|PDO_DLGHORZ, frozenLabels, N_("Frozen"), BC_HORZ|BC_NOBORDER },
#define I_MAP	(6)
    { PD_TOGGLE, &layerOnMap, "onmap", PDO_NOPREF|PDO_DLGHORZ, onMapLabels, N_("On Map"), BC_HORZ|BC_NOBORDER },
#define I_MOD 	(7)
	{ PD_TOGGLE, &layerModule, "module", PDO_NOPREF|PDO_DLGHORZ, moduleLabels, N_("Module"), BC_HORZ|BC_NOBORDER },
#define I_BUT   (8)
	{ PD_TOGGLE, &layerNoButton, "button", PDO_NOPREF|PDO_DLGHORZ, moduleLabels, N_("No Button"), BC_HORZ|BC_NOBORDER },
#define I_LINKLIST (9)
	{ PD_STRING, layerLinkList, "layerlist", PDO_NOPREF|PDO_STRINGLIMITLENGTH, (void*)(250-54), N_("Linked Layers"), 0, 0, sizeof(layerLinkList) },
#define I_SETTINGS (10)
	{ PD_DROPLIST, NULL, "settings", PDO_LISTINDEX, (void*) 250, N_("Settings when Current") },
#define I_COUNT (11)
    { PD_MESSAGE, N_("Object Count:"), NULL, PDO_DLGBOXEND|PDO_DLGNOLABELALIGN, (void *)370 },
    { PD_MESSAGE, N_("All Layer Preferences"), NULL, PDO_DLGRESETMARGIN, (void *)180 },
    { PD_BUTTON, (void*)DoLayerOp, "load", PDO_DLGRESETMARGIN, 0, N_("Load"), 0, (void *)ENUMLAYER_RELOAD },
    { PD_BUTTON, (void*)DoLayerOp, "save", PDO_DLGHORZ, 0, N_("Save"), 0, (void *)ENUMLAYER_SAVE },
    { PD_BUTTON, (void*)DoLayerOp, "clear", PDO_DLGHORZ | PDO_DLGBOXEND, 0, N_("Defaults"), 0, (void *)ENUMLAYER_CLEAR },
    { PD_LONG, &newLayerCount, "button-count", PDO_DLGBOXEND|PDO_DLGRESETMARGIN, &i0_20, N_("Number of Layer Buttons") },
};

#define settingsListL	((wList_p)layerPLs[I_SETTINGS].control)
#define MESSAGETEXT ((wMessage_p)layerPLs[I_COUNT].control)

static paramGroup_t layerPG = { "layer", 0, layerPLs, sizeof layerPLs/sizeof layerPLs[0] };

/**
 * Reload the listbox showing the current catalog
 */

static
int LoadFileListLoad(Catalog *catalog, char * name)
{
    CatalogEntry *currentEntry = catalog->head;
    DynString description;
    DynStringMalloc(&description, STR_SHORT_SIZE);

    wControlShow((wControl_p)settingsListL, FALSE);
    wListClear(settingsListL);

    int currset = 0;

    int i = 0;

    wListAddValue(settingsListL," ",NULL," ");

    while (currentEntry) {
    	i++;
		DynStringClear(&description);
		DynStringCatCStr(&description,
						 currentEntry->contents) ;
		wListAddValue(settingsListL,
					  DynStringToCStr(&description),
					  NULL,
					  (void*)currentEntry->fullFileName[0]);
		if (strcmp(currentEntry->fullFileName[0],name)==0) currset = i;
        currentEntry = currentEntry->next;
    }


    wListSetIndex(settingsListL,currset);

    wControlShow((wControl_p)settingsListL, TRUE);

    DynStringFree(&description);

    if (currset == 0 && strcmp(" ",name)!=0) return FALSE;
    return TRUE;

}

#define layerL	((wList_p)layerPLs[I_LIST].control)

#define layerS  ((wList_p)layerPLs[I_SETTINGS].control)

void GetLayerLinkString(int inx,char * list) {

	char * cp = &list[0];
	cp[0] = '\0';
	int len = 0;
	for (int i = 0; i<layers[inx].layerLinkList.cnt && len<STR_LONG_SIZE-5; i++) {
		int l = DYNARR_N(int,layers[inx].layerLinkList,i);
		if (i==0)
			cp += sprintf(cp,"%d",l);
		else
			cp += sprintf(cp,";%d",l);
		cp[0] = '\0';
	}
}

void PutLayerListArray(int inx, char * list) {
	char * cp = &list[0];
	DYNARR_RESET(int, layers[inx].layerLinkList);
	while (cp) {
		cp = strpbrk(list,",; ");
		if (cp) {
			cp[0] ='\0';
			int i =  abs((int)strtol(list,&list,0));
			if (i>0 && i !=inx-1 && i<NUM_LAYERS) {
				DYNARR_APPEND(int,layers[inx].layerLinkList,1);
				DYNARR_LAST(int, layers[inx].layerLinkList) = i;
			}
			cp[0] = ';';
			list = cp+1;
		} else {
			int i =  abs((int)strtol(list,&list,0));
			if (i>0 && i !=inx-1 && i<NUM_LAYERS) {
				DYNARR_APPEND(int,layers[inx].layerLinkList,1);
				DYNARR_LAST(int,layers[inx].layerLinkList) = i;
			}
			cp = 0;
		}
	}
}


/**
 * Load the layer settings to hard coded system defaults
 */

void
LayerSystemDefaults(void)
{
    int inx;

    for (inx=0; inx<NUM_LAYERS; inx++) {
        strcpy(layers[inx].name, inx==0?_("Main"):"");
        layers[inx].visible = TRUE;
        layers[inx].frozen = FALSE;
        layers[inx].onMap = TRUE;
        layers[inx].module = FALSE;
        layers[inx].button_off = FALSE;
        layers[inx].objCount = 0;
        DYNARR_RESET(int,layers[inx].layerLinkList);
        SetLayerColor(inx, layerColorTab[inx%COUNT(layerColorTab)]);
    }
}

/**
 * Load the layer listboxes in Manage Layers and the Toolbar with up-to-date information.
 */

void LoadLayerLists(void)
{
    int inx;
    /* clear both lists */
    wListClear(setLayerL);

    if (layerL) {
        wListClear(layerL);
    }

    if (layerS) {
    	wListClear(layerS);
    }


    /* add all layers to both lists */
    for (inx=0; inx<NUM_LAYERS; inx++) {
        char *layerLabel;
        layerLabel = FormatLayerName(inx);

        if (layerL) {
            wListAddValue(layerL, layerLabel, NULL, NULL);
        }

        wListAddValue(setLayerL, layerLabel, NULL, NULL);
        free(layerLabel);
    }

    /* set current layer to selected */
    wListSetIndex(setLayerL, curLayer);

    if (layerL) wListSetIndex(layerL,curLayer);

}

/**
 * Handle button presses for the layer dialog. For all button presses in the layer
 *	dialog, this function is called. The parameter identifies the button pressed and
 * the operation is performed.
 *
 * \param[IN] data identifier for the button prerssed
 * \return
 */

static void DoLayerOp(void * data)
{
    switch ((long)data) {
    case ENUMLAYER_CLEAR:
        InitializeLayers(LayerSystemDefaults, -1);
        break;

    case ENUMLAYER_SAVE:
        LayerPrefSave();
        break;

    case ENUMLAYER_RELOAD:
        LayerPrefLoad();
        break;
    }

    UpdateLayerDlg();

    if (layoutLayerChanged) {
        MainProc(mainW, wResize_e, NULL, NULL);
        layoutLayerChanged = FALSE;
        changed++;
        SetWindowTitle();
    }
}

/**
 * Update all dialogs and dialog elements after changing layers preferences. Once the global array containing
 * the settings for the labels has been changed, this function needs to be called to update all the user interface
 * elements to the new settings.
 */

static void
UpdateLayerDlg()
{
    int inx;
    /* update the globals for the layer dialog */
    layerVisible = layers[curLayer].visible;
    layerFrozen = layers[curLayer].frozen;
    layerOnMap = layers[curLayer].onMap;
    layerModule = layers[curLayer].module;
    layerColor = layers[curLayer].color;
    layerUseColor = layers[curLayer].useColor;
    layerNoButton = layers[curLayer].button_off;
    strcpy(layerName, layers[curLayer].name);
    strcpy(settingsName, layers[curLayer].settingsName);
    GetLayerLinkString(curLayer,layerLinkList);

    layerCurrent = curLayer;
    /* now re-load the layer list boxes */
    LoadLayerLists();


    /* force update of the 'manage layers' dialogbox */
    if (layerL) {
        ParamLoadControls(&layerPG);
    }

    if (layerS) {
    	if (!LoadFileListLoad(settingsCatalog,settingsName))
    		layers[curLayer].settingsName[0] = '\0';
    }

    sprintf(message, "Object Count: %ld", layers[curLayer].objCount);
    if (MESSAGETEXT) wMessageSetValue(MESSAGETEXT, message);

    /* finally show the layer buttons with balloon text */
    for (inx = 0; inx < NUM_BUTTONS; inx++) {
    	if (!layers[inx].button_off) {
    		wButtonSetBusy(layer_btns[inx], layers[inx].visible != 0);
    		wControlSetBalloonText((wControl_p)layer_btns[inx],
                               (layers[inx].name[0] != '\0' ? layers[inx].name :_("Show/Hide Layer")));
    	}
    }

}

/**
 * Fill a layer dropbox with the current layer settings
 * 
 * \param listLayers the dropbox
 * \return 
 */
  
void
FillLayerList( wList_p listLayers)
{
	wListClear(listLayers);  // Rebuild list on each invovation

	for (int inx = 0; inx < NUM_LAYERS; inx++) {
		char *layerFormattedName;
		layerFormattedName = FormatLayerName(inx);
		wListAddValue((wList_p)listLayers, layerFormattedName, NULL, (void*)(long)inx);
		free(layerFormattedName);
	}

	/* set current layer to selected */
	wListSetIndex(listLayers, curLayer);
}
/**
 * Initialize the layer lists.
 *
 * \param IN pointer to function that actually initialize tha data structures
 * \param IN current layer (0...NUM_LAYERS), (-1) for no change
 */

static void
InitializeLayers(void LayerInitFunc(void), int newCurrLayer)
{
    /* reset the data structures to default valuses */
    LayerInitFunc();
    /* count the objects on each layer */
    LayerSetCounts();

    /* Switch the current layer when requested */
    if (newCurrLayer != -1) {
        curLayer = newCurrLayer;
    }
}

/**
 * Save the customized layer information to preferences.
 */

static void
LayerPrefSave(void)
{
    unsigned int inx;
    int flags;
    char buffer[ 80 ];
    char links[STR_LONG_SIZE];
    char layersSaved[ 3 * NUM_LAYERS + 1 ];			/* 0..99 plus separator */
    /* FIXME: values for layers that are configured to default now should be overwritten in the settings */
    layersSaved[ 0 ] = '\0';

    for (inx = 0; inx < NUM_LAYERS; inx++) {
        /* if a name is set that is not the default value or a color different from the default has been set,
            information about the layer needs to be saved */
        if ((layers[inx].name[0]) ||
                layers[inx].frozen || (!layers[inx].onMap) || (!layers[inx].visible) ||
				layers[inx].button_off || (layers[inx].layerLinkList.cnt>0) ||
				layers[inx].module ||
                layers[inx].color != layerColorTab[inx%COUNT(layerColorTab)]) {
            sprintf(buffer, LAYERPREF_NAME ".%0u", inx);
            wPrefSetString(LAYERPREF_SECTION, buffer, layers[inx].name);
            sprintf(buffer, LAYERPREF_COLOR ".%0u", inx);
            wPrefSetInteger(LAYERPREF_SECTION, buffer, wDrawGetRGB(layers[inx].color));
            flags = 0;

            if (layers[inx].frozen) {
                flags |= LAYERPREF_FROZEN;
            }

            if (layers[inx].onMap) {
                flags |= LAYERPREF_ONMAP;
            }

            if (layers[inx].visible) {
                flags |= LAYERPREF_VISIBLE;
            }

            if (layers[inx].module) {
            	flags |= LAYERPREF_MODULE;
            }

            if (layers[inx].button_off) {
            	flags |= LAYERPREF_NOBUTTON;
            }

            sprintf(buffer, LAYERPREF_FLAGS ".%0u", inx);
            wPrefSetInteger(LAYERPREF_SECTION, buffer, flags);

            if (layers[inx].layerLinkList.cnt>0) {
            	sprintf(buffer, LAYERPREF_LIST ".%0u", inx);
            	GetLayerLinkString(inx,links);
            	wPrefSetString(LAYERPREF_SECTION, buffer, links);

            	if (settingsName[0] && strcmp(settingsName," ")!=0) {
            		sprintf(buffer, LAYERPREF_SETTINGS ".%0u", inx);
            		wPrefSetString(LAYERPREF_SECTION, buffer, layers[inx].settingsName);
            	}
            }

            /* extend the list of layers that are set up via the preferences */
            if (layersSaved[ 0 ]) {
                strcat(layersSaved, ",");
            }

            sprintf(buffer, "%u", inx);
            strcat(layersSaved, buffer);
        }
    }

    wPrefSetString(LAYERPREF_SECTION, "layers", layersSaved);
}


/**
 * Load the settings for all layers from the preferences.
 */

static void
LayerPrefLoad(void)
{
    const char *prefString;
    long rgb;
    long flags;
    /* reset layer preferences to system default */
    LayerSystemDefaults();
    prefString = wPrefGetString(LAYERPREF_SECTION, "layers");

    if (prefString && prefString[ 0 ]) {
        char layersSaved[3 * NUM_LAYERS];
        strncpy(layersSaved, prefString, sizeof(layersSaved));
        prefString = strtok(layersSaved, ",");

        while (prefString) {
            int inx;
            char layerOption[20];
            const char *layerValue;
            char listValue[STR_LONG_SIZE];
            int color;
            inx = atoi(prefString);
            sprintf(layerOption, LAYERPREF_NAME ".%d", inx);
            layerValue = wPrefGetString(LAYERPREF_SECTION, layerOption);

            if (layerValue) {
                strcpy(layers[inx].name, layerValue);
            } else {
                *(layers[inx].name) = '\0';
            }

            /* get and set the color, using the system default color in case color is not available from prefs */
            sprintf(layerOption, LAYERPREF_COLOR ".%d", inx);
            wPrefGetInteger(LAYERPREF_SECTION, layerOption, &rgb,
                            layerColorTab[inx%COUNT(layerColorTab)]);
            color = wDrawFindColor(rgb);
            SetLayerColor(inx, color);
            /* get and set the flags */
            sprintf(layerOption, LAYERPREF_FLAGS ".%d", inx);
            wPrefGetInteger(LAYERPREF_SECTION, layerOption, &flags,
                            LAYERPREF_ONMAP | LAYERPREF_VISIBLE);
            layers[inx].frozen = ((flags & LAYERPREF_FROZEN) != 0);
            layers[inx].onMap = ((flags & LAYERPREF_ONMAP) != 0);
            layers[inx].visible = ((flags & LAYERPREF_VISIBLE) != 0);
            layers[inx].module = ((flags & LAYERPREF_MODULE) !=0);
            layers[inx].button_off = ((flags & LAYERPREF_NOBUTTON) !=0);

            sprintf(layerOption, LAYERPREF_LIST ".%d", inx);
            layerValue = wPrefGetString(LAYERPREF_SECTION,layerOption);
            if (layerValue) {
            	strcpy(listValue,layerValue);
            	PutLayerListArray(inx,listValue);
            } else {
            	listValue[0] = '\0';
            	PutLayerListArray(inx,listValue);
            }
            sprintf(layerOption, LAYERPREF_SETTINGS ".%d", inx);
            layerValue = wPrefGetString(LAYERPREF_SECTION,layerOption);
            if (layerValue) {
            	strcpy(layers[inx].settingsName,layerValue);
            } else {
            	layers[inx].settingsName[0] = '\0';
            }

            prefString = strtok(NULL, ",");
        }
    }
}

/**
 * Increment the count of objects on a given layer.
 *
 * \param index IN the layer to change
 */

void IncrementLayerObjects(unsigned int layer)
{
    assert(layer <= NUM_LAYERS);
    layers[layer].objCount++;
}

/**
* Decrement the count of objects on a given layer.
*
* \param index IN the layer to change
*/

void DecrementLayerObjects(unsigned int layer)
{
    assert(layer <= NUM_LAYERS);
    layers[layer].objCount--;
}

/**
 *	Count the number of objects on each layer and store result in layers data structure.
 */

void LayerSetCounts(void)
{
    int inx;
    track_p trk;

    for (inx=0; inx<NUM_LAYERS; inx++) {
        layers[inx].objCount = 0;
    }

    for (trk=NULL; TrackIterate(&trk);) {
        inx = GetTrkLayer(trk);

        if (inx >= 0 && inx < NUM_LAYERS) {
            layers[inx].objCount++;
        }
    }
}

int FindUnusedLayer(unsigned int start) {
	int inx;
	for (inx=start; inx<NUM_LAYERS; inx++) {
	    if (layers[inx].objCount == 0 && !layers[inx].frozen) return inx;
	}
	ErrorMessage( MSG_NO_EMPTY_LAYER );
	return -1;
}

/**
 * Reset layer options to their default values. The default values are loaded
 * from the preferences file.
 */

void
DefaultLayerProperties(void)
{
    InitializeLayers(LayerPrefLoad, 0);
    UpdateLayerDlg();

    if (layoutLayerChanged) {
        MainProc(mainW, wResize_e, NULL, NULL);
        layoutLayerChanged = FALSE;
    }
}

/**
 * Update all UI elements after selecting a layer.
 *
 */

static void LayerUpdate(void)
{
    BOOL_T redraw;
    char *layerFormattedName;
    ParamLoadData(&layerPG);

    if (!IsLayerValid(layerCurrent)) {
        return;
    }

    if (layerCurrent == curLayer && layerFrozen) {
        NoticeMessage(MSG_LAYER_FREEZE, _("Ok"), NULL);
        layerFrozen = FALSE;
        ParamLoadControl(&layerPG, I_FRZ);
    }

    if (layerCurrent == curLayer && !layerVisible) {
        NoticeMessage(MSG_LAYER_HIDE, _("Ok"), NULL);
        layerVisible = TRUE;
        ParamLoadControl(&layerPG, I_VIS);
    }

    if (layerCurrent == curLayer && layerModule) {
            NoticeMessage(MSG_LAYER_MODULE, _("Ok"), NULL);
            layerModule = FALSE;
            ParamLoadControl(&layerPG, I_MOD);
    }
    char oldLinkList[STR_LONG_SIZE];
    GetLayerLinkString((int)layerCurrent,oldLinkList);

    if (strcmp(layers[(int)layerCurrent].name, layerName) ||
            layerColor != layers[(int)layerCurrent].color ||
			layers[(int)layerCurrent].useColor != (BOOL_T)layerUseColor ||
            layers[(int)layerCurrent].visible != (BOOL_T)layerVisible ||
            layers[(int)layerCurrent].frozen != (BOOL_T)layerFrozen ||
            layers[(int)layerCurrent].onMap != (BOOL_T)layerOnMap ||
			layers[(int)layerCurrent].module != (BOOL_T)layerModule ||
			layers[(int)layerCurrent].button_off != (BOOL_T)layerNoButton ||
			strcmp(layers[(int)layerCurrent].settingsName,settingsName) ||
			strcmp(oldLinkList,layerLinkList)) {
        changed++;
        SetWindowTitle();
    }

    if (layerL) {
        strncpy(layers[(int)layerCurrent].name, layerName,
                sizeof layers[(int)layerCurrent].name);
        layerFormattedName = FormatLayerName(layerCurrent);
        wListSetValues(layerL, layerCurrent, layerFormattedName, NULL, NULL);
        free(layerFormattedName);
    }


    layerFormattedName = FormatLayerName(layerCurrent);
    wListSetValues(setLayerL, layerCurrent, layerFormattedName, NULL, NULL);
    free(layerFormattedName);

    if (layerCurrent < NUM_BUTTONS && !layers[(int)layerCurrent].button_off) {
        if (strlen(layers[(int)layerCurrent].name)>0) {
            wControlSetBalloonText((wControl_p)layer_btns[(int)layerCurrent],
                                   layers[(int)layerCurrent].name);
        } else {
            wControlSetBalloonText((wControl_p)layer_btns[(int)layerCurrent],
                                   _("Show/Hide Layer"));
        }
    }

    redraw = (layerColor != layers[(int)layerCurrent].color ||
    		layers[(int)layerCurrent].useColor != (BOOL_T)layerUseColor ||
              (BOOL_T)layerVisible != layers[(int)layerCurrent].visible);

    if ((!layerRedrawMap) && redraw) {
        RedrawLayer((unsigned int)layerCurrent, FALSE);
    }

    SetLayerColor(layerCurrent, layerColor);

    if (layerCurrent<NUM_BUTTONS &&
            layers[(int)layerCurrent].visible!=(BOOL_T)layerVisible && !layers[(int)layerCurrent].button_off) {
        wButtonSetBusy(layer_btns[(int)layerCurrent], layerVisible);
    }

    layers[(int)layerCurrent].useColor = (BOOL_T)layerUseColor;
    if (layers[(int)layerCurrent].visible != (BOOL_T)layerVisible)
			FlipLayer(layerCurrent);
    layers[(int)layerCurrent].visible = (BOOL_T)layerVisible;
    layers[(int)layerCurrent].frozen = (BOOL_T)layerFrozen;
    layers[(int)layerCurrent].onMap = (BOOL_T)layerOnMap;
    layers[(int)layerCurrent].module = (BOOL_T)layerModule;
    strcpy(layers[(int)layerCurrent].settingsName,settingsName);

    PutLayerListArray((int)layerCurrent,layerLinkList);

    SetLayerHideButton(layerCurrent,layerNoButton);

    MainProc( mainW, wResize_e, NULL, NULL );

    if (layerRedrawMap) {
        DoRedraw();
    } else if (redraw) {
        RedrawLayer((unsigned int)layerCurrent, TRUE);
    }

    layerRedrawMap = FALSE;
}


static void LayerSelect(
    wIndex_t inx)
{
    LayerUpdate();

    if (inx < 0 || inx >= NUM_LAYERS) {
        return;
    }

    layerCurrent = (unsigned int)inx;
    strcpy(layerName, layers[inx].name);
    strcpy(settingsName, layers[inx].settingsName);
    layerVisible = layers[inx].visible;
    layerFrozen = layers[inx].frozen;
    layerOnMap = layers[inx].onMap;
    layerModule = layers[inx].module;
    layerColor = layers[inx].color;
    layerUseColor = layers[inx].useColor;
    layerNoButton = layers[inx].button_off;
    sprintf(message, "%ld", layers[inx].objCount);
    GetLayerLinkString(inx,layerLinkList);
    ParamLoadMessage(&layerPG, I_COUNT, message);
    ParamLoadControls(&layerPG);

    if (layerS) {
    	if (!LoadFileListLoad(settingsCatalog,settingsName)) {
    		settingsName[0] = '\0';
    		layers[inx].settingsName[0] = '\0';
    	}

    }
}

void ResetLayers(void)
{
    int inx;

    for (inx=0; inx<NUM_LAYERS; inx++) {
        strcpy(layers[inx].name, inx==0?_("Main"):"");
        layers[inx].visible = TRUE;
        layers[inx].frozen = FALSE;
        layers[inx].onMap = TRUE;
        layers[inx].module = FALSE;
        layers[inx].button_off = FALSE;
        layers[inx].objCount = 0;
        strcpy(layers[inx].settingsName,"");
        DYNARR_RESET(int,layers[inx].layerLinkList);
        SetLayerColor(inx, layerColorTab[inx%COUNT(layerColorTab)]);


        if (inx<NUM_BUTTONS) {
            wButtonSetLabel(layer_btns[inx], (char*)show_layer_bmps[inx]);
        }
    }

    wControlSetBalloonText((wControl_p)layer_btns[0], _("Main"));

    for (inx=1; inx<NUM_BUTTONS; inx++) {
        wControlSetBalloonText((wControl_p)layer_btns[inx], _("Show/Hide Layer"));
    }

    curLayer = 0;
    layerVisible = TRUE;
    layerFrozen = FALSE;
    layerOnMap = TRUE;
    layerModule = FALSE;
    layerColor = layers[0].color;
    layerUseColor = TRUE;
    strcpy(layerName, layers[0].name);
    strcpy(settingsName, layers[0].settingsName);

    LoadLayerLists();

    if (layerL) {
        ParamLoadControls(&layerPG);
        ParamLoadMessage(&layerPG, I_COUNT, "0");
    }
}


void SaveLayers(void)
{
    layers_save = malloc(NUM_LAYERS * sizeof(layers[0]));

    if (layers_save == NULL) {
        abort();
    }

    for (int i=0;i<NUM_LAYERS;i++) {
    	layers[i].settingsName[0] = '\0';
    }

    memcpy(layers_save, layers, NUM_LAYERS * sizeof layers[0]);
    ResetLayers();
}

void RestoreLayers(void)
{
    int inx;
    char * label;
    wDrawColor color;
    assert(layers_save != NULL);
    memcpy(layers, layers_save, NUM_LAYERS * sizeof layers[0]);
    free(layers_save);

    for (inx=0; inx<NUM_BUTTONS; inx++) {
        color = layers[inx].color;
        layers[inx].color = -1;
        SetLayerColor(inx, color);

        if (layers[inx].name[0] == '\0') {
            if (inx == 0) {
                label = _("Main");
            } else {
                label = _("Show/Hide Layer");
            }
        } else {
            label = layers[inx].name;
        }

        wControlSetBalloonText((wControl_p)layer_btns[inx], label);
    }

    if (layerL) {
        ParamLoadControls(&layerPG);
        ParamLoadMessage(&layerPG, I_COUNT, "0");
    }

    LoadLayerLists();
}

/**
 * This function is called when the Done button on the layer dialog is pressed. It hides the layer dialog and
 * updates the layer information.
 *
 * \param IN ignored
 *
 */

static void LayerOk(void * junk)
{
    LayerSelect(layerCurrent);

    if (newLayerCount != layerCount) {
        layoutLayerChanged = TRUE;

        if (newLayerCount > NUM_BUTTONS) {
            newLayerCount = NUM_BUTTONS;
        }

        layerCount = newLayerCount;
    }

    if (layoutLayerChanged) {
        MainProc(mainW, wResize_e, NULL, NULL);
    }

    wHide(layerW);
}


static void LayerDlgUpdate(
    paramGroup_p pg,
    int inx,
    void * valueP)
{
    switch (inx) {
    case I_LIST:
        LayerSelect((wIndex_t)*(long*)valueP);
        break;

    case I_NAME:
        LayerUpdate();
        break;

    case I_MAP:
        layerRedrawMap = TRUE;
        break;

    case I_SETTINGS:
    	if (strcmp((char*)wListGetItemContext(settingsListL,(wIndex_t)*(long*)valueP)," ")==0)
    		settingsName[0] = '\0';
    	else
    		strcpy(settingsName,(char*)wListGetItemContext(settingsListL,(wIndex_t)*(long*)valueP));
    	break;
    }
}

/**
 * Scan opened directory for the next settings file
 *
 * \param dir IN opened directory handle
 * \param dirName IN name of directory
 * \param fileName OUT fully qualified filename
 *
 * \return TRUE if file found, FALSE if not
 */

static bool
GetNextSettingsFile(DIR *dir, const char *dirName, char **fileName)
{
    bool done = false;
    bool res = false;

    /*
    * get all files from the directory
    */
    while (!done) {
        struct stat fileState;
        struct dirent *ent;

        ent = readdir(dir);

        if (ent) {
            if (!XtcStricmp(FindFileExtension(ent->d_name), "xset")) {
                /* create full file name and get the state for that file */
                MakeFullpath(fileName, dirName, ent->d_name, NULL);

                if (stat(*fileName, &fileState) == -1) {
                    fprintf(stderr, "Error getting file state for %s\n", *fileName);
                    continue;
                }

                /* ignore any directories */
                if (!(fileState.st_mode & S_IFDIR)) {
                    done = true;
                    res = true;
                }
            }
        } else {
            done = true;
            res = false;
        }
    }
    return (res);
}


/*
 * Get all the settings files in the working directory
 */

static CatalogEntry *
ScanSettingsDirectory(Catalog *catalog, const char *dirName)
{
    DIR *d;
#if defined(WINDOWS)
	#define PATH_SEPARATOR '\\'
#else
	#define PATH_SEPARATOR '/'
#endif
    CatalogEntry *newEntry = catalog->head;
    char contents[STR_SHORT_SIZE];

    d = opendir(dirName);
    if (d) {
        char *fileName = NULL;

        while (GetNextSettingsFile(d, dirName, &fileName)) {
            CatalogEntry *existingEntry;
            char *contents_start = strrchr(fileName,PATH_SEPARATOR);
            if (contents_start[0] == '/') contents_start++;
            char *contents_end = strchr(contents_start,'.');
            if (contents_end[0] == '.') contents_end[0] = '\0';
            strcpy(contents,contents_start);
            contents_end[0] = '.';
			newEntry = InsertInOrder(catalog,contents);
            UpdateCatalogEntry(newEntry, fileName, contents);
            free(fileName);
            fileName = NULL;
        }
        closedir(d);
    }

    return (newEntry);
}

static void DoLayer(void * junk)
{
    if (layerW == NULL) {
        layerW = ParamCreateDialog(&layerPG, MakeWindowTitle(_("Layers")), _("Done"),
                                   LayerOk, wHide, TRUE, NULL, 0, LayerDlgUpdate);
    }

    if (settingsCatalog) CatalogDiscard(settingsCatalog);
    else settingsCatalog = InitCatalog();
    ScanSettingsDirectory(settingsCatalog, wGetAppWorkDir());


    /* set the globals to the values for the current layer */
    UpdateLayerDlg();
    layerRedrawMap = FALSE;
    wShow(layerW);
    layoutLayerChanged = FALSE;
}



BOOL_T ReadLayers(char * line)
{
    char * name, *layerLinkList, *layerSettingsName;
    int inx, visible, frozen, color, onMap, module, dontUseColor, ColorFlags, button_off;
    unsigned long rgb;

    /* older files didn't support layers */

    if (paramVersion < 7) {
        return TRUE;
    }

    /* set the current layer */

    if (strncmp(line, "CURRENT", 7) == 0) {
        curLayer = atoi(line+7);

        if (!IsLayerValid(curLayer)) {
            curLayer = 0;
        }

        if (layerL) {
            wListSetIndex(layerL, curLayer);
        }

        if (setLayerL) {
            wListSetIndex(setLayerL, curLayer);
        }

        return TRUE;
    }

    if (strncmp(line, "LINK", 4) == 0) {
    	if (!GetArgs(line+4, "dq" , &inx, &layerLinkList)) {
    		return FALSE;
    	}
    	PutLayerListArray(inx,layerLinkList);
    	return TRUE;
    }

    if (strncmp(line, "SET", 3) == 0) {
    	if (!GetArgs(line+3, "dq", &inx, &layerSettingsName)) {
    		return FALSE;
    	}
    	strcpy(layers[inx].settingsName,layerSettingsName);
    	return TRUE;
    }

    /* get the properties for a layer from the file and update the layer accordingly */

	if (!GetArgs(line, "dddduddddq", &inx, &visible, &frozen, &onMap, &rgb, &module, &dontUseColor, &ColorFlags, &button_off,
			 &name)) {

		return FALSE;
	}


    if (paramVersion < 9) {
        if ((int)rgb < sizeof oldColorMap/sizeof oldColorMap[0]) {
            rgb = wRGB(oldColorMap[(int)rgb][0], oldColorMap[(int)rgb][1],
                       oldColorMap[(int)rgb][2]);
        } else {
            rgb = 0;
        }
    }

    if (inx < 0 || inx >= NUM_LAYERS) {
        return FALSE;
    }

    color = wDrawFindColor(rgb);
    SetLayerColor(inx, color);
    strncpy(layers[inx].name, name, sizeof layers[inx].name);
    layers[inx].visible = visible;
    layers[inx].frozen = frozen;
    layers[inx].onMap = onMap;
    layers[inx].module = module;
    layers[inx].color = color;
    layers[inx].useColor = !dontUseColor;
    layers[inx].button_off = button_off;

    colorTrack = ColorFlags&1;  //Make sure globals are set
    colorDraw = ColorFlags&2;

    if (inx<NUM_BUTTONS && !layers[inx].button_off) {
        if (strlen(name) > 0) {
            wControlSetBalloonText((wControl_p)layer_btns[(int)inx], layers[inx].name);
        }
        wButtonSetBusy(layer_btns[(int)inx], visible);
    }
    MyFree(name);

    return TRUE;
}

/**
 * Find out whether layer information should be saved to the layout file.
 * Usually only layers where settings are off from the default are written.
 * NOTE: as a fix for a problem with XTrkCadReader a layer definition is
 * written for each layer that is used.
 *
 * \param layerNumber IN index of the layer
 * \return TRUE if configured, FALSE if not
 */

BOOL_T
IsLayerConfigured(unsigned int layerNumber)
{
    return (!layers[layerNumber].visible ||
            layers[layerNumber].frozen ||
            !layers[layerNumber].onMap ||
			layers[layerNumber].module ||
			layers[layerNumber].button_off ||
            layers[layerNumber].color !=
            layerColorTab[layerNumber % (COUNT(layerColorTab))] ||
            layers[layerNumber].name[0] ||
			layers[layerNumber].layerLinkList.cnt > 0 ||
            layers[layerNumber].objCount);
}


/**
 * Save the layer information to the file.
 *
 * \paran f IN open file handle
 * \return always TRUE
 */

BOOL_T WriteLayers(FILE * f)
{
    unsigned int inx;

    int ColorFlags = 0;

    if (colorTrack) ColorFlags |= 1;
    if (colorDraw) ColorFlags |= 2;

    for (inx = 0; inx < NUM_LAYERS; inx++) {
        if (IsLayerConfigured(inx)) {
            fprintf(f, "LAYERS %u %d %d %d %ld %d %d %d %d \"%s\"\n",
                    inx,
                    layers[inx].visible,
                    layers[inx].frozen,
                    layers[inx].onMap,
                    wDrawGetRGB(layers[inx].color),
                    layers[inx].module,
					layers[inx].useColor?0:1,
					ColorFlags, layers[inx].button_off,
                    PutTitle(layers[inx].name));
        }
    }

    fprintf(f, "LAYERS CURRENT %u\n", curLayer);

    for (inx = 0; inx < NUM_LAYERS; inx++) {
    	GetLayerLinkString(inx,layerLinkList);
    	if (IsLayerConfigured(inx) && strlen(layerLinkList)>0)
    		fprintf(f, "LAYERS LINK %u \"%s\"\n",inx,layerLinkList);
    	if (IsLayerConfigured(inx) && layers[inx].settingsName[0])
    		fprintf(f, "LAYERS SET %u \"%s\"\n",inx, layers[inx].settingsName);
    }
    return TRUE;
}

#include "bitmaps/background.xpm"

void InitLayers(void)
{
    unsigned int i;
    wPrefGetInteger(PREFSECT, "layer-button-count", &layerCount, layerCount);

    for (i = 0; i<COUNT(layerRawColorTab); i++) {
        layerColorTab[i] = wDrawFindColor(layerRawColorTab[i]);
    }

    /* create the bitmaps for the layer buttons */
    /* all bitmaps have to have the same dimensions */
    for (int i = 0;i<NUM_LAYERS; i++) {
        show_layer_bmps[i] = wIconCreateBitMap(l1_width, l1_height, show_layer_bits[i],
                                               layerColorTab[i%(COUNT(layerColorTab))]);
        layers[i].color = layerColorTab[i%(COUNT(layerColorTab))];
        layers[i].useColor = TRUE;
    }

    /* layer list for toolbar */
    setLayerL = wDropListCreate(mainW, 0, 0, "cmdLayerSet", NULL, 0, 10, 200, NULL,
                                SetCurrLayer, NULL);
    wControlSetBalloonText((wControl_p)setLayerL, GetBalloonHelpStr("cmdLayerSet"));
    AddToolbarControl((wControl_p)setLayerL, IC_MODETRAIN_TOO);
	
	backgroundB = AddToolbarButton("cmdBackgroundShow", wIconCreatePixMap(background), 0,
		(addButtonCallBack_t)BackgroundToggleShow, NULL);
	wControlActive((wControl_p)backgroundB, FALSE);

    for (int i = 0; i<NUM_LAYERS; i++) {
        char *layerName;

        if (i<NUM_BUTTONS) {
            /* create the layer button */
            sprintf(message, "cmdLayerShow%u", i);
            layer_btns[i] = wButtonCreate(mainW, 0, 0, message,
                                          (char*)(show_layer_bmps[i]),
                                          BO_ICON, 0, (wButtonCallBack_p)FlipLayer, (void*)(intptr_t)i);
            /* add the help text */
            wControlSetBalloonText((wControl_p)layer_btns[i], _("Show/Hide Layer"));
            /* put on toolbar */
			AddToolbarControl((wControl_p)layer_btns[i], IC_MODETRAIN_TOO);
			/* set state of button */
			wButtonSetBusy(layer_btns[i], 1);
        }

        layerName = FormatLayerName(i);
        wListAddValue(setLayerL, layerName, NULL, (void*)(long)i);
        free(layerName);
    }

    AddPlaybackProc("SETCURRLAYER", PlaybackCurrLayer, NULL);
    AddPlaybackProc("LAYERS", (playbackProc_p)ReadLayers, NULL);
}


addButtonCallBack_t InitLayersDialog(void)
{
    ParamRegister(&layerPG);
    return &DoLayer;
}
