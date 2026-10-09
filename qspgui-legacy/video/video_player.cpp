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

#include "video_player.h"
#include <algorithm>
#include <climits>
#include <optional>
#include <set>

namespace
{
    using namespace std::chrono_literals;

    constexpr size_t QUEUED_FRAMES = 4;           // frames decoded ahead
    constexpr size_t MAX_QUEUED_BYTES = 256 << 20; // when the audio is far ahead of the video in the file
    constexpr double AUDIO_BUFFER_SECONDS = 2.0;
    constexpr double AUDIO_AHEAD_SECONDS = 0.5;   // audio decoded ahead
    constexpr double AUDIO_START_SECONDS = 0.2;   // audio buffered before the playback starts
    constexpr auto POLL_INTERVAL = 10ms;

    float overallVolume = 1.0f;
    std::set<VideoPlayer *> players;
}

std::unique_ptr<VideoPlayer> VideoPlayer::Create(const wxString &path, const bool toPlay, const bool toLoop, wxString &error)
{
    std::unique_ptr<VideoPlayer> player(new VideoPlayer(toPlay, toLoop));
    std::promise<wxString> opened;
    std::future<wxString> result = opened.get_future();
    player->m_thread = std::thread(&VideoPlayer::Run, player.get(), path, std::move(opened));
    error = result.get();
    if (!error.empty()) return nullptr;

    if (toPlay && player->m_audioRate > 0)
    {
        player->m_streamCapacity = (unsigned int)(player->m_audioRate * AUDIO_BUFFER_SECONDS);
        player->m_stream = sound_stream_create((unsigned int)player->m_audioChannels,
            (unsigned int)player->m_audioRate, player->m_streamCapacity);
        player->ApplyVolume();
    }
    {
        std::lock_guard lock(player->m_mutex);
        player->m_isReady = true;
    }
    player->m_condition.notify_all();
    return player;
}

void VideoPlayer::SetOverallVolume(const float volume)
{
    overallVolume = volume;
    for (VideoPlayer *player : players)
        player->ApplyVolume();
}

VideoPlayer::VideoPlayer(const bool toPlay, const bool toLoop) : m_toPlay(toPlay), m_toLoop(toLoop)
{
    players.insert(this);
}

VideoPlayer::~VideoPlayer()
{
    {
        std::lock_guard lock(m_mutex);
        m_toStop = true;
    }
    m_condition.notify_all();
    if (m_thread.joinable()) m_thread.join();
    if (m_stream) sound_stream_free(m_stream);
    players.erase(this);
}

void VideoPlayer::SetTargetSize(const wxSize &size)
{
    std::lock_guard lock(m_mutex);
    m_targetSize = size;
}

void VideoPlayer::SetMuted(const bool isMuted)
{
    m_isMuted = isMuted;
    ApplyVolume();
}

bool VideoPlayer::Update()
{
    std::unique_lock lock(m_mutex);
    if (m_frames.empty()) return false;

    std::optional<VideoFrame> frame;
    if (!m_toPlay)
    {
        // Show the first frame only, like a browser does without "autoplay"
        if (m_isPosterShown) return false;
        frame = std::move(m_frames.front());
        m_frames.pop_front();
        m_isPosterShown = true;
    }
    else
    {
        if (!m_isStarted)
        {
            if (!IsReadyToStart()) return false;
            m_isStarted = true;
            m_startTime = std::chrono::steady_clock::now();
            if (m_stream) sound_stream_start(m_stream);
        }
        const double clock = GetClock();
        while (!m_frames.empty() && m_frames.front().time <= clock)
        {
            frame = std::move(m_frames.front());
            m_frames.pop_front();
        }
        if (!frame) return false;
    }
    const wxSize targetSize = m_targetSize;
    lock.unlock();
    m_condition.notify_all();

    wxImage &image = frame->image;
    if (targetSize.x > 0 && targetSize.y > 0 && image.GetSize() != targetSize)
        image.Rescale(targetSize.x, targetSize.y, wxIMAGE_QUALITY_BILINEAR);
    m_bitmap = wxBitmap(image);
    return true;
}

bool VideoPlayer::IsFinished()
{
    std::lock_guard lock(m_mutex);
    return m_isDecoded && m_frames.empty();
}

void VideoPlayer::Run(wxString path, std::promise<wxString> opened)
{
    wxString error;
    try
    {
        m_decoder = CreateVideoDecoder(path, error);
    }
    catch (const std::exception &e)
    {
        error = e.what();
    }
    if (m_decoder)
    {
        m_videoSize = m_decoder->GetSize();
        m_audioRate = m_decoder->GetAudioRate();
        m_audioChannels = m_decoder->GetAudioChannels();
        opened.set_value(wxString());
    }
    else
    {
        opened.set_value(error.empty() ? wxString("can't open the file") : error);
        return;
    }
    try
    {
        DecodeLoop();
    }
    catch (...)
    {
    }
    m_decoder.reset();

    std::lock_guard lock(m_mutex);
    m_isDecoded = true;
}

