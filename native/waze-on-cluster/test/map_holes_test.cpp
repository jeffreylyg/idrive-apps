// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Jeffrey Lee
/* Host test of the fullscreen map's see-through holes (map_holes.h): each
 * shape read with its defaults, and a shape that cannot be used refused with
 * the shape named, leaving none; inside a shape covered, outside not, a
 * rect's rounded corner not; a ring's band only, and only within its angles,
 * which go clockwise from the top and may pass it; an edge half covered with
 * a fade and the fade's width linear across it, sharp without; overlapping
 * shapes the larger cover; the mask half the panel's size, sampled at each
 * 2x2's centre. */
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "map_holes.h"

using namespace woc;

static int failures;

static void ok(const char *name, bool cond)
{
    std::printf("%-4s %s\n", cond ? "ok" : "FAIL", name);
    if (!cond)
        failures++;
}

static bool near(float a, float b, float eps = 0.02f)
{
    return std::fabs(a - b) <= eps;
}

static std::vector<Hole> parsed(const std::string &text)
{
    std::vector<Hole> h;
    std::string why;
    if (const char *e = parse_holes(text, &h, &why))
        std::printf("     (%s)\n", e);
    return h;
}

static const char *refused(const std::string &text)
{
    static std::string why;
    std::vector<Hole> h{Hole()};
    const char *e = parse_holes(text, &h, &why);
    return e && h.empty() ? e : nullptr;
}

int main()
{
    std::vector<Hole> h = parsed(" rect:10,20,300,100 ; ellipse:0,0,50,40;ring:100,100,400,300,30;"
                                 "ring:0,0,200,200,20,300,420;rect:0,0,100,100,25;");
    ok("five shapes read, spaces and an empty item ignored", h.size() == 5);
    ok("rect: its box, no corner radius",
       h.size() == 5 && h[0].kind == Hole::Rect && h[0].x == 10 && h[0].y == 20 && h[0].w == 300 && h[0].h == 100 &&
           h[0].r == 0);
    ok("ellipse", h.size() == 5 && h[1].kind == Hole::Ellipse && h[1].w == 50 && h[1].h == 40);
    ok("ring: its band, all of it by default",
       h.size() == 5 && h[2].kind == Hole::Ring && h[2].r == 30 && h[2].a0 == 0 && h[2].a1 == 360);
    ok("ring: its angles, past the top", h.size() == 5 && h[3].a0 == 300 && h[3].a1 == 420);
    ok("rect: a corner radius", h.size() == 5 && h[4].r == 25);
    ok("empty: no holes, not refused", parsed("").empty() && !refused(""));

    ok("a shape without its kind refused, the shape named",
       refused("10,20,30,40") && std::string(refused("10,20,30,40")).find("10,20,30,40: ") == 0);
    ok("an unknown kind refused", refused("square:0,0,10,10"));
    ok("too few or too many numbers refused", refused("rect:0,0,10") && refused("ellipse:0,0,10,10,5") &&
                                                  refused("ring:0,0,100,100") && refused("ring:0,0,100,100,10,20"));
    ok("a number that does not read refused", refused("rect:0,0,10,1x") && refused("rect:0,,10,10"));
    ok("an empty box refused", refused("rect:0,0,0,10") && refused("ellipse:0,0,10,-5"));
    ok("a ring as wide as half its box refused", refused("ring:0,0,100,100,50") && refused("ring:0,0,100,100,0"));
    ok("a ring's angles going down, or past a turn, refused",
       refused("ring:0,0,100,100,10,90,45") && refused("ring:0,0,100,100,10,0,400"));
    ok("a negative corner radius refused", refused("rect:0,0,10,10,-1"));
    std::string many;
    for (int i = 0; i < 33; i++)
        many += "rect:0,0,10,10;";
    ok("33 shapes refused, 32 taken", refused(many) && parsed(many.substr(15)).size() == 32);
    ok("one bad shape refuses all", refused("rect:0,0,10,10;rect:0,0"));

    const std::vector<Hole> r = parsed("rect:100,100,200,100");
    ok("rect sharp: inside covered, outside not", hole_cover(r, 0, 200, 150) == 1 && hole_cover(r, 0, 99, 150) == 0 &&
                                                      hole_cover(r, 0, 301, 150) == 0 && hole_cover(r, 0, 200, 99) == 0);
    ok("rect faded over 20 px: half on the edge, linear across, whole past it",
       near(hole_cover(r, 20, 100, 150), 0.5f) && near(hole_cover(r, 20, 105, 150), 0.75f) &&
           near(hole_cover(r, 20, 95, 150), 0.25f) && hole_cover(r, 20, 110, 150) == 1 &&
           hole_cover(r, 20, 90, 150) == 0);
    const std::vector<Hole> rr = parsed("rect:0,0,100,100,30");
    ok("a rounded rect: its corner not covered, its edge's middle covered",
       hole_cover(rr, 0, 3, 3) == 0 && hole_cover(rr, 0, 50, 1) == 1 && hole_cover(rr, 0, 12, 12) == 1);

    const std::vector<Hole> e = parsed("ellipse:0,0,200,100");
    ok("ellipse: its centre and inside its edge covered, its box's corner not",
       hole_cover(e, 0, 100, 50) == 1 && hole_cover(e, 0, 195, 50) == 1 && hole_cover(e, 0, 100, 2) == 1 &&
           hole_cover(e, 0, 10, 10) == 0 && hole_cover(e, 0, 201, 50) == 0);
    ok("ellipse faded: half on its edge", near(hole_cover(e, 10, 200, 50), 0.5f) && near(hole_cover(e, 10, 100, 0), 0.5f));

    /* a 400x400 ring 40 wide, centre 200,200 */
    const std::vector<Hole> g = parsed("ring:0,0,400,400,40");
    ok("ring: its band covered, inside it and outside not",
       hole_cover(g, 0, 200, 20) == 1 && hole_cover(g, 0, 380, 200) == 1 && hole_cover(g, 0, 200, 200) == 0 &&
           hole_cover(g, 0, 200, 50) == 0 && hole_cover(g, 0, 2, 2) == 0);
    /* from 9 o'clock (270) over the top to 3 o'clock (450 = 90) */
    const std::vector<Hole> arc = parsed("ring:0,0,400,400,40,270,450");
    ok("ring 270-450: the top, left and right of its band covered, the bottom not",
       hole_cover(arc, 0, 200, 20) == 1 && hole_cover(arc, 0, 20, 199) == 1 && hole_cover(arc, 0, 380, 199) == 1 &&
           hole_cover(arc, 0, 200, 380) == 0 && hole_cover(arc, 0, 60, 330) == 0);
    const std::vector<Hole> low = parsed("ring:0,0,400,400,40,90,270");
    ok("ring 90-270: the bottom covered, the top not",
       hole_cover(low, 0, 200, 380) == 1 && hole_cover(low, 0, 200, 20) == 0);
    ok("ring faded: its arc's end half covered, fading along the band either side of it",
       near(hole_cover(arc, 20, 20, 200), 0.5f, 0.05f) && hole_cover(arc, 20, 20, 205) > 0.1f &&
           hole_cover(arc, 20, 20, 205) < 0.4f && hole_cover(arc, 20, 20, 195) > 0.6f && hole_cover(arc, 20, 20, 195) < 0.9f);

    const std::vector<Hole> both = parsed("rect:0,0,100,100;rect:90,0,100,100");
    ok("overlapping shapes: the larger cover", hole_cover(both, 20, 100, 50) == 1 && near(hole_cover(both, 20, 190, 50), 0.5f));

    const std::vector<uint8_t> m = hole_mask(parsed("rect:0,0,960,720"), 0, 1920, 720);
    ok("mask: half the panel's size", m.size() == (size_t)960 * 360);
    ok("mask: the left half see-through, the right half not, the edge between two texels",
       m.size() == (size_t)960 * 360 && m[0] == 255 && m[479] == 255 && m[480] == 0 && m[959] == 0 &&
           m[(size_t)359 * 960 + 479] == 255);
    const std::vector<uint8_t> none = hole_mask({}, 12, 1920, 720);
    bool zero = none.size() == (size_t)960 * 360;
    for (uint8_t b : none)
        zero = zero && b == 0;
    ok("mask without holes: all map", zero);

    std::printf(failures ? "%d FAILED\n" : "all ok\n", failures);
    return failures ? 1 : 0;
}
