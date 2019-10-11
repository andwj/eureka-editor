//------------------------------------------------------------------------
//  3D RENDERING : SOFTWARE MODE
//------------------------------------------------------------------------
//
//  Eureka DOOM Editor
//
//  Copyright (C) 2001-2019 Andrew Apted
//  Copyright (C) 1997-2003 Andr� Majorel et al
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

#include <map>
#include <algorithm>

#ifndef NO_OPENGL
#include "FL/gl.h"
#endif

#include "im_color.h"
#include "im_img.h"
#include "e_hover.h"
#include "e_main.h"
#include "m_game.h"
#include "w_rawdef.h"
#include "w_texture.h"
#include "r_render.h"


extern rgb_color_t transparent_col;

extern bool render_high_detail;
extern bool render_lock_gravity;
extern bool render_missing_bright;
extern bool render_unknown_bright;


static img_pixel_t DoomLightRemap(int light, float dist, img_pixel_t pixel)
{
	int map = R_DoomLightingEquation(light, dist);

	if (pixel & IS_RGB_PIXEL)
	{
		map = (map ^ 31) + 1;

		int r = IMG_PIXEL_RED(pixel);
		int g = IMG_PIXEL_GREEN(pixel);
		int b = IMG_PIXEL_BLUE(pixel);

		r = (r * map) >> 5;
		g = (g * map) >> 5;
		b = (b * map) >> 5;

		return IMG_PIXEL_MAKE_RGB(r, g, b);
	}
	else
	{
		return raw_colormap[map][pixel];
	}
}


struct DrawSurf
{
public:
	enum
	{
		K_INVIS = 0,
		K_FLAT,
		K_TEXTURE
	};
	int kind;

	// heights for the surface (h1 is below h2)
	int h1, h2, tex_h;

	Img_c *img;
	img_pixel_t col;  /* used when no image */

	enum
	{
		SOLID_ABOVE = 1,
		SOLID_BELOW = 2
	};
	int y_clip;

	bool fullbright;

public:
	DrawSurf() : kind(K_INVIS), h1(), h2(), tex_h(),
				 img(NULL), col(), y_clip(),
				 fullbright(false)
	{ }

	~DrawSurf()
	{ }

	void FindFlat(const char * fname, Sector *sec)
	{
		fullbright = false;

		if (is_sky(fname))
		{
			col = game_info.sky_color;
			fullbright = true;
			return;
		}

		if (r_view.texturing)
		{
			img = W_GetFlat(fname);

			if (! img)
			{
				img = IM_UnknownFlat();
				fullbright = render_unknown_bright;
			}

			return;
		}

		// when lighting and no texturing, use a single color
		if (r_view.lighting)
			col = game_info.floor_colors[1];
		else
			col = HashedPalColor(fname, game_info.floor_colors);
	}

	void FindTex(const char * tname, LineDef *ld)
	{
		fullbright = false;

		if (r_view.texturing)
		{
			if (is_null_tex(tname))
			{
				img = IM_MissingTex();
				fullbright = render_missing_bright;
				return;
			}

			img = W_GetTexture(tname);

			if (! img)
			{
				img = IM_UnknownTex();
				fullbright = render_unknown_bright;
			}

			return;
		}

		// when lighting and no texturing, use a single color
		if (r_view.lighting)
			col = game_info.wall_colors[1];
		else
			col = HashedPalColor(tname, game_info.wall_colors);
	}
};


struct DrawWall
{
public:
	typedef std::vector<struct DrawWall *> vec_t;

	// when 'th' is >= 0, this is actually a sprite, and 'ld' and
	// 'sd' will be NULL.  Sprites use the info in the 'ceil' surface.
	int th;

	LineDef *ld;
	SideDef *sd;
	Sector *sec;

	// which side this wall faces (SIDE_LEFT or SIDE_RIGHT)
	// for sprites: a copy of the thinginfo flags
	int side;

	// lighting for wall, adjusted for N/S and E/W walls
	int wall_light;

	// line constants
	float delta_ang;
	float dist, t_dist;
	float normal;	// scale for things

	// distance values (inverted, so they can be lerped)
	double iz1, iz2;
	double diz, cur_iz;
	double mid_iz;

	// translate coord, for sprite
	float spr_tx1;

	// screen X coordinates
	int sx1, sx2;

	// for sprites, the remembered open space to clip to
	int oy1, oy2;

	/* surfaces */

	DrawSurf ceil;
	DrawSurf upper;
	DrawSurf lower;
	DrawSurf floor;
	DrawSurf rail;

	// IsCloser tests if THIS wall (wall A) is closer to the camera
	// than the given wall (wall B).
	//
	// Note that it is NOT suitable as a predicate for std::sort()
	// since it does not guarantee a linear order (total order) of
	// the elements.  Hence the need for our own sorting code.

	inline bool IsCloser(const DrawWall *const B) const
	{
		const DrawWall *const A = this;

		if (A == B)
			return false;

		if (A->ld && B->ld)
		{
			// handle cases where two linedefs share a vertex, since that
			// is where slime-trails would otherwise occur.

			// if they do share a vertex, we check if the other vertex of
			// wall A and the camera position are both on the same side of
			// wall B (extended to infinity).

			int A_other = -1;

			if (B->ld->TouchesVertex(A->ld->start))
				A_other = A->ld->end;
			else if (B->ld->TouchesVertex(A->ld->end))
				A_other = A->ld->start;

			if (A_other >= 0)
			{
				int ax = Vertices[A_other]->x;
				int ay = Vertices[A_other]->y;

				int bx1 = B->ld->Start()->x;
				int by1 = B->ld->Start()->y;
				int bx2 = B->ld->End()->x;
				int by2 = B->ld->End()->y;

				int cx = (int)r_view.x;  // camera
				int cy = (int)r_view.y;

				int A_side = PointOnLineSide(ax, ay, bx1, by1, bx2, by2);
				int C_side = PointOnLineSide(cx, cy, bx1, by1, bx2, by2);

				return (A_side * C_side >= 0);
			}
		}
		else if (A->th >= 0 && B->th >= 0)
		{
			// prevent two things at same location from flickering
			const Thing *const TA = Things[A->th];
			const Thing *const TB = Things[B->th];

			if (TA->x == TB->x && TA->y == TB->y)
				return A->th > B->th;
		}

		return A->cur_iz > B->cur_iz;
	}

