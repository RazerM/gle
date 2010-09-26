/************************************************************************
 *                                                                      *
 * GLE - Graphics Layout Engine <http://www.gle-graphics.org/>          *
 *                                                                      *
 * Modified BSD License                                                 *
 *                                                                      *
 * Copyright (C) 2009 GLE.                                              *
 *                                                                      *
 * Redistribution and use in source and binary forms, with or without   *
 * modification, are permitted provided that the following conditions   *
 * are met:                                                             *
 *                                                                      *
 *    1. Redistributions of source code must retain the above copyright *
 * notice, this list of conditions and the following disclaimer.        *
 *                                                                      *
 *    2. Redistributions in binary form must reproduce the above        *
 * copyright notice, this list of conditions and the following          *
 * disclaimer in the documentation and/or other materials provided with *
 * the distribution.                                                    *
 *                                                                      *
 *    3. The name of the author may not be used to endorse or promote   *
 * products derived from this software without specific prior written   *
 * permission.                                                          *
 *                                                                      *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR   *
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED       *
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE   *
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY       *
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL   *
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE    *
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS        *
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER *
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR      *
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN  *
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                        *
 *                                                                      *
 ************************************************************************/

#define GRAPHDEF

#include "all.h"
#include "tokens/stokenizer.h"
#include "mem_limits.h"
#include "token.h"
#include "core.h"
#include "glearray.h"
#include "polish.h"
#include "pass.h"
#include "var.h"
#include "graph.h"
#include "begin.h"
#include "axis.h"
#include "cutils.h"
#include "gprint.h"
#include "key.h"
#include "justify.h"

#define GLEG_CMD_AXIS     1
#define GLEG_CMD_LABELS   2
#define GLEG_CMD_SIDE     3
#define GLEG_CMD_SUBTICKS 4
#define GLEG_CMD_TICKS    5

#define BAR_SET_COLOR   0
#define BAR_SET_FILL    1
#define BAR_SET_TOP     2
#define BAR_SET_SIDE    3
#define BAR_SET_PATTERN 4
#define BAR_SET_BACKGROUND 5

extern int trace_on;
static int xxgrid[GLE_AXIS_MAX+1];

extern int g_nkd, g_keycol;
extern key_struct *kd[100];

int letline[MAX_NUMBER_OF_LET_COMMANDS];
int nlet;

int g_nbar = 0;
int g_graph_background = GLE_FILL_CLEAR;

int freedataset(int i);
void free_temp(void);
void set_sizelength(void);
void draw_graph(KeyInfo* keyinfo) throw (ParserError);
void do_set_bar_color(const char* tk, bar_struct* bar, int type);
void do_set_bar_style(const char* tk, bar_struct* bar);
void do_axis_part_all(int xset) throw (ParserError);
bool do_remaining_entries(int ct);
void graph_freebars();
double get_next_exp(TOKENS tk,int ntk,int *curtok);
void data_command();

GLEAxis xx[GLE_AXIS_MAX+1];

vector<int> g_fcalls;
vector<int> g_funder;

GLEColorMap* g_colormap;

bool check_axis_command_name(const char* name, const char* cmp) {
	int type = axis_type(name);
	if (type != GLE_AXIS_ALL) {
		int len = strlen(name);
		if (len > 2 && name[1] >= '0' && name[1] <= '9') {
			return str_i_equals(name+2, cmp);
		} else if (len > 1) {
			return str_i_equals(name+1, cmp);
		}
	}
	return false;
}

void ensureDataSetCreated(int d) {
	if (dp[d] == NULL) {
		dp[d] = new GLEDataSet();
		copy_default(d);
		if (ndata < d) ndata = d;
	}
}

void ensureDataSetCreatedAndSetUsed(int d) {
	ensureDataSetCreated(d);
	dp[d]->axisscale = true;
}

// Font sizes in a graph are based in g_fontsz, but this was set in the original
// GLE to a fixed proportion of the largest dimension of a graph. The size of an
// axis is defined in terms of its base size, which is equal to g_fontsz.
// A useful value for base is 0.25

