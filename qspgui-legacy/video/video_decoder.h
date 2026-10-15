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

#include <wx/string.h>
#include <wx/image.h>
#include <memory>
#include <vector>

struct VideoFrame
{
    wxImage image;
    double time{0.0}; // seconds from the start of the stream
};

// Audio is always interleaved float PCM.
// A decoder is created, used and destroyed on the same (worker) thread.
class VideoDecoder
{
public:
    enum class Status { Ok, End, Error };

    virtual ~VideoDecoder() = default;

    [[nodiscard]] virtual wxSize GetSize() const = 0;
    [[nodiscard]] virtual int GetAudioRate() const = 0; // 0 if there is no audio
    [[nodiscard]] virtual int GetAudioChannels() const = 0;

    // Decodes the next portion of the stream, appending its frames and audio samples
    virtual Status Decode(std::vector<VideoFrame> &frames, std::vector<float> &audio) = 0;
    // Restarts the stream from the beginning
    virtual bool Rewind() = 0;
};

// Built-in WebM decoder (VP8/VP9 + Opus/Vorbis), returns nullptr if it can't handle the file
std::unique_ptr<VideoDecoder> CreateWebMDecoder(const wxString &path, wxString &error);
// Platform decoder (Media Foundation, AVFoundation or GStreamer), returns nullptr if unavailable
std::unique_ptr<VideoDecoder> CreateSystemDecoder(const wxString &path, wxString &error);

// Tries the built-in decoder first and falls back to the platform one
std::unique_ptr<VideoDecoder> CreateVideoDecoder(const wxString &path, wxString &error);

// Most decoders can't handle anything bigger, and such frames would only exhaust memory
constexpr int VIDEO_MAX_DIMENSION = 8192;