	/* PREDICATES */

	struct MidDistCmp
	{
		inline bool operator() (const DrawWall * A, const DrawWall * B) const
		{
			return A->mid_iz > B->mid_iz;
		}
	};

	struct SX1Cmp
	{
		inline bool operator() (const DrawWall * A, const DrawWall * B) const
		{
			return A->sx1 < B->sx1;
		}

		inline bool operator() (const DrawWall * A, int x) const
		{
			return A->sx1 < x;
		}

		inline bool operator() (int x, const DrawWall * A) const
		{
			return x < A->sx1;
		}
	};

	struct SX2Less
	{
		int x;

		SX2Less(int _x) : x(_x) { }

		inline bool operator() (const DrawWall * A) const
		{
			return A->sx2 < x;
		}
	};

	/* methods */

	void ComputeWallSurface()
	{
		Sector *front = sec;
		Sector *back  = NULL;

		SideDef *back_sd = (side == SIDE_LEFT) ? ld->Right() : ld->Left();
		if (back_sd)
			back = Sectors[back_sd->sector];

		bool sky_upper = back && is_sky(front->CeilTex()) && is_sky(back->CeilTex());
		bool self_ref  = (front == back) ? true : false;

		if ((front->ceilh > r_view.z || is_sky(front->CeilTex()))
		    && ! sky_upper && ! self_ref)
		{
			ceil.kind = DrawSurf::K_FLAT;
			ceil.h1 = front->ceilh;
			ceil.h2 = +99999;
			ceil.tex_h = ceil.h1;
			ceil.y_clip = DrawSurf::SOLID_ABOVE;

			ceil.FindFlat(front->CeilTex(), front);
		}

		if (front->floorh < r_view.z && ! self_ref)
		{
			floor.kind = DrawSurf::K_FLAT;
			floor.h1 = -99999;
			floor.h2 = front->floorh;
			floor.tex_h = floor.h2;
			floor.y_clip = DrawSurf::SOLID_BELOW;

			floor.FindFlat(front->FloorTex(), front);
		}

		if (! back)
		{
			/* ONE-sided line */

			lower.kind = DrawSurf::K_TEXTURE;
			lower.h1 = front->floorh;
			lower.h2 = front->ceilh;
			lower.y_clip = DrawSurf::SOLID_ABOVE | DrawSurf::SOLID_BELOW;

			lower.FindTex(sd->MidTex(), ld);

			if (lower.img && (ld->flags & MLF_LowerUnpegged))
				lower.tex_h = lower.h1 + lower.img->height();
			else
				lower.tex_h = lower.h2;

			lower.tex_h += sd->y_offset;
			return;
		}

		/* TWO-sided line */

		if (back->ceilh < front->ceilh && ! sky_upper && ! self_ref)
		{
			upper.kind = DrawSurf::K_TEXTURE;
			upper.h1 = back->ceilh;
			upper.h2 = front->ceilh;
			upper.y_clip = DrawSurf::SOLID_ABOVE;

			upper.FindTex(sd->UpperTex(), ld);

			if (upper.img && ! (ld->flags & MLF_UpperUnpegged))
				upper.tex_h = upper.h1 + upper.img->height();
			else
				upper.tex_h = upper.h2;

			upper.tex_h += sd->y_offset;
		}

		if (back->floorh > front->floorh && ! self_ref)
		{
			lower.kind = DrawSurf::K_TEXTURE;
			lower.h1 = front->floorh;
			lower.h2 = back->floorh;
			lower.y_clip = DrawSurf::SOLID_BELOW;

			lower.FindTex(sd->LowerTex(), ld);

			// note "sky_upper" here, needed to match original DOOM behavior
			if (ld->flags & MLF_LowerUnpegged)
				lower.tex_h = sky_upper ? back->ceilh : front->ceilh;
			else
				lower.tex_h = lower.h2;

			lower.tex_h += sd->y_offset;
		}

		/* Mid-Masked texture */

		if (! r_view.texturing)
			return;

		if (is_null_tex(sd->MidTex()))
			return;

		rail.FindTex(sd->MidTex(), ld);
		if (! rail.img)
			return;

		int c_h = MIN(front->ceilh,  back->ceilh);
		int f_h = MAX(front->floorh, back->floorh);
		int r_h = rail.img->height();

		if (f_h >= c_h)
			return;

		if (ld->flags & MLF_LowerUnpegged)
		{
			rail.h1 = f_h + sd->y_offset;
			rail.h2 = rail.h1 + r_h;
		}
		else
		{
			rail.h2 = c_h + sd->y_offset;
			rail.h1 = rail.h2 - r_h;
		}

		rail.kind = DrawSurf::K_TEXTURE;
		rail.y_clip = 0;
		rail.tex_h = rail.h2;

		// clip railing, unless sectors on both sides are identical or
		// we have a sky upper

		if (! (sky_upper ||
				(back->ceilh == front->ceilh &&
				 back->ceil_tex == front->ceil_tex &&
				 back->light == front->light)))
		{
			rail.h2 = MIN(c_h, rail.h2);
		}

		if (! (back->floorh == front->floorh &&
			   back->floor_tex == front->floor_tex &&
			   back->light == front->light))
		{
			rail.h1 = MAX(f_h, rail.h1);
		}
	}
};


struct RendInfo
{
public:
	// complete set of walls/sprites to draw.
	DrawWall::vec_t walls;