void begin_graph(int *pln , int *pcode , int *cp) throw (ParserError) {
	int ct,i,t,d;
	int st;
	int erflg;
	stringstream err;

	g_colormap = NULL;
	nlet = 0;
	g_nkd = 0;
	g_keycol = 0;

	KeyInfo keyinfo;
	g_hscale = .7;
	g_vscale = .7;
	nlet = 0;
	erflg = true;
	if (g_get_compatibility() == GLE_COMPAT_35) {
		g_nobox = false;
	} else {
		g_nobox = true;
	}
	g_center = false;
	g_auto_s_h = false;
	g_auto_s_v = false;
	g_math = false;
	for (i = 1; i <= GLE_AXIS_MAX; i++) {
		xxgrid[i] = 0;
		vinit_axis(i);
	}
	graph_init();
	g_get_usersize(&g_xsize,&g_ysize);
	g_get_hei(&g_fontsz);
	set_sizelength();
	dp[0] = new GLEDataSet();  /* dataset for default settings */
	(*pln)++;
	begin_init();
	for (;;) {
		/* process next line */
do_next_line:
		srclin[0] = 0;
		st = begin_token(&pcode,cp,pln,srclin,tk,&ntk,outbuff);
		if (!st) {
			draw_graph(&keyinfo);
			return;
		}
		ct = 1;
		if (str_i_equals(tk[ct],"BAR")) goto do_bar;
		else if (str_i_equals(tk[ct],"DATA")) goto do_data;
		else kw("DATA") goto do_data;
		else kw("EXTRACT") goto do_extract;
		else kw("FILL") goto do_fill;
		else kw("HSCALE") goto do_hscale;
		else kw("LET") 	goto do_letsave;
		else kw("SIZE") goto do_size;
		else kw("KEY") goto do_key;
		else kw("VSCALE") goto do_vscale;
		else kw("SCALE") goto do_scale;
		else kw("COLORMAP") goto do_colormap;
		else kw("TITLE") goto do_main_title;
		else kw("UNDER") g_funder.push_back((*pln)-1);
		else kw("BACKGROUND")
		{
			g_graph_background = next_color;
		}
		else kw("!") goto do_next_line;
		else kw(" ") goto do_next_line;
		else if (check_axis_command_name(tk[ct],"NOTICKS")) goto do_noticks;
		else if (str_i_str(tk[ct],"AXIS") != NULL) do_axis_part_all(GLEG_CMD_AXIS);
		else if (str_i_str(tk[ct],"LABELS") != NULL) do_axis_part_all(GLEG_CMD_LABELS);
		else if (str_i_str(tk[ct],"SIDE") != NULL) do_axis_part_all(GLEG_CMD_SIDE);
		else if (str_i_str(tk[ct],"SUBTICKS") != NULL) do_axis_part_all(GLEG_CMD_SUBTICKS);
		else if (str_i_str(tk[ct],"TICKS") != NULL) do_axis_part_all(GLEG_CMD_TICKS);
		else if (check_axis_command_name(tk[ct],"NAMES")) goto do_names;
		else if (check_axis_command_name(tk[ct],"PLACES")) goto do_places;
		else if (check_axis_command_name(tk[ct],"TITLE")) goto do_title;
		else if (toupper(*tk[ct])=='D') goto do_datasets;
		else if (!do_remaining_entries(ct)) {
			/* This might be a function call, find out later */
			g_fcalls.push_back((*pln)-1);
		}
	}

/* ----------------------------------------------------------------*/
do_bar:
	/* bar d1,d2,d3 from d4,d5,d6 color red,green,blue fill grey,blue,green
		dist exp, width exp, LSTYLE 3,445,1
		3dbar .5 .5 top black,black,black side red,green,blue  notop */
{
	int ng = 0,fi;
	char *ss;
	g_nbar++;
	br[g_nbar] = new bar_struct();
	br[g_nbar]->ngrp = 0;
	ct = 2;
	ss = strtok(tk[ct],",");
	while (ss!=NULL) {
	  if (toupper(*ss)=='D') {
		ng = (br[g_nbar]->ngrp)++;
		d = get_dataset_identifier(ss);
		ensureDataSetCreatedAndSetUsed(d);
		br[g_nbar]->to[ng] = d;
 	  }
	  ss = strtok(0,",");
	}

	br[g_nbar]->horiz = false;
	for (i = 0; i <= ng; i++) {
		br[g_nbar]->color[i] = g_get_grey(0.0);
		br[g_nbar]->fill[i] = g_get_grey(i == 0 ? 0.0 : 1.0-ng/i);
		br[g_nbar]->from[i] = 0;
		br[g_nbar]->pattern[i] = -1;
		g_get_line_width(&br[g_nbar]->lwidth[i]);
		strcpy(br[g_nbar]->lstyle[i] ,"1\0");
	}

	ct++;
	while (ct<=ntk)  {
		kw("DIST") 	br[g_nbar]->dist = next_exp;
	   else kw("WIDTH") 	br[g_nbar]->width = next_exp;
	   else kw("3D") {
		br[g_nbar]->x3d = next_exp;
		br[g_nbar]->y3d = next_exp;
	   }
	   else kw("NOTOP") 	br[g_nbar]->notop = true;
	   else kw("HORIZ") 	br[g_nbar]->horiz = true;
	   else kw("LSTYLE") 	next_str((char *) br[g_nbar]->lstyle);
	   else kw("STYLE") {
	   	ct += 1;
		do_set_bar_style(tk[ct], br[g_nbar]);
	   }
	   else kw("LWIDTH") 	br[g_nbar]->lwidth[0] = next_exp;
	   else kw("FROM") {
		fi = 0;
		ct +=1;
		ss = strtok(tk[ct],",");
		while (ss!=NULL) {
		  if (toupper(*ss)=='D') {
			int di = get_dataset_identifier(ss);
			ensureDataSetCreatedAndSetUsed(di);
			br[g_nbar]->from[fi++] = di;
		  }
		  ss = strtok(0,",");
		}
	   }
	   else kw("COLOR") {
		ct += 1;
		do_set_bar_color(tk[ct], br[g_nbar], BAR_SET_COLOR);
	   }
	   else kw("SIDE") {
		ct += 1;
		do_set_bar_color(tk[ct], br[g_nbar], BAR_SET_SIDE);
	   }
	   else kw("TOP") {
		ct += 1;
		do_set_bar_color(tk[ct], br[g_nbar], BAR_SET_TOP);
	   }
	   else kw("FILL") {
		ct++;
		do_set_bar_color(tk[ct], br[g_nbar], BAR_SET_FILL);
	   }
	   else kw("PATTERN") {
		ct++;
		do_set_bar_color(tk[ct], br[g_nbar], BAR_SET_PATTERN);
	   }
	   else kw("BACKGROUND") {
		ct++;
		do_set_bar_color(tk[ct], br[g_nbar], BAR_SET_BACKGROUND);
	   }
	   else {
		g_throw_parser_error("unrecognised bar sub command '", tk[ct], "'");
	   }
	  ct++;
	}
}
goto do_next_line;
/*----------------------------------------------------------------*/
/* fill x1,d2 color green xmin 1 xmax 2 ymin 1 ymax 2   */
/* fill d1,x2 color green xmin 1 xmax 2 ymin 1 ymax 2   */
/* fill d1,d2 color green xmin 1 xmax 2 ymin 1 ymax 2   */
/* fill d1 color green xmin 1 xmax 2 ymin 1 ymax 2      */
do_fill:
{
	char *ss,s1[40],s2[40];
	fd[++nfd] = FD_CAST myallocz(sizeof(*fd[1]));
	ct = 2;
	strcpy(s1,strtok(tk[ct],","));
	ss = strtok(0,",");
	if (ss==NULL) strcpy(s2,""); else {
		strcpy(s2,ss);
		strtok(0,",");
	}
	if (str_i_equals(s1,"X1")) {
		fd[nfd]->type = 1;
		d = get_dataset_identifier(s2);
		fd[nfd]->da = d;
	} else if (str_i_equals(s2,"X2")) {
		fd[nfd]->type = 2;
		d = get_dataset_identifier(s1);
		fd[nfd]->da = d;
	} else if (!str_i_equals(s2,"")) {
		fd[nfd]->type = 3;
		d = get_dataset_identifier(s1);
		int d2 = get_dataset_identifier(s2);
		fd[nfd]->da = d;
		fd[nfd]->db = d2;
	} else if (toupper(*s1)=='D') {
		fd[nfd]->type = 4;
		d = get_dataset_identifier(s1);
		fd[nfd]->da = d;
	} else {
		g_throw_parser_error("invalid fill option, wanted d1,d2 or x1,d1 or d1,x2 or d1");
	}
	if (fd[nfd]->da != 0) ensureDataSetCreatedAndSetUsed(fd[nfd]->da);
	if (fd[nfd]->db != 0) ensureDataSetCreatedAndSetUsed(fd[nfd]->db);
	ct++;
	fd[nfd]->color = g_get_grey(1.0-nfd*.1);
	fd[nfd]->xmin = -GLE_INF;
	fd[nfd]->xmax = GLE_INF;
	fd[nfd]->ymin = -GLE_INF;
	fd[nfd]->ymax = GLE_INF;
	while (ct<=ntk)  {
		kw("COLOR") 	{ct++; fd[nfd]->color = pass_color(tk[ct]);}
	   else kw("XMIN") 	fd[nfd]->xmin = next_exp;
	   else kw("XMAX") 	fd[nfd]->xmax = next_exp;
	   else kw("YMIN") 	fd[nfd]->ymin = next_exp;
	   else kw("YMAX") 	fd[nfd]->ymax = next_exp;
	   else {
		g_throw_parser_error("unrecognised fill sub command: '", tk[ct], "'");
	   }
	   ct++;
	}
}
goto do_next_line;
/*----------------------------------------------------------------*/
do_key:
	ct = 2;
	while (ct<=ntk)  {
		kw("OFFSET") {
			keyinfo.setOffsetX(next_exp);
			keyinfo.setOffsetY(next_exp);
		}
		else kw("MARGINS") {
			double mx = next_exp;
			double my = next_exp;
			keyinfo.setMarginXY(mx, my);
		}
		else kw("ABSOLUTE") {
			if (g_xsize*g_ysize==0) {
				g_xsize = 10; g_ysize = 10;
				g_get_usersize(&g_xsize,&g_ysize);
			}
			window_set(false);
			store_window_bounds_to_vars();
			set_sizelength();

			keyinfo.setOffsetX(next_exp);
			keyinfo.setOffsetY(next_exp);
			keyinfo.setAbsolute(true);
		}
		else kw("BACKGROUND") keyinfo.setBackgroundColor(next_color);
		else kw("BOXCOLOR") keyinfo.setBoxColor(next_color);
		else kw("ROW") keyinfo.setBase(next_exp);
		else kw("LPOS") keyinfo.setLinePos(next_exp);
		else kw("LLEN") keyinfo.setLineLen(next_exp);
		else kw("NOBOX") keyinfo.setNoBox(true);
		else kw("NOLINE") keyinfo.setNoLines(true);
		else kw("COMPACT") keyinfo.setCompact(true);
		else kw("HEI") keyinfo.setHei(next_exp);
		else kw("POSITION") next_str(keyinfo.getJustify());
		else kw("POS") next_str(keyinfo.getJustify());
		else kw("JUSTIFY") {
			next_str(keyinfo.getJustify());
			keyinfo.setPosOrJust(false);
		}
		else kw("JUST") {
			next_str(keyinfo.getJustify());
			keyinfo.setPosOrJust(false);
		}
		else kw("DIST") keyinfo.setDist(next_exp);
		else kw("COLDIST") keyinfo.setColDist(next_exp);
		else kw("OFF") keyinfo.setDisabled(true);
		else kw("SEPARATOR") {
			if (g_nkd == 0) {
				g_throw_parser_error("key: 'separator' should come after a valid key entry");
			} else {
				ct++;
				kw("LSTYLE") {
					kd[g_nkd]->sepstyle = (int)floor(next_exp + 0.5);
				} else {
					ct--;
				}
				g_keycol++;
			}
		}
		else g_throw_parser_error("unrecognised KEY sub command: '",tk[ct],"'");
		ct++;
	}
goto do_next_line;
/*----------------------------------------------------------------*/
do_data:
{
   data_command();
   goto do_next_line;
}
goto do_next_line;
/*----------------------------------------------------------------*/
do_extract:
	goto do_next_line;
/*----------------------------------------------------------------*/
do_hscale:
	if (str_i_equals(tk[ct+1], "AUTO")) g_auto_s_h = true;
	else g_hscale = next_exp;
	goto do_next_line;
/*----------------------------------------------------------------*/
do_letsave:
	nlet++;
	letline[nlet] = g_get_error_line();
	if (nlet >= MAX_NUMBER_OF_LET_COMMANDS) gprint("TOO MANY LET Commands in graph! the maximum let commands allowed is %d\n",MAX_NUMBER_OF_LET_COMMANDS);
	goto do_next_line;
/*----------------------------------------------------------------*/
do_size:
	g_xsize = next_exp;
	g_ysize = next_exp;
	/* ! set up some more constants */
	set_sizelength();
	do_remaining_entries(ct+1);
	goto do_next_line;
/*----------------------------------------------------------------*/
do_scale:
	if (str_i_equals(tk[ct+1], "AUTO")) {
		g_auto_s_v = true;
		g_auto_s_h = true;
		ct++;
	} else {
		g_hscale = next_exp;
		g_vscale = next_exp;
	}
	do_remaining_entries(ct+1);
	goto do_next_line;
do_vscale:
	if (str_i_equals(tk[ct+1], "AUTO")) g_auto_s_v = true;
	else g_vscale = next_exp;
	goto do_next_line;
/*----------------------------------------------------------------*/
do_colormap:
	g_colormap = new GLEColorMap();
	g_colormap->setFunction(tk[++ct]);
	g_colormap->setWidth((int)floor(next_exp+0.5));
	g_colormap->setHeight((int)floor(next_exp+0.5));
	ct++;
	while (ct <= ntk) {
		kw("COLOR") g_colormap->setColor(true);
		kw("INVERT") g_colormap->setInvert(true);
		kw("ZMIN") g_colormap->setZMin(next_exp);
		kw("ZMAX") g_colormap->setZMax(next_exp);
		kw("PALETTE") {
			string tmp;
			next_str_cpp(tmp);
			str_to_uppercase(tmp);
			g_colormap->setPalette(tmp);
		}
		ct++;
	}
	g_colormap->readData();
	goto do_next_line;
/*----------------------------------------------------------------*/
do_names:
	t = axis_type_check(tk[1]);
	xx[t].label_off = false;
	if (ntk >= 3 && str_i_equals(tk[2], "FROM") && toupper(tk[3][0]) == 'D') {
		// Support syntax "xnames from d1" to retrieve names from the data set
		xx[t].setNamesDataSet(get_dataset_identifier(tk[3]));
	} else {
		ct = 1;
		while (ct<ntk)  {
			next_quote(strbuf);
			xx[t].addName(strbuf);
		}
	}
	goto do_next_line;
/*----------------------------------------------------------------*/
do_places:	/* x2places, or xplaces, or yplaces or y2places */
	t = axis_type_check(tk[1]);
	xx[t].label_off = false;
	ct = 1;
	while (ct<ntk)  {
		xx[t].addPlace(next_exp);
	}
	goto do_next_line;
/*----------------------------------------------------------------*/
do_noticks:
	t = axis_type_check(tk[1]);
	ct = 1;
	xx[t].clearNoTicks();
	if (t <= 2) xx[t+2].clearNoTicks();
	while (ct<ntk)  {
		double pos = next_exp;
		xx[t].addNoTick(pos);
		if (t <= 2) xx[t+2].addNoTick(pos);
	}
	goto do_next_line;
/*----------------------------------------------------------------*/
do_main_title:
	/* should change position and size of main title */
	t = GLE_AXIS_T;
	xx[GLE_AXIS_T].off = 0;
	ct = 1;
	next_vquote_cpp(xx[t].title);
	ct = 3;
	xx[t].title_dist = g_fontsz*.7;
	xx[t].title_hei = g_fontsz*g_get_fconst(GLEC_TITLESCALE);
	while (ct<=ntk)  {
	         kw("HEI")     xx[t].title_hei = next_exp;
	    else kw("OFF")     xx[t].title_off = true;
	    else kw("COLOR")   xx[t].title_color = next_color;
	    else kw("FONT")    xx[t].title_font = next_font;
	    else kw("DIST")    xx[t].title_dist = next_exp;
	    else g_throw_parser_error("expecting title sub command, not '", tk[ct], "'");
	    ct++;
	}
	goto do_next_line;
/*----------------------------------------------------------------*/
do_title:
	t = axis_type_check(tk[1]);
	ct = 1;
	next_vquote_cpp(xx[t].title);
	ct = 3;
	while (ct<=ntk)  {
	         kw("HEI")     xx[t].title_hei = next_exp;
	    else kw("OFF")     xx[t].title_off = true;
	    else kw("ROT")     xx[t].title_rot = true;
	    else kw("ROTATE")  xx[t].title_rot = true;
	    else kw("COLOR")   xx[t].title_color = next_color;
	    else kw("FONT")    xx[t].title_font = next_font;
	    else kw("DIST")    xx[t].title_dist = next_exp;
	    else kw("ADIST")   xx[t].title_adist = next_exp;
	    else kw("ALIGN") {
		string base;
		next_str_cpp(base);
		xx[t].setAlignBase(str_i_equals(base, "BASE"));
	    } else {
		g_throw_parser_error("expecting title sub command, not '", tk[ct], "'");
	    }
	    ct++;
	}
	goto do_next_line;
/*----------------------------------------------------------------*/
do_datasets:
	d = get_dataset_identifier(tk[1]); /* dataset number (right$(k$,2))  (0=dn) */
	if (d != 0) {
		ensureDataSetCreatedAndSetUsed(d);
		do_dataset(d);
	} else {
		for (d = 0; d < MAX_NB_DATA; d++) {
			if (dp[d] != NULL) {
				do_dataset(d);
			}
		}
	}
	goto do_next_line;
}

