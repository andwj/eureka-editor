//------------------------------------------------------------------------
//  OBJECT IDENTIFICATION
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
//  Copyright (C) 1997-2003 André Majorel et al
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
//
//  Based on Yadex which incorporated code from DEU 5.21 that was put
//  in the public domain in 1994 by Raphaël Quinet and Brendon Wyber.
//
//------------------------------------------------------------------------

#ifndef __EUREKA_OBJ_ID_H__
#define __EUREKA_OBJ_ID_H__


// main kinds of objects
typedef enum
{
	OBJ_THINGS,
	OBJ_LINEDEFS,
	OBJ_SIDEDEFS,
	OBJ_VERTICES,
	OBJ_SECTORS,
}
obj_type_e;


// special object number for "NONE"
#define NIL_OBJ		-1


// bit flags for object parts (bit zero is reserved)
#define PART_FLOOR		0x02
#define PART_CEIL		0x04
#define PART_SEC_ALL	(PART_FLOOR + PART_CEIL)

#define PART_RT_LOWER		0x02
#define PART_RT_UPPER		0x04
#define PART_RT_RAIL		0x08
#define PART_RT_ALL			(PART_RT_LOWER | PART_RT_UPPER | PART_RT_RAIL)

#define PART_LF_LOWER		0x20
#define PART_LF_UPPER		0x40
#define PART_LF_RAIL		0x80
#define PART_LF_ALL			(PART_LF_LOWER | PART_LF_UPPER | PART_LF_RAIL)


class Objid
{
public:
	obj_type_e type;

	int num;

	// this is some combination of PART_XXX flags, or 0 which
	// represents the object as a whole.
	int parts;

public:
	Objid() : type(OBJ_THINGS), num(NIL_OBJ), parts(0) { }
	Objid(obj_type_e t, int n) : type(t), num(n), parts(0) { }
	Objid(obj_type_e t, int n, int p) : type(t), num(n), parts(p) { }
	Objid(const Objid& other) : type(other.type), num(other.num), parts(other.parts) { }

	void clear()
	{
		num = NIL_OBJ;
		parts = 0;
	}

	bool operator== (const Objid& other) const
	{
		return (other.type == type) && (other.num == num);
	}

	bool operator!= (const Objid& other) const
	{
		return (other.type != type) || (other.num != num);
	}

	bool valid()  const { return num >= 0; }
	bool is_nil() const { return num <  0; }
};


#endif  /* __EUREKA_OBJ_ID_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
