// Copyright (C) 2001-2025 Val Argunov (byte AT qsp DOT org)
/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*/

#pragma once

#include <cstdint>

enum class YUVMatrix { BT601, BT709, BT2020 };

struct YUVPlanes
{
    const uint8_t *y, *u, *v;
    int yStride, uStride, vStride;
    int width, height;
    int xShift, yShift; // chroma subsampling: 1 for 4:2:0 in both directions
    YUVMatrix matrix;
    bool fullRange;
};

// Writes width * height RGB triplets
void ConvertYUVToRGB(const YUVPlanes &planes, uint8_t *rgb);
// Copies a plane, e.g. the luma of an alpha stream
void CopyPlane(const uint8_t *src, int srcStride, int width, int height, uint8_t *dst);
// Converts 4-byte pixels (BGRA or RGBA) into RGB triplets
void ConvertPackedToRGB(const uint8_t *src, int srcStride, int width, int height, bool bgr, uint8_t *rgb);