bool do_remaining_entries(int ct) {
	int nb_found = 0;
	bool found = true;
	while (found && ct <= ntk) {
		kw("NOBOX") g_nobox = true;
		else kw("BOX") g_nobox = false;
		else kw("NOBORDER") g_nobox = true; // for compatibility with v3.5
		else kw("BORDER") g_nobox = false;
		else kw("CENTER") g_center = true;
		else kw("FULLSIZE") {
			g_vscale = 1;
			g_hscale = 1;
			g_nobox = true;
		}
		else kw("MATH") {
			g_math = true;
			xx[GLE_AXIS_Y].offset = 0.0;
			xx[GLE_AXIS_Y].has_offset = true;
			xx[GLE_AXIS_Y].ticks_both = true;
			xx[GLE_AXIS_X].offset = 0.0;
			xx[GLE_AXIS_X].has_offset = true;
			xx[GLE_AXIS_X].ticks_both = true;
			xx[GLE_AXIS_X2].off = true;
			xx[GLE_AXIS_Y2].off = true;
		}
		else found = false;
		if (found) {
			ct++;
			nb_found++;
		}
	}
	return nb_found > 0;
}

void do_axis(int axis, bool craxis) throw (ParserError) {
	int ct = 2;
	while (ct <= ntk)  {
		kw("BASE") xx[axis].base = next_exp;
		else kw("COLOR") xx[axis].color = (int) next_exp;
		else kw("DSUBTICKS") xx[axis].dsubticks = next_exp;
		else kw("DTICKS") {
			xx[axis].dticks = next_exp;
			if (craxis) xx[axis].label_off = false;
		} else kw("FTICK") {
			xx[axis].ftick = next_exp;
			xx[axis].has_ftick = true;
		}
		else kw("SYMTICKS") xx[axis].ticks_both = get_on_off(tk, &ct);
		else kw("SHIFT") xx[axis].shift = next_exp;
		else kw("ANGLE") xx[axis].label_angle = next_exp;
		else kw("GRID") {
			xxgrid[axis] = true;
			if (str_i_equals(tk[ct+1],"ONTOP")) {
				xx[axis].setGridOnTop(true);
				ct++;
			}
		}
		else kw("NEGATE") {
			xx[axis].negate = true;
		}
		else kw("FONT") xx[axis].label_font = next_font;
		else kw("LOG") xx[axis].log = true;  /* don't make log depend on craxis, otherwise grid may not line up */
		else kw("LIN") xx[axis].log = false;
		else kw("LSTYLE") next_str(xx[axis].side_lstyle);
		else kw("LWIDTH") xx[axis].side_lwidth = next_exp;
		else kw("MIN") {
			double value = next_exp;
			if (craxis) xx[axis].getRange()->setMinSet(value);
		}
		else kw("MAX") {
			double value = next_exp;
			if (craxis) xx[axis].getRange()->setMaxSet(value);
		}
		else kw("OFFSET") {
			double offs = next_exp;
			if (craxis) {
				xx[axis].offset = offs;
				xx[axis].has_offset = true;
				if (!g_math) {
					if (axis == GLE_AXIS_X) xx[GLE_AXIS_X0].off = false;
					if (axis == GLE_AXIS_Y) xx[GLE_AXIS_Y0].off = false;
				}
			}
		}
		else kw("ROUNDRANGE") xx[axis].roundRange = get_on_off(tk, &ct);
		else kw("HEI") 	xx[axis].label_hei = next_exp;
		else kw("NOLAST") xx[axis].nolast = true;
		else kw("LAST") xx[axis].nolast = !get_on_off(tk, &ct);
		else kw("FIRST") xx[axis].nofirst = !get_on_off(tk, &ct);
		else kw("NOFIRST") xx[axis].nofirst = true;
		else kw("NSUBTICKS") xx[axis].nsubticks = (int) next_exp;
		else kw("NTICKS") {
			xx[axis].nticks = (int) next_exp;
			if (craxis) xx[axis].label_off = false;
		}
		else kw("ON") xx[axis].off = false;
		else kw("OFF") xx[axis].off = true;
		else kw("FORMAT") next_str_cpp(xx[axis].format);
		else kw("SCALE") {
			if (str_i_equals(tk[ct+1],"QUANTILE")) {
				ct++;
				GLERC<GLEAxisQuantileScale> q_scale = new GLEAxisQuantileScale();
				while (true) {
					if (str_i_equals(tk[ct+1], "LOWER")) {
						ct++;
						q_scale->setLowerQuantile(next_exp);
					} else if (str_i_equals(tk[ct+1], "UPPER")) {
						ct++;
						q_scale->setUpperQuantile(next_exp);
					} else if (str_i_equals(tk[ct+1], "FACTOR")) {
						ct++;
						double factor = next_exp;
						q_scale->setLowerQuantileFactor(factor);
						q_scale->setUpperQuantileFactor(factor);
					} else if (str_i_equals(tk[ct+1], "LFACTOR")) {
						ct++;
						q_scale->setLowerQuantileFactor(next_exp);
					} else if (str_i_equals(tk[ct+1], "UFACTOR")) {
						ct++;
						q_scale->setUpperQuantileFactor(next_exp);
					} else {
						break;
					}
				}
				xx[axis].setQuantileScale(q_scale.get());
			}
		}
		else {
			g_throw_parser_error("expecting axis sub command, found '", tk[ct], "'");
		}
		ct++;
	}
}

