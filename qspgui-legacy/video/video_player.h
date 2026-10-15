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

#include "video_decoder.h"
#include "../sound/sound_engine.h"
#include <wx/bitmap.h>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

// Decodes a video on a worker thread and hands out the frames that are due.
// Apart from the worker, everything is used from the GUI thread.
class VideoPlayer
{
public:
    // Opens the file and waits until the decoder is ready, returns nullptr on failure
    static std::unique_ptr<VideoPlayer> Create(const wxString &path, bool toPlay, bool toLoop, wxString &error);
    static void SetOverallVolume(float volume);

    ~VideoPlayer();

    [[nodiscard]] wxSize GetVideoSize() const { return m_videoSize; }
    void SetTargetSize(const wxSize &size);
    void SetMuted(bool isMuted);

    // Picks the frame that is due now, returns true if the picture changed
    bool Update();
    [[nodiscard]] const wxBitmap &GetBitmap() const { return m_bitmap; }
    // There will be no more frames
    [[nodiscard]] bool IsFinished();

private:
    VideoPlayer(bool toPlay, bool toLoop);

    void Run(wxString path, std::promise<wxString> opened);
    void DecodeLoop();
    bool WaitForWork();
    [[nodiscard]] bool NeedsData() const;
    bool WriteAudio(const float *samples, size_t frameCount);
    bool WriteSilence(size_t frameCount);
    [[nodiscard]] bool IsReadyToStart() const;
    [[nodiscard]] double GetClock() const;
    void ApplyVolume();

    const bool m_toPlay;
    const bool m_toLoop;

    std::unique_ptr<VideoDecoder> m_decoder;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_condition;

    // Shared with the worker, guarded by m_mutex
    std::deque<VideoFrame> m_frames;
    wxSize m_targetSize;
    bool m_isReady{false};
    bool m_toStop{false};
    bool m_isDecoded{false};
    bool m_isPosterShown{false};

    // Set before the worker starts decoding
    wxSize m_videoSize;
    int m_audioRate{0};
    int m_audioChannels{0};
    sound_stream *m_stream{nullptr};
    unsigned int m_streamCapacity{0};

    // GUI thread only
    bool m_isStarted{false};
    bool m_isMuted{false};
    std::chrono::steady_clock::time_point m_startTime;
    wxBitmap m_bitmap;
};