void VideoPlayer::DecodeLoop()
{
    std::vector<VideoFrame> frames;
    std::vector<float> audio;
    double timeOffset = 0.0;    // start of the current loop pass
    double videoEnd = 0.0;      // end of the last frame in the current pass
    double lastTime = -1.0;
    double frameDuration = 1.0 / 25.0;
    size_t audioFrames = 0;     // audio frames written during the current pass

    while (WaitForWork())
    {
        frames.clear();
        audio.clear();
        const VideoDecoder::Status status = m_decoder->Decode(frames, audio);

        if (m_stream && !audio.empty())
        {
            const size_t count = audio.size() / (size_t)m_audioChannels;
            if (!WriteAudio(audio.data(), count)) return;
            audioFrames += count;
        }

        if (!frames.empty())
        {
            wxSize targetSize;
            {
                std::lock_guard lock(m_mutex);
                targetSize = m_targetSize;
            }
            for (VideoFrame &frame : frames)
            {
                if (lastTime >= 0.0 && frame.time > lastTime)
                    frameDuration = std::min(frame.time - lastTime, 1.0);
                lastTime = frame.time;
                videoEnd = std::max(videoEnd, frame.time + frameDuration);
                if (targetSize.x > 0 && targetSize.y > 0 && frame.image.GetSize() != targetSize)
                    frame.image.Rescale(targetSize.x, targetSize.y, wxIMAGE_QUALITY_BILINEAR);
                frame.time += timeOffset;
            }

            std::lock_guard lock(m_mutex);
            for (VideoFrame &frame : frames)
            {
                const size_t frameBytes = (size_t)frame.image.GetWidth() * frame.image.GetHeight() * (frame.image.HasAlpha() ? 4 : 3);
                const size_t maxFrames = std::max<size_t>(QUEUED_FRAMES, MAX_QUEUED_BYTES / std::max<size_t>(frameBytes, 1));
                // Dropping frames is better than letting the audio run dry
                if (m_frames.size() < maxFrames) m_frames.push_back(std::move(frame));
            }
        }

        if (status == VideoDecoder::Status::Ok) continue;

        if (status == VideoDecoder::Status::End && m_toPlay && m_toLoop)
        {
            double duration = videoEnd;
            if (m_stream)
            {
                duration = std::max(duration, (double)audioFrames / m_audioRate);
                // Keep the sound in step with the next pass
                if (!WriteSilence((size_t)(duration * m_audioRate) - std::min((size_t)(duration * m_audioRate), audioFrames))) return;
            }
            if (duration <= 0.0 || !m_decoder->Rewind()) return;
            timeOffset += duration;
            videoEnd = 0.0;
            lastTime = -1.0;
            audioFrames = 0;
            continue;
        }

        // Let the audio clock run until the last frame is shown
        if (m_stream && videoEnd * m_audioRate > (double)audioFrames)
            WriteSilence((size_t)(videoEnd * m_audioRate) - audioFrames);
        return;
    }
}

bool VideoPlayer::WaitForWork()
{
    std::unique_lock lock(m_mutex);
    // The audio thread drains the stream without notifying us, hence the polling
    while (!m_toStop && !(m_isReady && NeedsData()))
        m_condition.wait_for(lock, POLL_INTERVAL);
    return !m_toStop;
}

bool VideoPlayer::NeedsData() const
{
    if (!m_toPlay) return m_frames.empty() && !m_isPosterShown;
    if (m_frames.size() < QUEUED_FRAMES) return true;
    return m_stream && sound_stream_get_queued(m_stream) < m_audioRate * AUDIO_AHEAD_SECONDS;
}

bool VideoPlayer::WriteAudio(const float *samples, size_t frameCount)
{
    while (frameCount > 0)
    {
        const unsigned int written = sound_stream_write(m_stream, samples, (unsigned int)std::min<size_t>(frameCount, UINT_MAX));
        samples += (size_t)written * m_audioChannels;
        frameCount -= written;
        if (frameCount > 0)
        {
            std::unique_lock lock(m_mutex);
            if (m_condition.wait_for(lock, POLL_INTERVAL, [this] { return m_toStop; })) return false;
        }
    }
    return true;
}

bool VideoPlayer::WriteSilence(size_t frameCount)
{
    const std::vector<float> silence((size_t)m_audioRate / 10 * m_audioChannels, 0.0f);
    while (frameCount > 0)
    {
        const size_t count = std::min(frameCount, silence.size() / m_audioChannels);
        if (!WriteAudio(silence.data(), count)) return false;
        frameCount -= count;
    }
    return true;
}

bool VideoPlayer::IsReadyToStart() const
{
    if (m_isDecoded || m_frames.size() >= QUEUED_FRAMES || !m_stream) return true;
    const unsigned int queued = sound_stream_get_queued(m_stream);
    return queued >= m_audioRate * AUDIO_START_SECONDS || queued >= m_streamCapacity / 2;
}

double VideoPlayer::GetClock() const
{
    if (m_stream) return (double)sound_stream_get_played(m_stream) / m_audioRate;
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_startTime).count();
}

void VideoPlayer::ApplyVolume()
{
    if (m_stream) sound_stream_set_volume(m_stream, m_isMuted ? 0.0f : overallVolume);
}