void do_labels(int axis, bool showerr) throw (ParserError) {
	int ct = 2;
	while (ct <= ntk)  {
		if (*tk[ct]==' ') ct++;
		kw("HEI") xx[axis].label_hei = next_exp;
		else kw("OFF") {
			if (showerr) {
				xx[axis].label_off = true;
				xx[axis].has_label_onoff = true;
			}
		}
		else kw("ON") {
			if (showerr) {
				xx[axis].label_off = false;
				xx[axis].has_label_onoff = true;
				xx[axis].off = false;
			}
		}
		else kw("COLOR") xx[axis].label_color = next_color;
		else kw("FONT") xx[axis].label_font = next_font;
		else kw("DIST") xx[axis].label_dist = next_exp;
		else kw("ALIGN") {
			ct++;
			kw("LEFT") xx[axis].label_align = JUST_LEFT;
			else kw("RIGHT") xx[axis].label_align = JUST_RIGHT;
		}
		else kw("LOG") {
			ct++;
			kw("OFF") xx[axis].lgset = GLE_AXIS_LOG_OFF;
			else kw("L25B") xx[axis].lgset = GLE_AXIS_LOG_25B;
			else kw("L25") xx[axis].lgset = GLE_AXIS_LOG_25;
			else kw("L1") xx[axis].lgset = GLE_AXIS_LOG_1;
			else kw("N1") xx[axis].lgset = GLE_AXIS_LOG_N1;
			else if (showerr) g_throw_parser_error("Expecting OFF, L25, L25B, L1, or N1, found '", tk[ct], "'");
		}
		else if (showerr) {
			g_throw_parser_error("Expecting LABELS sub command, found '", tk[ct], "'");
		}
		ct++;
	}
}

void do_side(int axis, bool showerr) throw (ParserError) {
	int ct = 2;
	while (ct <= ntk)  {
		if (*tk[ct]==' ') ct++;
		kw("OFF") {
			if (showerr) xx[axis].side_off = true;
		}
		else kw("ON") {
			if (showerr) xx[axis].side_off = false;
		}
		else kw("COLOR") xx[axis].side_color = next_color;
		else kw("LWIDTH") xx[axis].side_lwidth = next_exp;
		else kw("LSTYLE") next_str(xx[axis].side_lstyle);
		else if (showerr) {
			g_throw_parser_error("Expecting SIDE sub command, found '", tk[ct], "'");
		}
		ct++;
	}
}

void do_ticks(int axis, bool showerr) throw (ParserError) {
	int ct = 2;
	while (ct <= ntk)  {
		if (*tk[ct]==' ') ct++;
		kw("LENGTH") xx[axis].ticks_length = next_exp;
		else kw("OFF") {
			if (showerr) {
				xx[axis].ticks_off = true;
				xx[axis].subticks_off = true;
			}
		} else kw("ON") {
			if (showerr) {
				xx[axis].ticks_off = false;
				xx[axis].subticks_off = false;
			}
		} else kw("COLOR") {
			xx[axis].ticks_color = next_color;
			xx[axis].subticks_color = xx[axis].ticks_color;
		} else kw("LWIDTH") {
			xx[axis].ticks_lwidth = next_exp;
		} else kw("LSTYLE") {
			next_str(xx[axis].ticks_lstyle);
		} else if (showerr) {
			g_throw_parser_error("Expecting TICKS sub command, found '", tk[ct], "'");
		}
		ct++;
	}
}