	// the active list.  Pointers here are always duplicates of ones in
	// the walls list (no need to 'delete' any of them).
	DrawWall::vec_t active;

	// query state
	int query_mode;  // 0 for normal render
	int query_sx;
	int query_sy;

	DrawWall     *query_wall;  // the hit wall
	obj3d_type_e  query_part;  // the part of the hit wall

	// inverse distances over X range, 0 when empty.
	std::vector<double> depth_x;

	// vertical clip window, an inclusive range
	int open_y1;
	int open_y2;

	// these used by Highlight()
	int hl_ox, hl_oy;

private:
	static void DeleteWall(DrawWall *P)
	{
		delete P;
	}

public:
	RendInfo() :
		walls(), active(),
		query_mode(0), query_sx(), query_sy(),
		depth_x(), open_y1(), open_y2()
	{ }

	~RendInfo()
	{
		std::for_each(walls.begin(), walls.end(), DeleteWall);

		walls.clear ();
		active.clear ();
	}

	void InitDepthBuf (int width)
	{
		depth_x.resize(width);

		std::fill_n(depth_x.begin(), width, 0);
	}

	void AddHighlightLine(int sx1, int sy1, int sx2, int sy2, short thick, Fl_Color color)
	{
		if (! render_high_detail)
		{
			sx1 *= 2;  sy1 *= 2;
			sx2 *= 2;  sy2 *= 2;
		}

		fl_color(color);

		if (thick)
			fl_line_style(FL_SOLID, 2);

		fl_line(hl_ox + sx1, hl_oy + sy1, hl_ox + sx2, hl_oy + sy2);

		if (thick)
			fl_line_style(0);
	}

	void AddHighlightLine(int sx1, int sy1, int sx2, int sy2, int sel_mode)
	{
		AddHighlightLine(sx1, sy1, sx2, sy2,
				(sel_mode & 1) ? 2 : 0,
				(sel_mode == 3) ? HI_AND_SEL_COL :
				(sel_mode == 2) ? HI_COL : SEL_COL);
	}

	static inline float PointToAngle(float x, float y)
	{
		if (-0.01 < x && x < 0.01)
			return (y > 0) ? M_PI/2 : (3 * M_PI/2);

		float angle = atan2(y, x);

		if (angle < 0)
			angle += 2*M_PI;

		return angle;
	}

	static inline int AngleToX(float ang)
	{
		float t = tan(M_PI/2 - ang);

		int x = int(r_view.aspect_sw * t);

		x = (r_view.screen_w + x) / 2;

		if (x < 0)
			x = 0;
		else if (x > r_view.screen_w)
			x = r_view.screen_w;

		return x;
	}

	static inline float XToAngle(int x)
	{
		x = x * 2 - r_view.screen_w;

		float ang = M_PI/2 + atan(x / r_view.aspect_sw);

		if (ang < 0)
			ang = 0;
		else if (ang > M_PI)
			ang = M_PI;

		return ang;
	}

	static inline int DeltaToX(double iz, float tx)
	{
		int x = int(r_view.aspect_sw * tx * iz);

		x = (x + r_view.screen_w) / 2;

		return x;
	}

	static inline float XToDelta(int x, double iz)
	{
		x = x * 2 - r_view.screen_w;

		float tx = x / iz / r_view.aspect_sw;

		return tx;
	}

	static inline int DistToY(double iz, int sec_h)
	{
		if (sec_h > 32770)
			return -9999;

		if (sec_h < -32770)
			return +9999;

		int y = int(r_view.aspect_sh * (sec_h - r_view.z) * iz);

		return (r_view.screen_h - y) / 2;
	}

	static inline float YToDist(int y, int sec_h)
	{
		y = r_view.screen_h - y * 2;

		if (y == 0)
			return 999999;

		return r_view.aspect_sh * (sec_h - r_view.z) / y;
	}

	static inline float YToSecH(int y, double iz)
	{
		y = y * 2 - r_view.screen_h;

		return r_view.z - (float(y) / r_view.aspect_sh / iz);
	}

