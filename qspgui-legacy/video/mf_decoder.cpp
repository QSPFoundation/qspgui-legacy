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

// System decoder for Windows, based on the Media Foundation source reader (Windows 7+)

#include "video_decoder.h"
#include "yuv.h"
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <cstdlib>
#include <mutex>

namespace
{
    template <typename T>
    class ComPtr
    {
    public:
        ComPtr() = default;
        ComPtr(const ComPtr &) = delete;
        ComPtr &operator=(const ComPtr &) = delete;
        ~ComPtr() { Reset(); }

        void Reset()
        {
            if (m_ptr)
            {
                m_ptr->Release();
                m_ptr = nullptr;
            }
        }

        T **operator&()
        {
            Reset();
            return &m_ptr;
        }

        T *operator->() const { return m_ptr; }
        T *Get() const { return m_ptr; }
        explicit operator bool() const { return m_ptr != nullptr; }

    private:
        T *m_ptr{nullptr};
    };

    bool StartMediaFoundation()
    {
        static bool isStarted = false;
        static std::once_flag once;
        // Never shut down: other threads may still use it while the application exits
        std::call_once(once, [] { isStarted = SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_LITE)); });
        return isStarted;
    }

    class MFDecoder : public VideoDecoder
    {
    public:
        MFDecoder()
        {
            // The decoder lives on its own worker thread
            m_isComInitialized = SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
        }

        ~MFDecoder() override
        {
            m_reader.Reset();
            if (m_isComInitialized) CoUninitialize();
        }

        bool Open(const wxString &path, wxString &error)
        {
            if (!StartMediaFoundation())
            {
                error = "Media Foundation isn't available";
                return false;
            }

            bool isOpened = false;
            for (const bool isAdvanced : {true, false})
            {
                if (!CreateReader(path, isAdvanced))
                {
                    error = "Media Foundation can't open the file";
                    continue;
                }
                if (SelectStreams() && ConfigureVideo())
                {
                    isOpened = true;
                    break;
                }
                error = "Media Foundation can't decode the video";
            }
            if (!isOpened) return false;
            if (m_audioStream != NO_STREAM && !ConfigureAudio())
            {
                m_reader->SetStreamSelection(m_audioStream, FALSE);
                m_audioStream = NO_STREAM;
            }
            return true;
        }

        [[nodiscard]] wxSize GetSize() const override { return m_size; }
        [[nodiscard]] int GetAudioRate() const override { return m_audioStream != NO_STREAM ? m_audioRate : 0; }
        [[nodiscard]] int GetAudioChannels() const override { return m_audioStream != NO_STREAM ? m_audioChannels : 0; }

        Status Decode(std::vector<VideoFrame> &frames, std::vector<float> &audio) override
        {
            DWORD streamIndex = 0, flags = 0;
            LONGLONG timestamp = 0;
            ComPtr<IMFSample> sample;
            if (FAILED(m_reader->ReadSample(MF_SOURCE_READER_ANY_STREAM, 0, &streamIndex, &flags, &timestamp, &sample)))
                return Status::Error;
            if (flags & MF_SOURCE_READERF_ERROR) return Status::Error;

            const bool isVideo = streamIndex == m_videoStream;
            if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
            {
                (isVideo ? m_isVideoDone : m_isAudioDone) = true;
                if (m_isVideoDone && (m_audioStream == NO_STREAM || m_isAudioDone)) return Status::End;
            }
            if (isVideo && (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) && !ReadVideoFormat())
                return Status::Error;
            if (!sample) return Status::Ok;

            if (isVideo)
                return ReadVideoSample(sample.Get(), (double)timestamp / 1e7, frames) ? Status::Ok : Status::Error;
            if (streamIndex == m_audioStream)
                ReadAudioSample(sample.Get(), audio);
            return Status::Ok;
        }

        bool Rewind() override
        {
            PROPVARIANT position;
            PropVariantInit(&position);
            position.vt = VT_I8;
            position.hVal.QuadPart = 0;
            m_isVideoDone = m_isAudioDone = false;
            return SUCCEEDED(m_reader->SetCurrentPosition(GUID{}, position));
        }

    private:
        static constexpr DWORD NO_STREAM = (DWORD)-1;

        bool CreateReader(const wxString &path, const bool isAdvanced)
        {
            static const GUID advancedVideoProcessing =
                {0x0f81da2c, 0xb537, 0x4672, {0xa8, 0xb2, 0xa6, 0x81, 0xb1, 0x73, 0x07, 0xa3}};

            m_reader.Reset();
            m_videoStream = m_audioStream = NO_STREAM;
            ComPtr<IMFAttributes> attributes;
            return SUCCEEDED(MFCreateAttributes(&attributes, 1)) &&
                SUCCEEDED(attributes->SetUINT32(isAdvanced ? advancedVideoProcessing : MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE)) &&
                SUCCEEDED(MFCreateSourceReaderFromURL(path.wc_str(), attributes.Get(), &m_reader));
        }

        bool SelectStreams()
        {
            m_reader->SetStreamSelection((DWORD)MF_SOURCE_READER_ALL_STREAMS, FALSE);
            for (DWORD index = 0;; ++index)
            {
                ComPtr<IMFMediaType> type;
                const HRESULT result = m_reader->GetNativeMediaType(index, 0, &type);
                if (result == MF_E_INVALIDSTREAMNUMBER) break;
                if (FAILED(result)) continue;
                GUID major;
                if (FAILED(type->GetGUID(MF_MT_MAJOR_TYPE, &major))) continue;
                if (major == MFMediaType_Video && m_videoStream == NO_STREAM)
                    m_videoStream = index;
                else if (major == MFMediaType_Audio && m_audioStream == NO_STREAM)
                    m_audioStream = index;
            }
            if (m_videoStream == NO_STREAM) return false;
            m_reader->SetStreamSelection(m_videoStream, TRUE);
            if (m_audioStream != NO_STREAM) m_reader->SetStreamSelection(m_audioStream, TRUE);
            return true;
        }

        bool ConfigureVideo()
        {
            ComPtr<IMFMediaType> type;
            return SUCCEEDED(MFCreateMediaType(&type)) &&
                SUCCEEDED(type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video)) &&
                SUCCEEDED(type->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32)) &&
                SUCCEEDED(m_reader->SetCurrentMediaType(m_videoStream, nullptr, type.Get())) &&
                ReadVideoFormat();
        }

        bool ReadVideoFormat()
        {
            ComPtr<IMFMediaType> type;
            GUID subtype;
            UINT64 frameSize = 0;
            if (FAILED(m_reader->GetCurrentMediaType(m_videoStream, &type)) ||
                FAILED(type->GetGUID(MF_MT_SUBTYPE, &subtype)) || subtype != MFVideoFormat_RGB32 ||
                FAILED(type->GetUINT64(MF_MT_FRAME_SIZE, &frameSize)))
            {
                return false;
            }
            m_frameWidth = (int)(frameSize >> 32);
            m_frameHeight = (int)(frameSize & 0xFFFFFFFF);
            if (m_frameWidth <= 0 || m_frameHeight <= 0 ||
                m_frameWidth > VIDEO_MAX_DIMENSION || m_frameHeight > VIDEO_MAX_DIMENSION)
            {
                return false;
            }

            UINT32 stride = 0;
            m_stride = SUCCEEDED(type->GetUINT32(MF_MT_DEFAULT_STRIDE, &stride)) ? (LONG)stride : m_frameWidth * 4;

            // Decoders pad the frames, e.g. 1920x1080 is decoded as 1920x1088
            m_area = wxRect(0, 0, m_frameWidth, m_frameHeight);
            MFVideoArea aperture;
            if (SUCCEEDED(type->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, (UINT8 *)&aperture, sizeof(aperture), nullptr)))
            {
                const wxRect area(aperture.OffsetX.value, aperture.OffsetY.value, aperture.Area.cx, aperture.Area.cy);
                if (area.width > 0 && area.height > 0 && wxRect(0, 0, m_frameWidth, m_frameHeight).Contains(area))
                    m_area = area;
            }
            m_size = m_area.GetSize();
            return true;
        }

        bool ConfigureAudio()
        {
            ComPtr<IMFMediaType> type;
            if (FAILED(MFCreateMediaType(&type)) ||
                FAILED(type->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)) ||
                FAILED(type->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float)) ||
                FAILED(m_reader->SetCurrentMediaType(m_audioStream, nullptr, type.Get())))
            {
                return false;
            }
            ComPtr<IMFMediaType> current;
            UINT32 channels = 0, rate = 0;
            if (FAILED(m_reader->GetCurrentMediaType(m_audioStream, &current)) ||
                FAILED(current->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels)) ||
                FAILED(current->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate)))
            {
                return false;
            }
            m_audioChannels = (int)channels;
            m_audioRate = (int)rate;
            return m_audioChannels > 0 && m_audioChannels <= 8 && m_audioRate > 0;
        }

        bool ReadVideoSample(IMFSample *sample, const double time, std::vector<VideoFrame> &frames)
        {
            ComPtr<IMFMediaBuffer> buffer;
            if (FAILED(sample->ConvertToContiguousBuffer(&buffer))) return false;

            BYTE *scanline0 = nullptr;
            LONG pitch = 0;
            ComPtr<IMF2DBuffer> buffer2D;
            BYTE *data = nullptr;
            DWORD length = 0;
            const bool is2D = SUCCEEDED(buffer->QueryInterface(IID_PPV_ARGS(&buffer2D))) &&
                SUCCEEDED(buffer2D->GetContiguousLength(&length)) &&
                SUCCEEDED(buffer2D->Lock2D(&scanline0, &pitch));
            if (!is2D)
            {
                if (FAILED(buffer->Lock(&data, nullptr, &length))) return false;
                // Bottom-up images have a negative stride
                pitch = m_stride;
                scanline0 = pitch >= 0 ? data : data + (ptrdiff_t)(-pitch) * (m_frameHeight - 1);
            }
            // Never trust the buffer to match the format
            if ((size_t)length < (size_t)std::abs(pitch) * m_frameHeight || std::abs(pitch) < m_frameWidth * 4)
            {
                if (is2D)
                    buffer2D->Unlock2D();
                else
                    buffer->Unlock();
                return false;
            }

            VideoFrame frame;
            frame.time = time;
            frame.image.Create(m_area.width, m_area.height, false);
            if (frame.image.IsOk())
            {
                const BYTE *origin = scanline0 + (ptrdiff_t)m_area.y * pitch + (ptrdiff_t)m_area.x * 4;
                ConvertPackedToRGB(origin, (int)pitch, m_area.width, m_area.height, true, frame.image.GetData());
                frames.push_back(std::move(frame));
            }

            if (is2D)
                buffer2D->Unlock2D();
            else
                buffer->Unlock();
            return true;
        }

        void ReadAudioSample(IMFSample *sample, std::vector<float> &audio) const
        {
            ComPtr<IMFMediaBuffer> buffer;
            BYTE *data = nullptr;
            DWORD length = 0;
            if (FAILED(sample->ConvertToContiguousBuffer(&buffer)) || FAILED(buffer->Lock(&data, nullptr, &length)))
                return;
            const size_t count = length / sizeof(float) / m_audioChannels * m_audioChannels;
            const float *samples = reinterpret_cast<const float *>(data);
            audio.insert(audio.end(), samples, samples + count);
            buffer->Unlock();
        }

        bool m_isComInitialized{false};
        ComPtr<IMFSourceReader> m_reader;
        DWORD m_videoStream{NO_STREAM};
        DWORD m_audioStream{NO_STREAM};
        int m_frameWidth{0};
        int m_frameHeight{0};
        LONG m_stride{0};
        wxRect m_area;
        wxSize m_size;
        int m_audioRate{0};
        int m_audioChannels{0};
        bool m_isVideoDone{false};
        bool m_isAudioDone{false};
    };
}

std::unique_ptr<VideoDecoder> CreateSystemDecoder(const wxString &path, wxString &error)
{
    auto decoder = std::make_unique<MFDecoder>();
    if (!decoder->Open(path, error)) return nullptr;
    return decoder;
}