void do_subticks(int axis, bool showerr) throw (ParserError) {
	int ct = 2;
	while (ct <= ntk)  {
		if (*tk[ct]==' ') ct++;
		kw("LENGTH") xx[axis].subticks_length = next_exp;
		else kw("OFF") {
			if (showerr) {
				xx[axis].subticks_off = true;
				xx[axis].has_subticks_onoff = true;
			}
		}
		else kw("ON") {
			if (showerr) {
				xx[axis].subticks_off = false;
				xx[axis].has_subticks_onoff = true;
			}
		}
		else kw("COLOR") xx[axis].subticks_color = next_color;
		else kw("LWIDTH") xx[axis].subticks_lwidth = next_exp;
		else kw("LSTYLE") next_str(xx[axis].subticks_lstyle);
		else {
			g_throw_parser_error("Expecting SUBTICKS sub command, found '", tk[ct], "'");
		}
		ct++;
	}
}

void do_axis_part(int axis, bool craxis, int xset) throw (ParserError) {
	// craxis = command is for current axis
	// showerr = passing options for axis command to labels/side/ticks commands
	switch (xset) {
	  case GLEG_CMD_AXIS:
		do_axis(axis, craxis);
		do_labels(axis, false);
		do_side(axis, false);
		do_ticks(axis, false);
		break;
	  case GLEG_CMD_LABELS:
		do_labels(axis, true);
		break;
	  case GLEG_CMD_SIDE:
		do_side(axis, true);
		break;
	  case GLEG_CMD_TICKS:
		do_ticks(axis, true);
		break;
	  case GLEG_CMD_SUBTICKS:
		do_subticks(axis, true);
		break;
	}
}

void do_axis_part_all(int xset) throw (ParserError) {
	int axis = axis_type(tk[1]);
	if (axis == GLE_AXIS_ALL) {
		do_axis_part(GLE_AXIS_X, false, xset);
		do_axis_part(GLE_AXIS_X0, false, xset);
		do_axis_part(GLE_AXIS_X2, false, xset);
		do_axis_part(GLE_AXIS_Y, false, xset);
		do_axis_part(GLE_AXIS_Y0, false, xset);
		do_axis_part(GLE_AXIS_Y2, false, xset);
	} else {
		do_axis_part(axis, true, xset);
	}
	if (axis == GLE_AXIS_X) {
		do_axis_part(GLE_AXIS_X2, false, xset);
		do_axis_part(GLE_AXIS_X0, false, xset);
		do_axis_part(GLE_AXIS_T, false, xset);
	}
	if (axis == GLE_AXIS_Y) {
		do_axis_part(GLE_AXIS_Y2, false, xset);
		do_axis_part(GLE_AXIS_Y0, false, xset);
	}
}

void do_set_bar_color(const char* tk, bar_struct* bar, int type) {
	/* Use tokenizer that jumps over () pairs to be able to parse CVTRGB(...) */
	int fi = 0;
	int dataset = 0;
	string tokstr = (const char*)tk;
	level_char_separator separator(",", "", "(", ")");
	tokenizer<level_char_separator> tokens(tokstr, separator);
	while (tokens.has_more()) {
		int color = pass_color_var(tokens.next_token().c_str());
		switch (type) {
			case BAR_SET_COLOR:
				bar->color[fi++] = color;
				break;
			case BAR_SET_FILL:
				bar->fill[fi] = color;
				dataset = bar->to[fi++];
				if (dp[dataset] != NULL) {
					dp[dataset]->key_fill = color;
				}
				break;
			case BAR_SET_TOP:
				bar->top[fi++] = color;
				break;
			case BAR_SET_SIDE:
				bar->side[fi++] = color;
				break;
			case BAR_SET_PATTERN:
				bar->pattern[fi] = color;
				dataset = bar->to[fi++];
				if (dp[dataset] != NULL) {
					dp[dataset]->key_pattern = color;
				}
				break;
			case BAR_SET_BACKGROUND:
				bar->background[fi] = color;
				dataset = bar->to[fi++];
				if (dp[dataset] != NULL) {
					dp[dataset]->key_background = color;
				}
				break;
		}
	}
}

void do_set_bar_style(const char* tk, bar_struct* bar) {
	int fi = 0;
	string tokstr = (const char*)tk;
	level_char_separator separator(",", "", "(", ")");
	tokenizer<level_char_separator> tokens(tokstr, separator);
	while (tokens.has_more()) {
		pass_file_name(tokens.next_token().c_str(), bar->style[fi]);
		str_to_uppercase(bar->style[fi]);
		fi++;
	}
}

void set_sizelength() {
	double ox, oy;

	/* get the origin x,y */
	g_get_xy(&ox,&oy);
	if (g_hscale==0) g_hscale = .7;
	if (g_vscale==0) g_vscale = .7;

	xbl = ox + (g_xsize/2)-(g_xsize*g_hscale/2);
	ybl = oy + (g_ysize/2)-(g_ysize*g_vscale/2);
	xlength = g_xsize*g_hscale;
	ylength = g_ysize*g_vscale;

	// In more recent versions, the size of a graph font depends on the hei parameter
	if (g_get_compatibility() == GLE_COMPAT_35) {
		if (xlength<ylength) g_fontsz = xlength/23;
		else g_fontsz = ylength/23;
	}

	/* set graph globals */
	graph_x1 = xbl;
	graph_y1 = ybl;
	graph_x2 = xbl + xlength;
	graph_y2 = ybl + ylength;
	graph_xmin = xx[GLE_AXIS_X].getMin();
	graph_xmax = xx[GLE_AXIS_X].getMax();
	graph_ymin = xx[GLE_AXIS_Y].getMin();
	graph_ymax = xx[GLE_AXIS_Y].getMax();
}

void axis_init_length() {
	for (int i = 1; i <= GLE_AXIS_MAX; i++) {
		xx[i].type = i;
		if (xx[i].base==0) xx[i].base = g_fontsz;
		if (axis_horizontal(i)) xx[i].length = xlength;
		else xx[i].length = ylength;
	}
}

void axis_add_grid() {
	for (int i = 1; i <= 2; i++) {
		if (xxgrid[i]) {
			double len = axis_horizontal(i) ? ylength : xlength;
			if (!xx[i].hasGridOnTop()) xx[i].setGrid(true);
			xx[i].ticks_length = len;
			xx[i+2].ticks_off = true;
			if (xx[i].subticks_length == 0.0) {
				xx[i].subticks_length = len;
				xx[i+2].subticks_off = true;
			}
			if (!xx[i].has_subticks_onoff) {
				/* only change default behaviour if no explicit setting given */
				if (xx[i].log) {
					xx[i].subticks_off = false;
				} else {
					xx[i].subticks_off = true;
				}
			}
		}
	}
}

void axis_add_noticks() {
	// Disable ticks and places on some axis
	for (int i = 1; i < GLE_AXIS_MAX; i++) {
		if (!xx[i].off) {
			if (xx[i].has_offset) {
				// axis has offset:
				// - disable ticks and labels at each intersection
				for (int j = 0; j < 3; j++) {
					int orth = axis_get_orth(i, j);
					if (!xx[orth].off) {
						if (xx[orth].has_offset) {
							xx[i].insertNoTickOrLabel(xx[orth].offset);
						} else {
							if (axis_is_max(orth)) xx[i].insertNoTickOrLabel(xx[i].getMax());
							else xx[i].insertNoTickOrLabel(xx[i].getMin());
						}
					}
				}
			} else {
				// axis has no offset:
				// - disable inside ticks (ticks1) for each intersection
				for (int j = 0; j < 3; j++) {
					int orth = axis_get_orth(i, j);
					if (!xx[orth].off) {
						if (xx[orth].has_offset) {
							xx[i].insertNoTick1(xx[orth].offset);
						} else {
							if (axis_is_max(orth)) xx[i].insertNoTick1(xx[i].getMax());
							else xx[i].insertNoTick1(xx[i].getMin());
						}
					}
				}
			}
		}
	}
#ifdef AXIS_DEBUG
	cout << endl;
	for (int i = 1; i <= GLE_AXIS_MAX; i++) {
		cout << "AXIS: " << i << endl;
		xx[i].printNoTicks();
		cout << endl;
	}
#endif
}