	void AddLine(int ld_index)
	{
		LineDef *ld = LineDefs[ld_index];

		if (! is_vertex(ld->start) || ! is_vertex(ld->end))
			return;

		if (! ld->Right())
			return;

		float x1 = ld->Start()->x - r_view.x;
		float y1 = ld->Start()->y - r_view.y;
		float x2 = ld->End()->x - r_view.x;
		float y2 = ld->End()->y - r_view.y;

		float tx1 = x1 * r_view.Sin - y1 * r_view.Cos;
		float ty1 = x1 * r_view.Cos + y1 * r_view.Sin;
		float tx2 = x2 * r_view.Sin - y2 * r_view.Cos;
		float ty2 = x2 * r_view.Cos + y2 * r_view.Sin;

		// reject line if complete behind viewplane
		if (ty1 <= 0 && ty2 <= 0)
			return;

		float angle1 = PointToAngle(tx1, ty1);
		float angle2 = PointToAngle(tx2, ty2);
		float span = angle1 - angle2;

		if (span < 0)
			span += 2*M_PI;

		int side = SIDE_RIGHT;

		if (span >= M_PI)
			side = SIDE_LEFT;

		// ignore the line when there is no facing sidedef
		SideDef *sd = (side == SIDE_LEFT) ? ld->Left() : ld->Right();

		if (! sd)
			return;

		if (side == SIDE_LEFT)
		{
			float tmp = angle1;
			angle1 = angle2;
			angle2 = tmp;
		}

		// clip angles to view volume

		float base_ang = angle1;

		float leftclip  = (3 * M_PI / 4);
		float rightclip = M_PI / 4;

		float tspan1 = angle1 - rightclip;
		float tspan2 = leftclip - angle2;

		if (tspan1 < 0) tspan1 += 2*M_PI;
		if (tspan2 < 0) tspan2 += 2*M_PI;

		if (tspan1 > M_PI/2)
		{
			// Totally off the left edge?
			if (tspan2 >= M_PI)
				return;

			angle1 = leftclip;
		}

		if (tspan2 > M_PI/2)
		{
			// Totally off the left edge?
			if (tspan1 >= M_PI)
				return;

			angle2 = rightclip;
		}

		// convert angles to on-screen X positions
		int sx1 = AngleToX(angle1);
		int sx2 = AngleToX(angle2) - 1;

		if (sx1 > sx2)
			return;

		// optimisation for query mode
		if (query_mode && (sx2 < query_sx || sx1 > query_sx))
			return;

		// compute distance from eye to wall
		float wdx = x2 - x1;
		float wdy = y2 - y1;

		float wlen = sqrt(wdx * wdx + wdy * wdy);
		float dist = fabs((y1 * wdx / wlen) - (x1 * wdy / wlen));

		if (dist < 0.01)
			return;

		// compute normal of wall (translated coords)
		float normal;

		if (side == SIDE_LEFT)
			normal = PointToAngle(ty2 - ty1, tx1 - tx2);
		else
			normal = PointToAngle(ty1 - ty2, tx2 - tx1);

		// compute inverse distances
		double iz1 = cos(normal - angle1) / dist / cos(M_PI/2 - angle1);
		double iz2 = cos(normal - angle2) / dist / cos(M_PI/2 - angle2);

		double diz = (iz2 - iz1) / MAX(1, sx2 - sx1);

		// create drawwall structure

		DrawWall *dw = new DrawWall;

		dw->th = -1;
		dw->ld = ld;
		dw->sd = sd;
		dw->sec = sd->SecRef();

		dw->side = side;

		dw->wall_light = dw->sec->light;

		// add "fake constrast" for axis-aligned walls
		if (ld->Start()->x == ld->End()->x)
			dw->wall_light += 16;
		else if (ld->Start()->y == ld->End()->y)
			dw->wall_light -= 16;

		dw->delta_ang = angle1 + XToAngle(sx1) - normal;

		dw->dist = dist;
		dw->normal = normal;
		dw->t_dist = tan(base_ang - normal) * dist;

		dw->iz1 = iz1;
		dw->iz2 = iz2;
		dw->diz = diz;
		dw->mid_iz = iz1 + (sx2 - sx1 + 1) * diz / 2;

		dw->sx1 = sx1;
		dw->sx2 = sx2;

		walls.push_back(dw);
	}

	void AddThing(int th_index)
	{
		Thing *th = Things[th_index];

		const thingtype_t *info = M_GetThingType(th->type);

		float x = th->x - r_view.x;
		float y = th->y - r_view.y;

		float tx = x * r_view.Sin - y * r_view.Cos;
		float ty = x * r_view.Cos + y * r_view.Sin;

		// reject sprite if complete behind viewplane
		if (ty < 4)
			return;

		bool is_unknown = false;

		float scale = info->scale;

		Img_c *sprite = W_GetSprite(th->type);
		if (! sprite)
		{
			sprite = IM_UnknownSprite();
			is_unknown = true;
		}

		float tx1 = tx - sprite->width() * scale / 2.0;
		float tx2 = tx + sprite->width() * scale / 2.0;

		double iz = 1 / ty;

		int sx1 = DeltaToX(iz, tx1);
		int sx2 = DeltaToX(iz, tx2) - 1;

		if (sx1 < 0)
			sx1 = 0;

		if (sx2 >= r_view.screen_w)
			sx2 = r_view.screen_w - 1;

		if (sx1 > sx2)
			return;

		// optimisation for query mode
		if (query_mode && (sx2 < query_sx || sx1 > query_sx))
			return;

		int thsec = r_view.thing_sectors[th_index];

		int h1, h2;

		if (info && (info->flags & THINGDEF_CEIL))
		{
			// IOANCH 9/2015: also add z
			h2 = (is_sector(thsec) ? Sectors[thsec]->ceilh : 192) - th->z;
			h1 = h2 - sprite->height() * scale;
		}
		else
		{
			h1 = (is_sector(thsec) ? Sectors[thsec]->floorh : 0) + th->z;
			h2 = h1 + sprite->height() * scale;
		}

		// create drawwall structure

		DrawWall *dw = new DrawWall;

		dw->th  = th_index;
		dw->ld  = NULL;
		dw->sd  = NULL;
		dw->sec = NULL;

		dw->side = info ? info->flags : 0;

		if (is_unknown && render_unknown_bright)
			dw->side |= THINGDEF_LIT;
		else if (r_view.hl.isThing() && th_index == r_view.hl.num)
			dw->side |= THINGDEF_LIT;

		dw->spr_tx1 = tx1;

		dw->normal = scale;

		dw->iz1 = dw->mid_iz = iz;
		dw->diz = 0;

		dw->sx1 = sx1;
		dw->sx2 = sx2;

		dw->ceil.img = sprite;
		dw->ceil.h1  = h1;
		dw->ceil.h2  = h2;

		walls.push_back(dw);
	}

	void ComputeSurfaces()
	{
		DrawWall::vec_t::iterator S;

		for (S = walls.begin() ; S != walls.end() ; S++)
		{
			if ((*S)->ld)
				(*S)->ComputeWallSurface();
		}
	}

