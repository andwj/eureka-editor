//------------------------------------------------------------------------
//  TEXT EDITOR WINDOW
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2018 Andrew Apted
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#include "main.h"
#include "w_wad.h"

#include "ui_window.h"


class UI_TedStatusBar : public Fl_Group
{
private:
	Fl_Box *row_col;
	Fl_Box *mod_box;

	int  cur_row;
	int  cur_column;
	bool cur_modified;

public:
	UI_TedStatusBar(int X, int Y, int W, int H, const char *label = NULL) :
		Fl_Group(X, Y, W, H, label),
		cur_row(1), cur_column(1), cur_modified(false)
	{
		box(FL_UP_BOX);

		row_col = new Fl_Box(FL_FLAT_BOX, X, Y+1, W*2/3, H-2, "");
		row_col->align(FL_ALIGN_INSIDE | FL_ALIGN_LEFT);

		mod_box = new Fl_Box(FL_FLAT_BOX, X+W*2/3, Y+1, W/3, H-2, "");
		mod_box->align(FL_ALIGN_INSIDE | FL_ALIGN_RIGHT);

		end();

		Update();
	}

	virtual ~UI_TedStatusBar()
	{ }

public:
	void SetPosition(int row, int column)
	{
		if (row != cur_row || column != cur_column)
		{
			cur_row = row;
			cur_column = column;

			Update();
		}
	}

	void SetModified(bool modified)
	{
		if (modified != cur_modified)
		{
			cur_modified = modified;
			Update();
		}
	}

private:
	void Update()
	{
		static char buffer[256];

		snprintf(buffer, sizeof(buffer), " Line: %-6d Col: %d", cur_row, cur_column);

		row_col->copy_label(buffer);

		if (cur_modified)
			mod_box->label("MODIFIED ");
		else
			mod_box->label("");

		// ensure background gets redrawn
		redraw();
	}
};


//------------------------------------------------------------------------

// andrewj: this class only exists because a very useful method of
//          Fl_Text_Display is not public.  FFS.
class UI_TedWrapper : public Fl_Text_Editor
{
public:
	UI_TedWrapper(int X, int Y, int W, int H, const char *l=0) :
		Fl_Text_Editor(X, Y, W, H, l)
	{ }

	virtual ~UI_TedWrapper()
	{ }

	bool GetLineAndColumn(int *line, int *col)
	{
		if (position_to_linecol(insert_position(), line, col) == 0)
			return false;

		*col += 1;

		return true;
	}
};


//------------------------------------------------------------------------

static void ted_do_save(Fl_Widget *w, void *data)
{
	// FIXME
}

static void ted_do_quit(Fl_Widget *w, void *data)
{
	UI_TextEditor *ted = (UI_TextEditor *)data;
	SYS_ASSERT(ted);

	ted->Cmd_Quit();
}

static void ted_do_undo(Fl_Widget *w, void *data)
{
	// FIXME
}

static void ted_do_redo(Fl_Widget *w, void *data)
{
	// FIXME
}

static void ted_do_find(Fl_Widget *w, void *data)
{
	// FIXME
}


#undef FCAL
#define FCAL  (Fl_Callback *)

static Fl_Menu_Item ted_menu_items[] =
{
	{ "&File", 0, 0, 0, FL_SUBMENU },
		{ "&Insert File...",       FL_COMMAND + 'i', FCAL ted_do_save },
		{ "&Export to File...  ",   FL_COMMAND + 'e', FCAL ted_do_save },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "&Save Lump",   FL_COMMAND + 's', FCAL ted_do_save },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "&Close",       FL_COMMAND + 'q', FCAL ted_do_quit },
		{ 0 },

	{ "&Edit", 0, 0, 0, FL_SUBMENU },
		{ "&Undo",    FL_COMMAND + 'z',  FCAL ted_do_undo },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "Cu&t",     FL_COMMAND + 'x',  FCAL ted_do_redo },
		{ "&Copy",    FL_COMMAND + 'c',  FCAL ted_do_redo },
		{ "&Paste",   FL_COMMAND + 'v',  FCAL ted_do_redo },
		{ "&Delete",  0,                 FCAL ted_do_redo },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "Select &All",  FL_COMMAND + 'a',  FCAL ted_do_redo },
		{ "Unselect All  ", FL_COMMAND + 'u',  FCAL ted_do_redo },
		{ 0 },

	{ "&Search", 0, 0, 0, FL_SUBMENU },
		{ "&Find",     FL_COMMAND + 'f',  FCAL ted_do_find },
		{ "Find Next", FL_COMMAND + 'g',  FCAL ted_do_find },
		{ "&Replace",  FL_COMMAND + 'r',  FCAL ted_do_find },
		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },
		{ "&Next Wotsit  ",  FL_COMMAND + 'n',  FCAL ted_do_find },
		{ "Go to &Top",     FL_COMMAND + 't',  FCAL ted_do_find },
		{ "Go to &Bottom  ",  FL_COMMAND + 'b',  FCAL ted_do_find },
		{ 0 },

	{ "&View", 0, 0, 0, FL_SUBMENU },
		// TODO : flesh these out   [ will need config-file vars too ]
		{ "Colors",   0,                 FCAL ted_do_find },
		{ "Font",     0,                 FCAL ted_do_find },
		{ "Line Numbers", 0,                 FCAL ted_do_find },
		{ 0 },

	{ 0 }
};