void draw_axis_pos(int axis, double xpos, double ypos, bool xy, bool grid, GLERectangle* box) {
	if (xx[axis].has_offset) {
		if (xy) g_move(graph_xgraph(xx[axis].offset), ypos);
		else g_move(xpos, graph_ygraph(xx[axis].offset));
	} else {
		g_move(xpos,ypos);
	}
	draw_axis(&xx[axis], box, grid);
}

void graph_draw_grids() {
	GLERectangle box;
	box.initRange();
	draw_axis_pos(GLE_AXIS_Y0, xbl, ybl, true, true, &box);
	draw_axis_pos(GLE_AXIS_Y, xbl, ybl, true, true, &box);
	draw_axis_pos(GLE_AXIS_Y2, xbl+xlength, ybl, true, true, &box);
	draw_axis_pos(GLE_AXIS_X, xbl, ybl, false, true, &box);
	draw_axis_pos(GLE_AXIS_X0, xbl, ybl, false, true, &box);
	draw_axis_pos(GLE_AXIS_X2, xbl, ybl+ylength, false, true, &box);
}

void graph_draw_axis(GLERectangle* box) {
	// y-axis should *not* influence position of title!
	draw_axis_pos(GLE_AXIS_Y0, xbl, ybl, true, false, box);
	draw_axis_pos(GLE_AXIS_Y, xbl, ybl, true, false, box);
	draw_axis_pos(GLE_AXIS_Y2, xbl+xlength, ybl, true, false, box);
	GLEMeasureBox measure;
	measure.measureStart();
	draw_axis_pos(GLE_AXIS_X, xbl, ybl, false, false, box);
	draw_axis_pos(GLE_AXIS_X0, xbl, ybl, false, false, box);
	draw_axis_pos(GLE_AXIS_X2, xbl, ybl+ylength, false, false, box);
	g_update_bounds(xbl+xlength/2, ybl+ylength);
	measure.measureEnd();
	draw_axis_pos(GLE_AXIS_T, xbl, measure.getYMax(), true, false, box);
	// otherwise it does not work if all axis are disabled!
	g_update_bounds_box(box);
}

void prepare_graph_key_and_clip(double ox, double oy, KeyInfo* keyinfo) {
	if (!keyinfo->hasHei()) keyinfo->setHei(g_fontsz);
	keyinfo->setNbEntries(g_nkd);
	measure_key(keyinfo);
	if (keyinfo->getNbEntries() > 0 && !keyinfo->isDisabled() && !keyinfo->getNoBox() && keyinfo->getBackgroundColor() == (int)GLE_FILL_CLEAR) {
		g_gsave();
		g_beginclip();
		g_set_path(true);
		g_newpath();
		GLERectangle fullFig;
		g_get_userbox_undev(&fullFig);
		g_box_stroke(&fullFig, true);
		g_box_stroke(keyinfo->getRect(), false);
		g_clip();
		g_set_path(false);
	}
}

void draw_graph(KeyInfo* keyinfo) throw (ParserError) {
	GLERectangle box;
	int i;
	double ox,oy;

	do_bigfile_compatibility();
	g_get_bounds(&box);

	/* if no size given, take entire drawing */
	if (g_xsize*g_ysize==0) {
		g_xsize = 10; g_ysize = 10;
		g_get_usersize(&g_xsize,&g_ysize);
	}

	/* get scales from data sets */
	do_each_dataset_settings();
	set_bar_axis_places();
	get_dataset_ranges();

	/* this window set is there to initialize ranges based on data */
	/* in case there is no ranges given in let commands */
	window_set(false);

	/* update scale based on let commands if not explicit scale given */
	if (should_autorange_based_on_lets()) {
		/* if min max not set, do_let in approximate way first */
		for (i = 1; i <= nlet; i++) {
			do_let(letline[i], false);
		}
		get_dataset_ranges();
		for (int i = 1; i <= ndata; i++) {
			if (dp[i] != NULL) {
				dp[i]->restore();
			}
		}
	} else {
		/* otherwise window_set will round ranges found by previous window_set! */
		reset_axis_ranges();
	}

	window_set(true);
	store_window_bounds_to_vars();

	/* get the origin x,y */
	g_get_xy(&ox,&oy);
	g_gsave();
	set_sizelength();
	g_set_hei(g_fontsz);

	/* draw the box */
	if (!g_nobox) {
		g_box_stroke(ox, oy, ox+g_xsize, oy+g_ysize);
	}

	/* init axis things */
	vinit_title_axis();
	axis_add_noticks();
	axis_init_length();

	/* center? */
	if (g_center || g_auto_s_v || g_auto_s_h) {
		GLERectangle dummy;
		dummy.initRange();
		GLEMeasureBox measure;
		GLEDevice* old_device = g_set_dummy_device();
		measure.measureStart();
		graph_draw_axis(&dummy);
		measure.measureEnd();
		g_restore_device(old_device);
		if (g_auto_s_h) {
			double d1 = measure.getXMin() - ox - g_fontsz/5;
			double d2 = ox + g_xsize - measure.getXMax() - g_fontsz/5;
			double l1 = ox + g_xsize/2 - xlength/2 - measure.getXMin();
			double new_xlen = xlength + d1 + d2;
			g_hscale = new_xlen/g_xsize;
			ox += new_xlen/2 - g_xsize/2 + l1 + g_fontsz/5;
		} else if (g_center) {
			ox += ox+g_xsize/2.0 - measure.getXMid();
		}
		if (g_auto_s_v) {
			double d1 = measure.getYMin() - oy - g_fontsz/5;
			double d2 = oy + g_ysize - measure.getYMax() - g_fontsz/5;
			double l1 = oy + g_ysize/2 - ylength/2 - measure.getYMin();
			double new_ylen = ylength + d1 + d2;
			g_vscale = new_ylen/g_ysize;
			oy += new_ylen/2 - g_ysize/2 + l1 + g_fontsz/5;
		} else if (g_center) {
			oy += oy+g_ysize/2.0 - measure.getYMid();
		}
		g_move(ox,oy);
		set_sizelength();
		axis_init_length();
	}

	/* Measure key */
	g_move(ox,oy);
	prepare_graph_key_and_clip(ox, oy, keyinfo);

	/* must come after auto scale */
	axis_add_grid();

	/* do LETS now */
	for (i = 1; i <= nlet; i++) {
		do_let(letline[i], true);
	}

	/* Throw away missing values if NOMISS on datasets */
	gr_thrownomiss();

	/* Draw graph background */
	if (g_graph_background != (int)GLE_FILL_CLEAR) {
		int old_fill;
		g_get_fill(&old_fill);
		g_set_fill(g_graph_background);
		g_box_fill(graph_x1, graph_y1, graph_x2, graph_y2);
		g_set_fill(old_fill);
	}

	if (g_colormap != NULL) {
		g_colormap->setXRange(xx[1].getMin(), xx[1].getMax());
		g_colormap->setYRange(xx[2].getMin(), xx[2].getMax());
		g_colormap->draw(graph_x1, graph_y1, xlength, ylength);
		delete g_colormap;
		g_colormap = NULL;
	}

	/* draw the grids */
	graph_draw_grids();

	/* Lets do any filling now. */
	draw_fills();
	g_move(ox,oy);

	/* Draw the bars */
	draw_bars();

	/* Draw the function calls under the axis */
	draw_user_function_calls(true);

	/* Draw the axis */
	g_init_bounds();
	graph_draw_axis(&box);

	/* Draw the function calls on top of the axis */
	draw_user_function_calls(false);

	/* Draw the lines and  markers */
	draw_lines();
	g_move(ox,oy);

	/* Draw the error bars */
	draw_err();
	g_move(ox,oy);

	/* Draw markers, */
	/* (after lines so symbol blanking [white markers] will work) */
	draw_markers();

	/* Draw the key */
	if (keyinfo->getNbEntries() > 0 && !keyinfo->isDisabled() && !keyinfo->getNoBox() && keyinfo->getBackgroundColor() == (int)GLE_FILL_CLEAR) {
		g_endclip();
		g_grestore();
	}
	draw_key_after_measure(keyinfo);

	g_move(ox,oy);
	g_grestore();
	g_init_bounds();
	g_set_bounds(&box);
	return;
}