	void Highlight_WallPart(obj3d_type_e part, const DrawWall *dw,
							int sel_mode)
	{
		int h1, h2;

		if (! dw->ld->TwoSided())
		{
			h1 = dw->sd->SecRef()->floorh;
			h2 = dw->sd->SecRef()->ceilh;
		}
		else
		{
			const Sector *front = dw->ld->Right()->SecRef();
			const Sector *back  = dw->ld-> Left()->SecRef();

			if (part == OB3D_Lower)
			{
				h1 = MIN(front->floorh, back->floorh);
				h2 = MAX(front->floorh, back->floorh);
			}
			else  /* part == OB3D_Upper */
			{
				h1 = MIN(front->ceilh, back->ceilh);
				h2 = MAX(front->ceilh, back->ceilh);
			}
		}

		int x1 = dw->sx1 - 1;
		int x2 = dw->sx2 + 1;

		int ly1 = DistToY(dw->iz1, h2);
		int ly2 = DistToY(dw->iz1, h1);

		int ry1 = DistToY(dw->iz2, h2);
		int ry2 = DistToY(dw->iz2, h1);

		// workaround for crappy line clipping in X windows
		if (ly1 < -5000 || ly2 < -5000 || ly1 >  5000 || ly2 >  5000 ||
			ry1 < -5000 || ry2 < -5000 || ry1 >  5000 || ry2 >  5000)
			return;

		// keep the lines thin, makes aligning textures easier
		AddHighlightLine(x1, ly1, x1, ly2, sel_mode);
		AddHighlightLine(x2, ry1, x2, ry2, sel_mode);
		AddHighlightLine(x1, ly1, x2, ry1, sel_mode);
		AddHighlightLine(x1, ly2, x2, ry2, sel_mode);
	}

	void Highlight_Line(obj3d_type_e part, int ld, int side, int sel_mode)
	{
		const LineDef *L = LineDefs[ld];

		DrawWall::vec_t::iterator S;

		for (S = walls.begin() ; S != walls.end() ; S++)
		{
			const DrawWall *dw = (*S);

			if (dw->ld == L && dw->side == side)
				Highlight_WallPart(part, dw, sel_mode);
		}
	}

	void Highlight_Sector(obj3d_type_e part, int sec_num, int sel_mode)
	{
		int sec_h;

		if (part == OB3D_Floor)
		{
			sec_h = Sectors[sec_num]->floorh;

			if (sec_h >= r_view.z)
				return;
		}
		else  /* OB3D_Ceil */
		{
			sec_h = Sectors[sec_num]->ceilh;

			if (sec_h <= r_view.z)
				return;
		}

		DrawWall::vec_t::iterator S;

		for (S = walls.begin() ; S != walls.end() ; S++)
		{
			const DrawWall *dw = (*S);

			if (dw->ld && dw->ld->TouchesSector(sec_num))
			{
				int sy1 = DistToY(dw->iz1, sec_h);
				int sy2 = DistToY(dw->iz2, sec_h);

				// workaround for crappy line clipping in X windows
				if (sy1 < -5000 || sy2 < -5000 ||
					sy1 >  5000 || sy2 >  5000)
					continue;

				AddHighlightLine(dw->sx1, sy1, dw->sx2, sy2, sel_mode);
			}
		}
	}

	void Highlight_Thing(int th, int sel_mode)
	{
		DrawWall::vec_t::iterator S;

		for (S = walls.begin() ; S != walls.end() ; S++)
		{
			const DrawWall *dw = (*S);

			if (! (dw->th >= 0 && dw->th == th))
				continue;

			int h1 = dw->ceil.h1 - 1;
			int h2 = dw->ceil.h2 + 1;

			int x1 = dw->sx1 - 1;
			int x2 = dw->sx2 + 1;

			int y1 = DistToY(dw->iz1, h2);
			int y2 = DistToY(dw->iz1, h1);

			AddHighlightLine(x1, y1, x1, y2, sel_mode);
			AddHighlightLine(x2, y1, x2, y2, sel_mode);
			AddHighlightLine(x1, y1, x2, y1, sel_mode);
			AddHighlightLine(x1, y2, x2, y2, sel_mode);
			break;
		}
	}

	inline void Highlight_Object(Obj3d_t& obj, int sel_mode)
	{
		if (obj.isThing())
		{
			Highlight_Thing(obj.num, sel_mode);
		}
		else if (obj.isSector())
		{
			Highlight_Sector(obj.type, obj.num, sel_mode);
		}
		else if (obj.isLine())
		{
			Highlight_Line(obj.type, obj.num, obj.side, sel_mode);
		}
	}

	void Highlight(int ox, int oy)
	{
		hl_ox = ox;
		hl_oy = oy;

		/* do the selection */

		bool saw_hl = false;

		for (unsigned int k = 0 ; k < r_view.sel.size() ; k++)
		{
			if (! r_view.sel[k].valid())
				continue;

			int sel_mode = 1;

			if (r_view.hl.valid() && r_view.hl == r_view.sel[k])
			{
				sel_mode |= 2;
				saw_hl = true;
			}

			Highlight_Object(r_view.sel[k], sel_mode);
		}

		/* do the highlight */

		if (! saw_hl)
			Highlight_Object(r_view.hl, 2);
	}

	void ClipSolids()
	{
		// perform a rough depth sort of the walls and sprites.

		std::sort(walls.begin(), walls.end(), DrawWall::MidDistCmp());

		// go forwards, from closest to furthest away

		DrawWall::vec_t::iterator S;

		for (S = walls.begin() ; S != walls.end() ; S++)
		{
			DrawWall *dw = (*S);

			if (! dw)
				continue;

			int one_sided = dw->ld && ! dw->ld->Left();
			int vis_count = dw->sx2 - dw->sx1 + 1;

			for (int x = dw->sx1 ; x <= dw->sx2 ; x++)
			{
				double iz = dw->iz1 + (dw->diz * (x - dw->sx1));

				if (iz < depth_x[x])
					vis_count--;
				else if (one_sided)
					depth_x[x] = iz;
			}

			if (vis_count == 0)
			{
				delete dw;
				(*S) = NULL;
			}
		}

		// remove null pointers

		S = std::remove(walls.begin(), walls.end(), (DrawWall *) NULL);

		walls.erase(S, walls.end());
	}

