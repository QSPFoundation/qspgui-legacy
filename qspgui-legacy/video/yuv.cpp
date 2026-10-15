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

#include "yuv.h"
#include <cstddef>
#include <cstring>
#include <cmath>

namespace
{
    constexpr int SHIFT = 14;

    struct Coefficients
    {
        int y, rv, gu, gv, bu;
    };

    Coefficients GetCoefficients(const YUVMatrix matrix, const bool fullRange)
    {
        double kr, kb;
        switch (matrix)
        {
        case YUVMatrix::BT709: kr = 0.2126; kb = 0.0722; break;
        case YUVMatrix::BT2020: kr = 0.2627; kb = 0.0593; break;
        default: kr = 0.299; kb = 0.114; break;
        }
        const double kg = 1.0 - kr - kb;
        const double yScale = fullRange ? 1.0 : 255.0 / 219.0;
        const double cScale = fullRange ? 1.0 : 255.0 / 224.0;
        const auto fixed = [](const double value) { return (int)std::lround(value * (1 << SHIFT)); };
        return {
            fixed(yScale),
            fixed(cScale * 2.0 * (1.0 - kr)),
            fixed(cScale * 2.0 * kb * (1.0 - kb) / kg),
            fixed(cScale * 2.0 * kr * (1.0 - kr) / kg),
            fixed(cScale * 2.0 * (1.0 - kb))
        };
    }

    inline uint8_t Clamp(const int value)
    {
        const int result = value >> SHIFT;
        return (uint8_t)(result < 0 ? 0 : (result > 255 ? 255 : result));
    }
}

void ConvertYUVToRGB(const YUVPlanes &planes, uint8_t *rgb)
{
    const Coefficients k = GetCoefficients(planes.matrix, planes.fullRange);
    const int yOffset = planes.fullRange ? 0 : 16;
    const int round = 1 << (SHIFT - 1);
    for (int row = 0; row < planes.height; ++row)
    {
        const uint8_t *yRow = planes.y + (ptrdiff_t)row * planes.yStride;
        const uint8_t *uRow = planes.u + (ptrdiff_t)(row >> planes.yShift) * planes.uStride;
        const uint8_t *vRow = planes.v + (ptrdiff_t)(row >> planes.yShift) * planes.vStride;
        for (int col = 0; col < planes.width; ++col)
        {
            const int y = (yRow[col] - yOffset) * k.y + round;
            const int u = uRow[col >> planes.xShift] - 128;
            const int v = vRow[col >> planes.xShift] - 128;
            *rgb++ = Clamp(y + k.rv * v);
            *rgb++ = Clamp(y - k.gu * u - k.gv * v);
            *rgb++ = Clamp(y + k.bu * u);
        }
    }
}

void CopyPlane(const uint8_t *src, const int srcStride, const int width, const int height, uint8_t *dst)
{
    for (int row = 0; row < height; ++row)
        std::memcpy(dst + (ptrdiff_t)row * width, src + (ptrdiff_t)row * srcStride, width);
}

void ConvertPackedToRGB(const uint8_t *src, const int srcStride, const int width, const int height, const bool bgr, uint8_t *rgb)
{
    const int r = bgr ? 2 : 0;
    const int b = bgr ? 0 : 2;
    for (int row = 0; row < height; ++row)
    {
        const uint8_t *pixel = src + (ptrdiff_t)row * srcStride;
        for (int col = 0; col < width; ++col, pixel += 4)
        {
            *rgb++ = pixel[r];
            *rgb++ = pixel[1];
            *rgb++ = pixel[b];
        }
    }
}