void g_graph_init() {
	for (int i = 0; i < MAX_NB_FILL; i++) {
		fd[i] = NULL;
	}
	for (int i = 0; i < MAX_NB_DATA; i++) {
		dp[i] = NULL;
	}
}

void graph_free() {
	for (int i = 0; i < MAX_NB_FILL; i++) {
		if (fd[i] != NULL) {
			myfree(fd[i]);
			fd[i] = NULL;
		}
	}
	for (int i = 0; i < MAX_NB_DATA; i++) {
		if (dp[i] != NULL) {
			iffree(dp[i]->key_name,"a");
			delete dp[i];
		}
		dp[i] = NULL;
	}
}

void graph_freebars() {
	for (int i = 1; i <= g_nbar; i++) {
		delete br[i];
		br[i] = NULL;
	}
	g_nbar = 0;
}

void graph_init() {
	g_graph_background = GLE_FILL_CLEAR;
	ndata = 0; nfd = 0; g_nbar = 0;
	xx[GLE_AXIS_X0].off = true;
	xx[GLE_AXIS_Y0].off = true;
	xx[GLE_AXIS_T].off = true;
	graph_freebars();
	graph_free();
	g_fcalls.clear();
	g_funder.clear();
}

void iffree(void *p, const char *s) {
	if (p!=NULL) myfrees(p,s);
}

int freedataset(int d) {
	int i,c=0;
	for (i=1;i<=ndata;i++)
	{
		if (dp[i] == NULL) c++;
		else if (dp[i]->xv == NULL) c++;
		if (c==d) return i;
	}
	return ndata + d - c;
}

void copy_default(int dn) {
	dp[dn]->copy(dp[0]);
	dp[dn]->key_name = NULL;
	dp[dn]->axisscale = false;
}

bool is_float_miss(const string& str) {
	if (is_float(str)) return true;
	if (str == "*") return true;
	return false;
}

int get_dataset_identifier(const char* ds, bool def) throw(ParserError) {
	int len = strlen(ds);
	if (len <= 1 || toupper(ds[0]) != 'D') {
		g_throw_parser_error("illegal data set identifier '", ds, "'");
	}
	if (str_i_equals(ds, "dn")) {
		return 0;
	}
	char* ptr = NULL;
	int result = strtol(ds+1, &ptr, 10);
	if (*ptr != 0) {
		g_throw_parser_error("data set identifier should be integer, not '", ds, "'");
	}
	if (result < 0 || result >= MAX_NB_DATA) {
		g_throw_parser_error("data set identifier out of range '", ds, "'");
	}
	if (def && dp[result] == NULL) {
		g_throw_parser_error("data set '", ds, "' not defined");
	}
	return result;
}

int get_dataset_identifier(const string& ds, GLEParser* parser, bool def) throw(ParserError) {
	Tokenizer* tokens = parser->getTokens();
	if (str_i_equals(ds, "d")) {
		tokens->ensure_next_token("[");
		int result = (int)floor(parser->evalTokenToDouble() + 0.5);
		if (result < 0 || result >= MAX_NB_DATA) {
			ostringstream err;
			err << "data set identifier out of range: '" << result << "'";
			throw tokens->error(err.str());
		}
		tokens->ensure_next_token("]");
		if (def && dp[result] == NULL) {
			ostringstream err;
			err << "data set d" << result << " not defined";
			throw tokens->error(err.str());
		}
		return result;
	} else if (str_i_equals(ds, "dn")) {
		return 0;
	} else {
		if (ds.size() <= 1 || toupper(ds[0]) != 'D') {
			throw tokens->error("illegal data set identifier");
		}
		char* ptr = NULL;
		int result = strtol(ds.c_str() + 1, &ptr, 10);
		if (*ptr != 0) {
			throw tokens->error("data set identifier should be integer");
		}
		if (result < 0 || result >= MAX_NB_DATA) {
			throw tokens->error("data set identifier out of range");
		}
		if (def && dp[result] == NULL) {
			throw tokens->error("data set '", ds.c_str(), "' not defined");
		}
		return result;
	}
}

int get_column_number(GLEParser* parser) throw(ParserError) {
	Tokenizer* tokens = parser->getTokens();
	const string& token = tokens->next_token();
	if (str_i_equals(token, "c")) {
		tokens->ensure_next_token("[");
		int result = (int)floor(parser->evalTokenToDouble() + 0.5);
		if (result < 0) {
			ostringstream err;
			err << "column index out of range: '" << result << "'";
			throw tokens->error(err.str());
		}
		tokens->ensure_next_token("]");
		return result;
	} else {
		if (token.size() <= 1 || toupper(token[0]) != 'C') {
			throw tokens->error("illegal column index '", token.c_str(), "'");
		}
		char* ptr = NULL;
		int result = strtol(token.c_str()+1, &ptr, 10);
		if (*ptr != 0) {
			throw tokens->error("column index should be integer, not '", token.c_str(), "'");
		}
		if (result < 0) {
			throw tokens->error("column index out of range '", token.c_str(), "'");
		}
		return result;
	}
}

class GLEDataSetDescription {
public:
	GLEDataSetDescription();
	int ds;
	bool xygiven;
	int cx;
	int cy;
};

class GLEDataDescription {
public:
	GLEDataDescription();
	inline int getNbDataSets() { return m_description.size(); }
	inline GLEDataSetDescription* getDataSet(int i) { return &m_description[i]; }
	inline void addDataSet(const GLEDataSetDescription& description) { m_description.push_back(description); }

private:
		vector<GLEDataSetDescription> m_description;
public:
	string fileName;
	string comment;
	int ignore;
	bool nox;
};

GLEDataSetDescription::GLEDataSetDescription() {
	ds = 0;
	xygiven = false;
	cx = -1;
	cy = -1;
}

GLEDataDescription::GLEDataDescription() {
	ignore = -1;
	nox = false;
}

/* data a.dat												  */
/* data a.dat d2 d5											*/
/* data a.dat d1=c1,c3 d2=c5,c1						  */
/* data a.dat IGNORE n d2 d5							  */
/* data a.dat IGNORE n d1=c1,c3 d2=c5,c1			  */
/* data a.dat COMMENT # d1=c1,c3 d2=c5,c1				*/
/* data a.dat COMMENT # IGNORE n d1=c1,c3 d2=c5,c1	*/