	void RenderFlatColumn(DrawWall *dw, DrawSurf& surf,
			int x, int y1, int y2)
	{
		img_pixel_t *dest = r_view.screen;

		const img_pixel_t *src = surf.img->buf();

		int tw = surf.img->width();
		int th = surf.img->height();

		float ang = XToAngle(x);
		float modv = cos(ang - M_PI/2);

		float t_cos = cos(M_PI + -r_view.angle + ang) / modv;
		float t_sin = sin(M_PI + -r_view.angle + ang) / modv;

		dest += x + y1 * r_view.screen_w;

		int light = dw->sec->light;

		for ( ; y1 <= y2 ; y1++, dest += r_view.screen_w)
		{
			float dist = YToDist(y1, surf.tex_h);

			int tx = int( r_view.x - t_sin * dist) & (tw - 1);
			int ty = int(-r_view.y + t_cos * dist) & (th - 1);

			*dest = src[ty * tw + tx];

			if (r_view.lighting && ! surf.fullbright)
				*dest = DoomLightRemap(light, dist, *dest);
		}
	}

	void RenderTexColumn(DrawWall *dw, DrawSurf& surf,
			int x, int y1, int y2)
	{
		img_pixel_t *dest = r_view.screen;

		const img_pixel_t *src = surf.img->buf();

		int tw = surf.img->width();
		int th = surf.img->height();

		int  light = dw->wall_light;
		float dist = 1.0 / dw->cur_iz;

		/* compute texture X coord */

		float cur_ang = dw->delta_ang - XToAngle(x);

		int tx = int(dw->t_dist - tan(cur_ang) * dw->dist);

		tx = (dw->sd->x_offset + tx) & (tw - 1);

		/* compute texture Y coords */

		float hh = surf.tex_h - YToSecH(y1, dw->cur_iz);
		float dh = surf.tex_h - YToSecH(y2, dw->cur_iz);

		dh = (dh - hh) / MAX(1, y2 - y1);
		hh += 0.2;

		src  += tx;
		dest += x + y1 * r_view.screen_w;

		for ( ; y1 <= y2 ; y1++, hh += dh, dest += r_view.screen_w)
		{
			int ty = int(floor(hh)) % th;

			// handle negative values (use % twice)
			ty = (ty + th) % th;

			img_pixel_t pix = src[ty * tw];

			if (pix == TRANS_PIXEL)
				continue;

			if (r_view.lighting && ! surf.fullbright)
				*dest = DoomLightRemap(light, dist, pix);
			else
				*dest = pix;
		}
	}

	void SolidFlatColumn(DrawWall *dw, DrawSurf& surf, int x, int y1, int y2)
	{
		img_pixel_t *dest = r_view.screen;

		dest += x + y1 * r_view.screen_w;

		int light = dw->sec->light;

		for ( ; y1 <= y2 ; y1++, dest += r_view.screen_w)
		{
			float dist = YToDist(y1, surf.tex_h);

			if (r_view.lighting && ! surf.fullbright)
				*dest = DoomLightRemap(light, dist, surf.col);
			else
				*dest = surf.col;
		}
	}

	void SolidTexColumn(DrawWall *dw, DrawSurf& surf, int x, int y1, int y2)
	{
		int  light = dw->wall_light;
		float dist = 1.0 / dw->cur_iz;

		img_pixel_t *dest = r_view.screen;

		dest += x + y1 * r_view.screen_w;

		for ( ; y1 <= y2 ; y1++, dest += r_view.screen_w)
		{
			if (r_view.lighting && ! surf.fullbright)
				*dest = DoomLightRemap(light, dist, surf.col);
			else
				*dest = surf.col;
		}
	}

	inline void RenderWallSurface(DrawWall *dw, DrawSurf& surf, int x,
								  obj3d_type_e part)
	{
		if (surf.kind == DrawSurf::K_INVIS)
			return;

		int y1 = DistToY(dw->cur_iz, surf.h2);
		int y2 = DistToY(dw->cur_iz, surf.h1) - 1;

		// clip to the open region
		if (y1 < open_y1)
			y1 = open_y1;

		if (y2 > open_y2)
			y2 = open_y2;

		// update open region based on ends which are "solid"
		if (surf.y_clip & DrawSurf::SOLID_ABOVE)
			open_y1 = MAX(open_y1, y2 + 1);

		if (surf.y_clip & DrawSurf::SOLID_BELOW)
			open_y2 = MIN(open_y2, y1 - 1);

		if (y1 > y2)
			return;

		/* query mode : is mouse over this wall part? */

		if (query_mode)
		{
			if (y1 <= query_sy && query_sy <= y2)
			{
				query_wall = dw;
				query_part = part;
			}
			return;
		}

		/* fill pixels */

		if (! surf.img)
		{
			if (surf.kind == DrawSurf::K_FLAT)
				SolidFlatColumn(dw, surf, x, y1, y2);
			else
				SolidTexColumn(dw, surf, x, y1, y2);
		}
		else switch (surf.kind)
		{
			case DrawSurf::K_FLAT:
				RenderFlatColumn(dw, surf, x, y1, y2);
				break;

			case DrawSurf::K_TEXTURE:
				RenderTexColumn(dw, surf, x, y1, y2);
				break;
		}
	}

