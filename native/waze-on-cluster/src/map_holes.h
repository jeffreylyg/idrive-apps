// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Jeffrey Lee
/* See-through holes in the fullscreen map (waze-on-cluster.conf
 * `fullscreen_holes`): the parts of the whole-cluster panel where the map is
 * not drawn, so the cluster's own gauges under the panel show there (its
 * scale rings, numbers, speed and rev readouts, the bottom bar), as on a
 * cluster whose map is drawn under its gauges.
 *
 * A hole is a shape in the panel's pixels (from its top-left):
 *   rect:X,Y,W,H[,R]          a rectangle, its corners rounded by R
 *   ellipse:X,Y,W,H           the ellipse in that box
 *   ring:X,Y,W,H,T[,A0,A1]    the band T wide inside that ellipse's edge,
 *                             from A0 to A1 degrees clockwise from the top
 *                             (0,360 by default: all of it)
 * separated by ";". Their edges fade over `feather` pixels, centred on the
 * shape's edge.
 *
 * The mask is made on the head unit once (hole_mask), at half the panel's
 * size, one byte a pixel: 255 where the map is not drawn, 0 where it is. An
 * effect layer above the map (MapFeed) samples it and takes that much of
 * the map's colour and alpha away, so the panel is see-through there. */
#ifndef WOC_MAP_HOLES_H
#define WOC_MAP_HOLES_H

#include <stdint.h>

#include <string>
#include <vector>

namespace woc {

struct Hole {
    enum Kind { Rect, Ellipse, Ring } kind = Rect;
    float x = 0, y = 0, w = 0, h = 0;
    float r = 0;               /* rect: corner radius; ring: the band's width */
    float a0 = 0, a1 = 360;    /* ring: from, to, degrees clockwise from the top */
    bool operator==(const Hole &o) const
    {
        return kind == o.kind && x == o.x && y == o.y && w == o.w && h == o.h && r == o.r && a0 == o.a0 &&
               a1 == o.a1;
    }
};

/* At most this many holes. */
const size_t kMaxHoles = 32;

/* Parses text (the shapes above) into *out. Returns null, or why it cannot be
 * used (the shape it is about named); *out is then empty. Empty text: no
 * holes. */
const char *parse_holes(const std::string &text, std::vector<Hole> *out, std::string *why);

/* How much of the pixel at x, y (panel pixels, its centre) is a hole: 0-1. */
float hole_cover(const std::vector<Hole> &holes, float feather, float x, float y);

/* The mask of holes on a tw x th panel, at half its size (tw/2 x th/2, one
 * byte a pixel, 255 a hole), each byte the cover at its 2x2 pixels' centre. */
std::vector<uint8_t> hole_mask(const std::vector<Hole> &holes, float feather, uint32_t tw, uint32_t th);

}  // namespace woc

#endif