void read_data_description(GLEDataDescription* description) {
	// Get data command
	string datacmd;
	get_block_line(g_get_error_line(), datacmd);
	// Initialize parser for this line
	GLEParser* parser = get_global_parser();
	parser->setString(datacmd.c_str());
	Tokenizer* tokens = parser->getTokens();
	tokens->ensure_next_token_i("DATA");
	// Read file name
	parser->evalTokenToFileName(&description->fileName);
	// Parse rest of the line
	while (true) {
		// read next token from the data description
		const string& token = tokens->try_next_token();
		if (token == "") {
			// no further tokens
			break;
		}
		// check if it is one of the options
		if (str_i_equals(token, "IGNORE")) {
			description->ignore = tokens->next_integer();
		} else if (str_i_equals(token, "COMMENT")) {
			parser->evalTokenToFileName(&description->comment);
		} else if(str_i_equals(token,"NOX")) {
			description->nox = true;
		} else {
			// check if it is a data set identifier
			GLEDataSetDescription datasetDescription;
			datasetDescription.ds = get_dataset_identifier(token, parser, false);
			if (tokens->is_next_token("=")) {
				datasetDescription.xygiven = true;
				datasetDescription.cx = get_column_number(parser);
				tokens->ensure_next_token(",");
				datasetDescription.cy = get_column_number(parser);
			}
			description->addDataSet(datasetDescription);
		}
	}
}

double get_data_value(const vector<string>& temp, int dn, int point, const char* xy, int offs, bool* missing) {
	const string& value(temp[offs]);
	if (value == "*") {
		*missing = true;
		return 0.0;
	} else {
		char *ptr;
		double result = strtod(value.c_str(), &ptr);
		if (*ptr != 0) {
/*			ostringstream err;
			err << "expected floating point number, not '" << value << "'";
			err << ", while reading the " << xy << "-value of point " << (point + 1) << " of d" << dn;
			g_throw_parser_error(err.str());  */
		}
		return result;
	}
}

void createDataSet(int dn) {
	if (dn > ndata) ndata = dn;
	if (dp[dn] == NULL) {
		dp[dn] = new GLEDataSet();
		copy_default(dn);
	}
}

void data_command() {
	// Read data set description
	GLEDataDescription description;
	read_data_description(&description);
	// Open file
	ifstream fptr;
	validate_open_input_stream(fptr, description.fileName);
	// File data
	int fileLine = 0;
	int dataLine = 0;
	int dataColumns = -1;
	vector<string> temp;
	// Tokenizer
	TokenizerLanguage tok_lang;
	StringTokenizer str_tok(&tok_lang);
	tok_lang.setSpaceTokens(" ,;\t");
	tok_lang.setLineCommentTokens("!");
	tok_lang.setDecimalDot('.');
	tok_lang.setParseStrings(true);
	str_tok.set_fname(description.fileName.c_str());
	// Read file line by line
	while (fptr.good()) {
		string line;
		getline(fptr, line);
		str_trim_right(line);
		if (fileLine >= description.ignore) {
			str_tok.set_string(line);
			str_tok.inc_line();
			int lineColumns = 0;
			while (true) {
				string& token = str_tok.try_next_token();
				// must compare first porton of next token
				const string& comment = description.comment;
				if (token == "" || (comment.length() > 0 &&  token.find(comment) == 0 ) ) {
					break;
				}
				if (token == "*" || token == "?" || token == "-" || token == ".") {
					token = "*";
				}
				lineColumns++;
				temp.push_back(token);
			}
			if (lineColumns > 0) {
				dataLine++;
				if (dataColumns == -1) {
					dataColumns = lineColumns;
				} else {
					if (lineColumns != dataColumns) {
						ostringstream err;
						err << "inconsistent number of columns " << lineColumns << " <> " << dataColumns;
						err << " at line " << (fileLine + 1);
						err	<< " in '" << description.fileName << "'";
						g_throw_parser_error(err.str());
					}
				}
			}
		}
		fileLine++;
	}
	fptr.close();
	// Handle special types of data sets
	bool all_str_col = true;
	for (int row = 0; row < dataLine; row++) {
		if (is_float_miss(temp[row * dataColumns])) {
			all_str_col = false;
			break;
		}
	}
	bool nox = description.nox || dataColumns == 1 || all_str_col;
	// Auto-assign columns
	int cx = nox ? 0 : 1;
	int cyOffs = nox && !all_str_col ? 0 : 1;
	int nbDataSetsGiven = description.getNbDataSets();
	for (int i = 0; i < nbDataSetsGiven; ++i) {
		GLEDataSetDescription* dataset = description.getDataSet(i);
		if (!dataset->xygiven) {
			dataset->cx = cx;
			dataset->cy = i + cyOffs + 1;
		}
	}
	if (nbDataSetsGiven == 0) {
		int nbDataSetsMax = dataColumns - cyOffs;
		for (int i = 0; i < nbDataSetsMax; ++i) {
			GLEDataSetDescription dataset;
			dataset.ds = freedataset(i + 1);
			dataset.cx = cx;
			dataset.cy = i + cyOffs + 1;
			description.addDataSet(dataset);
		}
	}
	// Construct names data set
	if (all_str_col) {
		int ds = 0;
		GLEDataSetDescription dataset;
		dataset.ds = ds;
		dataset.cx = 0;
		dataset.cy = 1;
		description.addDataSet(dataset);
		xx[GLE_AXIS_X].setNamesDataSet(ds);
	}
	// Validate column indices
	for (int i = 0; i < description.getNbDataSets(); i++) {
		GLEDataSetDescription* dataset = description.getDataSet(i);
		if (dataset->cx < 0 || dataset->cx > dataColumns) {
			ostringstream err;
			err << "x-value column index out of range for d" << dataset->ds << ": " << dataset->cx << " not in [0,...," << dataColumns << "]";
			g_throw_parser_error(err.str());
		}
		if (dataset->cy < 0 || dataset->cy > dataColumns) {
			ostringstream err;
			err << "y-value column index out of range for d" << dataset->ds << ": " << dataset->cy << " not in [0,...," << dataColumns << "]";
			g_throw_parser_error(err.str());
		}
	}
	// Auto-detect header
	bool has_header = true;
	for (int col = 0; col < dataColumns; col++) {
		if (is_float_miss(temp[col])) {
			has_header = false;
			break;
		}
	}
	// Read header and copy to key
	int first_row = 0;
	if (has_header) {
		first_row = 1;
		for (int i = 0; i < description.getNbDataSets(); i++) {
			GLEDataSetDescription* dataset = description.getDataSet(i);
			int dn = dataset->ds;
			int cy = dataset->cy;
			createDataSet(dn);
			str_remove_quote(temp[cy - 1]);
			dp[dn]->key_name = sdup(temp[cy - 1].c_str());
		}
	}
	// Copy all data to the data sets
	for (int i = 0; i < description.getNbDataSets(); i++) {
		GLEDataSetDescription* dataset = description.getDataSet(i);
		int dn = dataset->ds;
		createDataSet(dn);
		int np = dataLine - first_row;
		dp[dn]->clearAll();
		dp[dn]->xv = (double*) myallocz(sizeof(double) * (np + 1));
		dp[dn]->yv = (double*) myallocz(sizeof(double) * (np + 1));
		dp[dn]->miss = (int*) myallocz(sizeof(int) * (np + 1));
		dp[dn]->yv_str = new vector<string>();
		dp[dn]->np = np;
		for (int j = 0; j < np ; j++) {
			bool missing = false;
			if (dataset->cx == 0) {
				dp[dn]->xv[j] = j + 1;
			} else {
				int x_offs = (j + first_row) * dataColumns + dataset->cx - 1;
				dp[dn]->xv[j] = get_data_value(temp, dn, j, "x", x_offs, &missing);
			}
			int y_offs = (j + first_row) * dataColumns + dataset->cy - 1;
			dp[dn]->yv[j] = get_data_value(temp, dn, j, "y", y_offs, &missing);
			dp[dn]->miss[j] = missing;
			// yv_str is used to use this data set as labels
			str_remove_quote(temp[y_offs]);
			dp[dn]->yv_str->push_back(temp[y_offs]);
		}
	}
}