	inline void RenderSprite(DrawWall *dw, int x)
	{
		int y1 = DistToY(dw->cur_iz, dw->ceil.h2);
		int y2 = DistToY(dw->cur_iz, dw->ceil.h1) - 1;

		if (y1 < dw->oy1)
			y1 = dw->oy1;

		if (y2 > dw->oy2)
			y2 = dw->oy2;

		if (y1 > y2)
			return;

		if (query_mode)
		{
			if (y1 <= query_sy && query_sy <= y2)
			{
				query_wall = dw;
				query_part = OB3D_Thing;
			}
			return;
		}

		int tw = dw->ceil.img->width();
		int th = dw->ceil.img->height();

		float scale = dw->normal;

		int tx = int((XToDelta(x, dw->cur_iz) - dw->spr_tx1) / scale);

		if (tx < 0 || tx >= tw)
			return;

		float hh = dw->ceil.h2 - YToSecH(y1, dw->cur_iz);
		float dh = dw->ceil.h2 - YToSecH(y2, dw->cur_iz);

		dh = (dh - hh) / MAX(1, y2 - y1);

		int thsec = r_view.thing_sectors[dw->th];
		int light = is_sector(thsec) ? Sectors[thsec]->light : 255;
		float dist = 1.0 / dw->cur_iz;

		/* fill pixels */

		img_pixel_t *dest = r_view.screen;
		dest += x + y1 * r_view.screen_w;

		const img_pixel_t *src = dw->ceil.img->buf();
		src += tx;

		for ( ; y1 <= y2 ; y1++, hh += dh, dest += r_view.screen_w)
		{
			int ty = int(hh / scale);

			if (ty < 0 || ty >= th)
				continue;

			img_pixel_t pix = src[ty * tw];

			if (pix == TRANS_PIXEL)
				continue;

			if (dw->side & THINGDEF_INVIS)
			{
				if (*dest & IS_RGB_PIXEL)
					*dest = IS_RGB_PIXEL | ((*dest & 0x7bde) >> 1);
				else
					*dest = raw_colormap[14][*dest];
				continue;
			}

			*dest = pix;

			if (r_view.lighting && ! (dw->side & THINGDEF_LIT))
				*dest = DoomLightRemap(light, dist, *dest);
		}
	}

	inline void RenderMidMasker(DrawWall *dw, DrawSurf& surf, int x)
	{
		if (surf.kind == DrawSurf::K_INVIS)
			return;

		if (! surf.img)
			return;

		if (query_mode)
			return;

		int y1 = DistToY(dw->cur_iz, surf.h2);
		int y2 = DistToY(dw->cur_iz, surf.h1) - 1;

		if (y1 < dw->oy1)
			y1 = dw->oy1;

		if (y2 > dw->oy2)
			y2 = dw->oy2;

		if (y1 > y2)
			return;

		/* fill pixels */

		RenderTexColumn(dw, surf, x, y1, y2);
	}

	inline void Sort_Swap(int i, int k)
	{
		DrawWall *A = active[i];
		DrawWall *B = active[k];

		active[k] = A;
		active[i] = B;
	}

	int Sort_Partition(int lo, int hi, int pivot_idx)
	{
		/* this is Hoare's algorithm */

		const DrawWall *pivot = active[pivot_idx];

		int s = lo;
		int e = hi;

		for (;;)
		{
			while (s <= e && active[s]->IsCloser(pivot))
				s++;

			if (s > hi)
			{
				// all values were < pivot, including the pivot itself!

				if (pivot_idx != hi)
					Sort_Swap(pivot_idx, hi);

				return hi - 1;
			}

			while (e >= s && ! active[e]->IsCloser(pivot))
				e--;

			if (e < lo)
			{
				// all values were >= pivot

				if (pivot_idx != lo)
					Sort_Swap(pivot_idx, lo);

				return lo;
			}

			if (s < e)
			{
				Sort_Swap(s, e);

				s++;
				e--;

				continue;
			}

			/* NOT NEEDED (it seems)
			if (s == e && active[s]->IsCloser(pivot))
				s++;
			*/

			return s - 1;
		}
	}

	void Sort_Range(int s, int e)
	{
		SYS_ASSERT(s <= e);

		while (s < e)
		{
			// previously there was a bubble sort here, but timing
			// tests showed that it was overkill.  This is enough.
			if (s == e-1)
			{
				const DrawWall *const A = active[s];
				const DrawWall *const B = active[e];

				if (B->IsCloser(A))
					Sort_Swap(s, e);

				return;
			}

			// since we are usually sorting a mostly-sorted list, the
			// wall in the middle is highly likely to be a good pivot.
			int pivot_idx = (s + e) >> 1;

			int mid = Sort_Partition(s, e, pivot_idx);

			// handle degenerate cases
			if (mid <= s)
			{
				s++;
				continue;
			}
			else if (mid+1 >= e)
			{
				e--;
				continue;
			}

			// only use recursion on the smallest group
			// [ it helps to limit stack usage ]
			if ((mid - s) < (e - mid))
			{
				Sort_Range(s, mid);
				s = mid+1;
			}
			else
			{
				Sort_Range(mid+1, e);
				e = mid;
			}
		}
	}

	void SortActiveList()
	{
		// this uses the Quicksort algorithm to sort the active list.
		//
		// Note that this sorting code has been written assuming some
		// limitations of the DrawWall::IsCloser() method -- see the
		// description of that method for more details.

		if (active.size() < 2)
			return;

		Sort_Range(0, (int)active.size() - 1);
	}

#define IZ_EPSILON  1e-5

	void UpdateActiveList(int x)
	{
		DrawWall::vec_t::iterator S, E, P;

		bool changes = false;

		// remove walls that have finished.

		S = active.begin();
		E = active.end();

		S = std::remove_if (S, E, DrawWall::SX2Less(x));

		if (S != E)
		{
			active.erase(S, E);
			changes = true;
		}

		// add new walls that start in this column.

		S = walls.begin();
		E = walls.end();

		S = std::lower_bound(S, E, x, DrawWall::SX1Cmp());
		E = std::upper_bound(S, E, x, DrawWall::SX1Cmp());

		if (S != E)
			changes = true;

		for ( ; S != E ; S++)
		{
			active.push_back(*S);
		}

		// calculate new depth values

		S = active.begin();
		E = active.end();

		for (P=S ; (P != E) ; P++)
		{
			DrawWall *dw = (*P);

			dw->cur_iz = dw->iz1 + dw->diz * (x - dw->sx1);

			if (P != S && (*(P-1))->cur_iz < dw->cur_iz + IZ_EPSILON)
				changes = true;
		}

		// if there are changes, re-sort the active list...

		if (changes && active.size() > 0)
		{
			SortActiveList();
		}
	}