//------------------------------------------------------------------------

UI_TextEditor::UI_TextEditor() :
	Fl_Double_Window(580, 400, ""),
	want_close(false),
	read_only(false),
	has_changes(false)
{
	callback((Fl_Callback *) close_callback, this);

	color(WINDOW_BG, WINDOW_BG);

	int MW = w() / 2;

	menu_bar = new Fl_Menu_Bar(0, 0, MW, 28);
	menu_bar->copy(ted_menu_items, this /* userdata for every menu item */);

	status = new UI_TedStatusBar(MW, 0, w() - MW, 28);

	ted = new UI_TedWrapper(0, 28, w(), h() - 28);

	ted->color(FL_BLACK, FL_BLACK);
	ted->textfont(FL_COURIER);
	ted->textsize(18);
	ted->textcolor(fl_rgb_color(192,192,192));

	ted->cursor_color(FL_RED);
	ted->cursor_style(Fl_Text_Display::HEAVY_CURSOR);

	tbuf = new Fl_Text_Buffer();
	tbuf->add_modify_callback(text_modified_callback, this);

	ted->buffer(tbuf);

	resizable(ted);

	end();
}


UI_TextEditor::~UI_TextEditor()
{
	ted->buffer(NULL);

	delete tbuf; tbuf = NULL;
}


int UI_TextEditor::Run()
{
	set_modal();

	show();

	while (! want_close)
	{
		Fl::wait(0.2);

		UpdateStatus();
	}

	return 0;
}


void UI_TextEditor::Cmd_Quit()
{
	close_callback(this, this);
}


void UI_TextEditor::close_callback(Fl_Widget *w, void *data)
{
	UI_TextEditor * that = (UI_TextEditor *)data;

	// FIXME : if modified...

	that->want_close = true;
}


void UI_TextEditor::text_modified_callback(int, int nInserted, int nDeleted, int, const char*, void *data)
{
	UI_TextEditor * that = (UI_TextEditor *)data;

	if (nInserted + nDeleted > 0)
		that->has_changes = true;
}


// this sets the window's title too
bool UI_TextEditor::LoadLump(Wad_file *wad, const char *lump_name)
{
	static char title_buf[FL_PATH_MAX];

	Lump_c * lump = wad->FindLump(lump_name);

	// if the lump does not exist, we will create it
	if (! lump)
	{
		if (read_only)
		{
			// FIXME DLG_Notify
//!!!!			return false;
		}

		sprintf(title_buf, "%s lump (new)", lump_name);
		copy_label(title_buf);

		return true;
	}

	LogPrintf("Reading '%s' text lump\n", lump_name);

	if (! lump->Seek())
	{
		// FIXME: DLG_Notify
		return false;
	}

	// FIXME: LoadLump

	if (read_only)
		sprintf(title_buf, "%s lump (read-only)", lump_name);
	else
		sprintf(title_buf, "%s lump", lump_name);

	copy_label(title_buf);

	return true;
}


bool UI_TextEditor::SaveLump(Wad_file *wad, const char *lump_name)
{
	LogPrintf("Writing '%s' text lump\n", lump_name);

	wad->BeginWrite();

	int oldie = wad->FindLumpNum(lump_name);
	if (oldie >= 0)
		wad->RemoveLumps(oldie, 1);

	Lump_c *lump = wad->AddLump(lump_name);

	// FIXME: SaveLump

	return true;
}


void UI_TextEditor::UpdateStatus()
{
	int row, column;

	if (ted->GetLineAndColumn(&row, &column))
		status->SetPosition(row, column);

	status->SetModified(has_changes);
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
