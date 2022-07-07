//------------------------------------------------------------------------
//  LEVEL CUT 'N' PASTE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2009-2019 Andrew Apted
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

#ifndef __EUREKA_E_CUTPASTE_H__
#define __EUREKA_E_CUTPASTE_H__

#include "objid.h"

class EditOperation;
class selection_c;
class SString;
class StringID;
struct ConfigData;
struct Document;

//
// Edit menu command
//
enum class EditCommand
{
	cut,
	copy,
	paste,
	del
};

void Clipboard_ClearLocals();

void Clipboard_NotifyBegin();
void Clipboard_NotifyInsert(const Document &doc, ObjType type, int objnum);
void Clipboard_NotifyDelete(ObjType type, int objnum);
void Clipboard_NotifyChange(ObjType type, int objnum, int field);
void Clipboard_NotifyEnd();

void UnusedVertices(const Document &doc, const selection_c &lines, selection_c &result);
void UnusedSideDefs(const Document &doc, const selection_c &lines, const selection_c *secs, selection_c &result);

void DeleteObjects_WithUnused(EditOperation &op, const Document &doc, const selection_c &list,
			bool keep_things,
			bool keep_verts ,
			bool keep_lines );

//----------------------------------------------------------------------
//  Texture Clipboard
//----------------------------------------------------------------------

void Texboard_Clear();

StringID Texboard_GetFlatNum(const ConfigData &config);
StringID Texboard_GetTexNum(const ConfigData &config);
int Texboard_GetThing(const ConfigData &config);
void Texboard_SetFlat(const SString &new_flat, const ConfigData &config);
void Texboard_SetTex(const SString &new_tex, const ConfigData &config);
void Texboard_SetThing(int new_id);

#endif  /* __EUREKA_E_CUTPASTE_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