	void RenderWalls()
	{
		// sort walls by their starting column, to allow binary search.

		std::sort(walls.begin(), walls.end(), DrawWall::SX1Cmp());

		active.clear();

		for (int x=0 ; x < r_view.screen_w ; x++)
		{
			// clear vertical depth buffer

			open_y1 = 0;
			open_y2 = r_view.screen_h - 1;

			UpdateActiveList(x);

			// in query mode, only care about a single column
			if (query_mode && x != query_sx)
				continue;

			// render, front to back

			DrawWall::vec_t::iterator S, E, P;

			S = active.begin();
			E = active.end();

			for (P=S ; P != E ; P++)
			{
				DrawWall *dw = (*P);

				// for things, just remember the open space
				{
					dw->oy1 = open_y1;
					dw->oy2 = open_y2;
				}
				if (dw->th >= 0)
					continue;

				RenderWallSurface(dw, dw->ceil,  x, OB3D_Ceil);
				RenderWallSurface(dw, dw->floor, x, OB3D_Floor);

				RenderWallSurface(dw, dw->upper, x, OB3D_Upper);
				RenderWallSurface(dw, dw->lower, x, OB3D_Lower);

				if (open_y1 > open_y2)
					break;
			}

			// now render things, back to front
			// (mid-masked textures are done here too)

			if (P == E)
				P--;

			for ( ; P != (S-1) ; P--)
			{
				DrawWall *dw = (*P);

				if (dw->th >= 0)
					RenderSprite(dw, x);
				else
					RenderMidMasker(dw, dw->rail, x);
			}
		}
	}

	void ClearScreen()
	{
		// color #0 is black (DOOM, Heretic, Hexen)
		// [ other colors won't work here, since img_pixel_t is 16 bits ]
		byte COLOR = 0;

		size_t total = r_view.screen_w * r_view.screen_h;

		memset(r_view.screen, COLOR, total * sizeof(r_view.screen[0]));
	}

	void Render()
	{
		if (! query_mode)
			ClearScreen();

		InitDepthBuf(r_view.screen_w);

		r_view.SaveOffsets();

		for (int i=0 ; i < NumLineDefs ; i++)
			AddLine(i);

		if (r_view.sprites)
			for (int k=0 ; k < NumThings ; k++)
				AddThing(k);

		ClipSolids();

		ComputeSurfaces();

		RenderWalls();

		r_view.RestoreOffsets();
	}

	void Query(int qx, int qy)
	{
		query_mode = 1;
		query_wall = NULL;
		query_sx = qx;
		query_sy = qy;

		Render();

		query_mode = 0;
	}
};


static void BlitHires(int ox, int oy, int ow, int oh)
{
	u8_t line_rgb[r_view.screen_w * 3];

	for (int ry = 0 ; ry < r_view.screen_h ; ry++)
	{
		u8_t *dest = line_rgb;
		u8_t *dest_end = line_rgb + r_view.screen_w * 3;

		const img_pixel_t *src = r_view.screen + ry * r_view.screen_w;

		for ( ; dest < dest_end  ; dest += 3, src++)
		{
			IM_DecodePixel(*src, dest[0], dest[1], dest[2]);
		}

		fl_draw_image(line_rgb, ox, oy+ry, r_view.screen_w, 1);
	}
}


static void BlitLores(int ox, int oy, int ow, int oh)
{
	// if destination width is odd, we store an extra pixel here
	u8_t line_rgb[(ow + 1) * 3];

	for (int ry = 0 ; ry < r_view.screen_h ; ry++)
	{
		const img_pixel_t *src = r_view.screen + ry * r_view.screen_w;

		u8_t *dest = line_rgb;
		u8_t *dest_end = line_rgb + ow * 3;

		for (; dest < dest_end ; dest += 6, src++)
		{
			IM_DecodePixel(*src, dest[0], dest[1], dest[2]);
			IM_DecodePixel(*src, dest[3], dest[4], dest[5]);
		}

		fl_draw_image(line_rgb, ox, oy + ry*2, ow, 1);

		if (ry * 2 + 1 < oh)
		{
			fl_draw_image(line_rgb, ox, oy + ry*2 + 1, ow, 1);
		}
	}
}


void SW_RenderWorld(int ox, int oy, int ow, int oh)
{
	RendInfo rend;

	fl_push_clip(ox, oy, ow, oh);

	rend.Render();

	if (render_high_detail)
		BlitHires(ox, oy, ow, oh);
	else
		BlitLores(ox, oy, ow, oh);

	rend.Highlight(ox, oy);

	fl_pop_clip();
}


bool SW_QueryPoint(Obj3d_t& hl, int qx, int qy)
{
	if (! render_high_detail)
	{
		qx = qx / 2;
		qy = qy / 2;
	}

	RendInfo rend;

	// this runs the renderer, but *no* drawing is done
	rend.Query(qx, qy);

	if (! rend.query_wall)
	{
		// nothing was hit
		return false;
	}

	hl.type = rend.query_part;

	if (hl.type == OB3D_Thing)
	{
		hl.num = rend.query_wall->th;
	}
	else if (hl.type == OB3D_Floor || hl.type == OB3D_Ceil)
	{
		// ouch -- fix?
		for (int n = 0 ; n < NumSectors ; n++)
			if (rend.query_wall->sec == Sectors[n])
				hl.num = n;
	}
	else
	{
		hl.side = rend.query_wall->side;

		// ouch -- fix?
		for (int n = 0 ; n < NumLineDefs ; n++)
			if (rend.query_wall->ld == LineDefs[n])
				hl.num = n;
	}

	return hl.valid();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab