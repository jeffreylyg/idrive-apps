// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Jeffrey Lee
#include "map_holes.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace woc {

namespace {

const float kPi = 3.14159265358979f;

/* Signed distance from the edge of an ellipse of radii rx, ry centred on
 * the origin (negative inside), near the edge: the radial overshoot scaled
 * by the smaller radius, enough for a fade of a few pixels. */
float ellipse_dist(float px, float py, float rx, float ry)
{
    if (rx <= 0 || ry <= 0)
        return 1e9f;
    const float k = std::sqrt((px * px) / (rx * rx) + (py * py) / (ry * ry));
    return (k - 1) * std::min(rx, ry);
}

float hole_dist(const Hole &h, float x, float y)
{
    const float cx = h.x + h.w / 2, cy = h.y + h.h / 2, px = x - cx, py = y - cy;
    switch (h.kind) {
    case Hole::Rect: {
        const float r = std::min(h.r, std::min(h.w, h.h) / 2);
        const float qx = std::fabs(px) - (h.w / 2 - r), qy = std::fabs(py) - (h.h / 2 - r);
        const float ox = std::max(qx, 0.0f), oy = std::max(qy, 0.0f);
        return std::sqrt(ox * ox + oy * oy) + std::min(std::max(qx, qy), 0.0f) - r;
    }
    case Hole::Ellipse:
        return ellipse_dist(px, py, h.w / 2, h.h / 2);
    case Hole::Ring: {
        const float rx = h.w / 2, ry = h.h / 2;
        float d = std::max(ellipse_dist(px, py, rx, ry), -ellipse_dist(px, py, rx - h.r, ry - h.r));
        const float span = h.a1 - h.a0;
        if (span < 360) {
            /* degrees clockwise from the top, the panel's y down; the
             * distance along the band to the arc's nearer end, negative
             * within the arc, so its ends fade as its edges do */
            const float a = std::atan2(px, -py) * 180 / kPi;
            const float from = std::fmod(std::fmod(a - h.a0, 360.0f) + 360.0f, 360.0f);
            const float deg = from <= span ? -std::min(from, span - from) : std::min(from - span, 360 - from);
            d = std::max(d, deg * kPi / 180 * std::sqrt(px * px + py * py));
        }
        return d;
    }
    }
    return 1e9f;
}

bool numbers(const std::string &s, std::vector<float> *out)
{
    out->clear();
    std::stringstream ss(s);
    std::string n;
    while (std::getline(ss, n, ',')) {
        char *end = nullptr;
        const float v = std::strtof(n.c_str(), &end);
        if (n.empty() || *end || !std::isfinite(v))
            return false;
        out->push_back(v);
    }
    return true;
}

}  // namespace

const char *parse_holes(const std::string &text, std::vector<Hole> *out, std::string *why)
{
    out->clear();
    std::stringstream ss(text);
    std::string item;
    std::vector<float> v;
    while (std::getline(ss, item, ';')) {
        item.erase(std::remove_if(item.begin(), item.end(), [](char c) { return c == ' ' || c == '\t'; }),
                   item.end());
        if (item.empty())
            continue;
        const size_t colon = item.find(':');
        const std::string kind = item.substr(0, colon);
        Hole h;
        const char *err = nullptr;
        if (colon == std::string::npos || !numbers(item.substr(colon + 1), &v)) {
            err = "must be rect:X,Y,W,H[,R], ellipse:X,Y,W,H or ring:X,Y,W,H,T[,A0,A1]";
        } else if (kind == "rect" && (v.size() == 4 || v.size() == 5)) {
            h.kind = Hole::Rect;
            h.r = v.size() == 5 ? v[4] : 0;
        } else if (kind == "ellipse" && v.size() == 4) {
            h.kind = Hole::Ellipse;
        } else if (kind == "ring" && (v.size() == 5 || v.size() == 7)) {
            h.kind = Hole::Ring;
            h.r = v[4];
            if (v.size() == 7) {
                h.a0 = v[5];
                h.a1 = v[6];
            }
            if (h.r <= 0 || h.r * 2 >= std::min(v[2], v[3]))
                err = "a ring's width must be more than 0 and less than half its box";
            else if (h.a1 <= h.a0 || h.a1 - h.a0 > 360)
                err = "a ring's angles must go up, by at most 360";
        } else {
            err = "must be rect:X,Y,W,H[,R], ellipse:X,Y,W,H or ring:X,Y,W,H,T[,A0,A1]";
        }
        if (!err) {
            h.x = v[0];
            h.y = v[1];
            h.w = v[2];
            h.h = v[3];
            if (h.w <= 0 || h.h <= 0 || h.w > 4096 || h.h > 4096 || std::fabs(h.x) > 4096 || std::fabs(h.y) > 4096)
                err = "its box must be 1 to 4096 wide and high, within 4096 of the panel";
            else if (h.kind == Hole::Rect && h.r < 0)
                err = "a rect's corner radius cannot be negative";
            else if (out->size() == kMaxHoles)
                err = "too many holes (at most 32)";
        }
        if (err) {
            *why = item + ": " + err;
            out->clear();
            return why->c_str();
        }
        out->push_back(h);
    }
    return nullptr;
}

float hole_cover(const std::vector<Hole> &holes, float feather, float x, float y)
{
    float cover = 0;
    for (const Hole &h : holes) {
        const float d = hole_dist(h, x, y);
        const float c = feather > 0 ? std::min(1.0f, std::max(0.0f, 0.5f - d / feather)) : (d <= 0 ? 1.0f : 0.0f);
        cover = std::max(cover, c);
        if (cover >= 1)
            break;
    }
    return cover;
}

std::vector<uint8_t> hole_mask(const std::vector<Hole> &holes, float feather, uint32_t tw, uint32_t th)
{
    const uint32_t w = tw / 2, h = th / 2;
    std::vector<uint8_t> m((size_t)w * h, 0);
    if (holes.empty())
        return m;
    for (uint32_t j = 0; j < h; j++)
        for (uint32_t i = 0; i < w; i++)
            m[(size_t)j * w + i] = (uint8_t)std::lround(255 * hole_cover(holes, feather, 2.0f * i + 1, 2.0f * j + 1));
    return m;
}

}  // namespace woc
