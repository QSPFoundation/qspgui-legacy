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

#include "video_decoder.h"
#include "yuv.h"
#include <wx/ffile.h>
#include <nestegg/nestegg.h>
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>
#include <opus_multistream.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <iterator>
#include <thread>

// The implementation lives in the sound engine
#define STB_VORBIS_HEADER_ONLY
#include "stb_vorbis.c"

namespace
{
    constexpr int MAX_AUDIO_CHANNELS = 8;
    constexpr unsigned int NO_TRACK = (unsigned int)-1;
    constexpr unsigned int BLOCK_ADDITIONAL_ALPHA = 1;

    // nestegg I/O on top of a file

    class WebMFile
    {
    public:
        bool Open(const wxString &path) { return m_file.Open(path, "rb"); }
        bool Rewind() { return m_file.Seek(0); }

        nestegg_io GetIO()
        {
            return { &WebMFile::Read, &WebMFile::Seek, &WebMFile::Tell, this };
        }

    private:
        static int64_t Read(void *buffer, size_t length, void *userData)
        {
            wxFFile &file = static_cast<WebMFile *>(userData)->m_file;
            const size_t read = file.Read(buffer, length);
            if (read == 0 && file.Error()) return -1;
            return (int64_t)read;
        }

        static int Seek(int64_t offset, int whence, void *userData)
        {
            wxSeekMode mode;
            switch (whence)
            {
            case NESTEGG_SEEK_SET: mode = wxFromStart; break;
            case NESTEGG_SEEK_CUR: mode = wxFromCurrent; break;
            case NESTEGG_SEEK_END: mode = wxFromEnd; break;
            default: return -1;
            }
            return static_cast<WebMFile *>(userData)->m_file.Seek(offset, mode) ? 0 : -1;
        }

        static int64_t Tell(void *userData)
        {
            return static_cast<WebMFile *>(userData)->m_file.Tell();
        }

        wxFFile m_file;
    };

    // Audio codecs

    class AudioCodec
    {
    public:
        virtual ~AudioCodec() = default;
        [[nodiscard]] virtual int GetRate() const = 0;
        [[nodiscard]] virtual int GetChannels() const = 0;
        virtual bool Reset() = 0;
        virtual void Decode(const unsigned char *data, size_t size, int64_t discardPadding, std::vector<float> &audio) = 0;
    };

    class OpusCodec : public AudioCodec
    {
    public:
        ~OpusCodec() override
        {
            if (m_decoder) opus_multistream_decoder_destroy(m_decoder);
        }

        bool Init(const unsigned char *head, const size_t size)
        {
            // "OpusHead" packet, see RFC 7845
            if (size < 19 || std::memcmp(head, "OpusHead", 8) != 0 || (head[8] >> 4) != 0) return false;
            m_channels = head[9];
            m_preSkip = head[10] | (head[11] << 8);
            const int16_t gain = (int16_t)(head[16] | (head[17] << 8));
            const int family = head[18];
            if (m_channels < 1 || m_channels > MAX_AUDIO_CHANNELS) return false;

            int streams, coupled;
            std::array<unsigned char, 255> mapping{};
            if (family == 0)
            {
                if (m_channels > 2) return false;
                streams = 1;
                coupled = m_channels - 1;
                mapping[0] = 0;
                mapping[1] = 1;
            }
            else
            {
                if (size < 21 + (size_t)m_channels) return false;
                streams = head[19];
                coupled = head[20];
                if (streams < 1 || coupled > streams || streams + coupled > 255) return false;
                std::copy_n(head + 21, m_channels, mapping.begin());
            }

            int error;
            m_decoder = opus_multistream_decoder_create(48000, m_channels, streams, coupled, mapping.data(), &error);
            if (!m_decoder || error != OPUS_OK) return false;
            opus_multistream_decoder_ctl(m_decoder, OPUS_SET_GAIN(gain));
            m_buffer.resize((size_t)MAX_FRAME_SIZE * m_channels);
            m_toSkip = m_preSkip;
            return true;
        }

        [[nodiscard]] int GetRate() const override { return 48000; }
        [[nodiscard]] int GetChannels() const override { return m_channels; }

        bool Reset() override
        {
            m_toSkip = m_preSkip;
            return opus_multistream_decoder_ctl(m_decoder, OPUS_RESET_STATE) == OPUS_OK;
        }

        void Decode(const unsigned char *data, const size_t size, const int64_t discardPadding, std::vector<float> &audio) override
        {
            if (size > INT32_MAX) return;
            int samples = opus_multistream_decode_float(m_decoder, data, (opus_int32)size, m_buffer.data(), MAX_FRAME_SIZE, 0);
            if (samples <= 0) return;
            if (discardPadding > 0)
            {
                const int64_t discard = std::min<int64_t>(discardPadding * 48 / 1000000, samples);
                samples -= (int)discard;
            }
            const int skip = std::min(m_toSkip, samples);
            m_toSkip -= skip;
            audio.insert(audio.end(), m_buffer.begin() + (ptrdiff_t)skip * m_channels, m_buffer.begin() + (ptrdiff_t)samples * m_channels);
        }

    private:
        static constexpr int MAX_FRAME_SIZE = 5760; // 120 ms at 48 kHz

        OpusMSDecoder *m_decoder{nullptr};
        std::vector<float> m_buffer;
        int m_channels{0};
        int m_preSkip{0};
        int m_toSkip{0};
    };

    // stb_vorbis decodes Ogg pages, so every WebM packet gets wrapped into Ogg pages
    class VorbisCodec : public AudioCodec
    {
    public:
        ~VorbisCodec() override
        {
            if (m_vorbis) stb_vorbis_close(m_vorbis);
        }

        bool Init(nestegg *context, const unsigned int track)
        {
            unsigned int count;
            if (nestegg_track_codec_data_count(context, track, &count) != 0 || count != 3) return false;
            uint32_t sequence = 0;
            for (unsigned int i = 0; i < count; ++i)
            {
                unsigned char *data;
                size_t size;
                if (nestegg_track_codec_data(context, track, i, &data, &size) != 0) return false;
                AppendPacket(m_headers, data, size, i == 0, sequence);
            }
            m_headerPages = sequence;
            if (!Reset()) return false;
            const stb_vorbis_info info = stb_vorbis_get_info(m_vorbis);
            m_rate = (int)info.sample_rate;
            m_channels = info.channels;
            return m_rate > 0 && m_channels > 0 && m_channels <= MAX_AUDIO_CHANNELS;
        }

        [[nodiscard]] int GetRate() const override { return m_rate; }
        [[nodiscard]] int GetChannels() const override { return m_channels; }

        bool Reset() override
        {
            if (m_vorbis) stb_vorbis_close(m_vorbis);
            m_sequence = m_headerPages;
            int used, error;
            m_vorbis = stb_vorbis_open_pushdata(m_headers.data(), (int)m_headers.size(), &used, &error, nullptr);
            return m_vorbis != nullptr;
        }

        void Decode(const unsigned char *data, const size_t size, int64_t, std::vector<float> &audio) override
        {
            if (size > 16 * 1024 * 1024) return;
            m_page.clear();
            AppendPacket(m_page, data, size, false, m_sequence);

            size_t offset = 0;
            while (offset < m_page.size())
            {
                int channels, samples;
                float **output;
                const int used = stb_vorbis_decode_frame_pushdata(m_vorbis, m_page.data() + offset,
                    (int)(m_page.size() - offset), &channels, &output, &samples);
                if (used <= 0) break;
                offset += (size_t)used;
                if (samples <= 0 || channels != m_channels) continue;
                const size_t start = audio.size();
                audio.resize(start + (size_t)samples * channels);
                float *out = audio.data() + start;
                for (int sample = 0; sample < samples; ++sample)
                    for (int channel = 0; channel < channels; ++channel)
                        *out++ = output[channel][sample];
            }
        }

    private:
        static uint32_t PageCRC(const unsigned char *data, const size_t size)
        {
            static const std::array<uint32_t, 256> table = [] {
                std::array<uint32_t, 256> result{};
                for (uint32_t i = 0; i < 256; ++i)
                {
                    uint32_t value = i << 24;
                    for (int bit = 0; bit < 8; ++bit)
                        value = (value & 0x80000000) ? (value << 1) ^ 0x04C11DB7 : value << 1;
                    result[i] = value;
                }
                return result;
            }();
            uint32_t crc = 0;
            for (size_t i = 0; i < size; ++i)
                crc = (crc << 8) ^ table[((crc >> 24) ^ data[i]) & 0xFF];
            return crc;
        }

        static void PutLE32(unsigned char *out, const uint32_t value)
        {
            for (int i = 0; i < 4; ++i) out[i] = (unsigned char)(value >> (8 * i));
        }

        // Writes a packet as one or more Ogg pages
        static void AppendPacket(std::vector<unsigned char> &out, const unsigned char *data, const size_t size, const bool first, uint32_t &sequence)
        {
            std::vector<unsigned char> lacing(size / 255, 255);
            lacing.push_back((unsigned char)(size % 255));

            size_t lacingPos = 0, dataPos = 0;
            bool continued = false;
            while (lacingPos < lacing.size())
            {
                const size_t segments = std::min<size_t>(255, lacing.size() - lacingPos);
                size_t pageData = 0;
                for (size_t i = 0; i < segments; ++i) pageData += lacing[lacingPos + i];
                const bool finished = lacingPos + segments == lacing.size();

                const size_t pageStart = out.size();
                out.resize(pageStart + 27 + segments + pageData);
                unsigned char *page = out.data() + pageStart;
                std::memcpy(page, "OggS", 4);
                page[4] = 0;
                page[5] = (unsigned char)((continued ? 0x01 : 0) | (first && !continued ? 0x02 : 0));
                // A packet which doesn't finish on the page has no granule position
                const uint64_t granule = finished ? 0 : UINT64_MAX;
                for (int i = 0; i < 8; ++i) page[6 + i] = (unsigned char)(granule >> (8 * i));
                PutLE32(page + 14, 1);
                PutLE32(page + 18, sequence++);
                PutLE32(page + 22, 0);
                page[26] = (unsigned char)segments;
                std::copy_n(lacing.begin() + (ptrdiff_t)lacingPos, segments, page + 27);
                std::memcpy(page + 27 + segments, data + dataPos, pageData);
                PutLE32(page + 22, PageCRC(page, 27 + segments + pageData));

                lacingPos += segments;
                dataPos += pageData;
                continued = true;
            }
        }

        std::vector<unsigned char> m_headers;
        std::vector<unsigned char> m_page;
        stb_vorbis *m_vorbis{nullptr};
        uint32_t m_sequence{0};
        uint32_t m_headerPages{0};
        int m_rate{0};
        int m_channels{0};
    };

    // VP8/VP9

    class VpxCodec
    {
    public:
        ~VpxCodec()
        {
            if (m_initialized) vpx_codec_destroy(&m_codec);
        }

        bool Init(const int codecId, const unsigned int width, const unsigned int height)
        {
            vpx_codec_iface_t *iface = codecId == NESTEGG_CODEC_VP8 ? vpx_codec_vp8_dx() : vpx_codec_vp9_dx();
            vpx_codec_dec_cfg_t config{};
            config.threads = std::clamp(std::thread::hardware_concurrency(), 1u, 8u);
            config.w = width;
            config.h = height;
            m_initialized = vpx_codec_dec_init(&m_codec, iface, &config, 0) == VPX_CODEC_OK;
            return m_initialized;
        }

        // Returns the last frame produced by the packet, or nullptr
        vpx_image_t *Decode(const unsigned char *data, const size_t size)
        {
            if (size > UINT32_MAX || vpx_codec_decode(&m_codec, data, (unsigned int)size, nullptr, 0) != VPX_CODEC_OK)
            {
                m_failed = true;
                return nullptr;
            }
            vpx_codec_iter_t iter = nullptr;
            vpx_image_t *last = nullptr;
            while (vpx_image_t *image = vpx_codec_get_frame(&m_codec, &iter)) last = image;
            return last;
        }

        [[nodiscard]] bool HasFailed() const { return m_failed; }

    private:
        vpx_codec_ctx_t m_codec{};
        bool m_initialized{false};
        bool m_failed{false};
    };

    bool IsSupportedImage(const vpx_image_t *image)
    {
        if (!image || (image->fmt & VPX_IMG_FMT_HIGHBITDEPTH)) return false;
        if (image->d_w == 0 || image->d_h == 0 || image->d_w > VIDEO_MAX_DIMENSION || image->d_h > VIDEO_MAX_DIMENSION) return false;
        switch (image->fmt)
        {
        case VPX_IMG_FMT_I420:
        case VPX_IMG_FMT_I422:
        case VPX_IMG_FMT_I440:
        case VPX_IMG_FMT_I444:
            return true;
        default:
            return false;
        }
    }

    wxImage ConvertImage(const vpx_image_t *image, const vpx_image_t *alpha)
    {
        const int width = (int)image->d_w;
        const int height = (int)image->d_h;
        wxImage result(width, height, false);
        if (!result.IsOk()) return result;

        if (image->cs == VPX_CS_SRGB)
        {
            // 4:4:4 RGB stored as GBR planes
            unsigned char *out = result.GetData();
            for (int row = 0; row < height; ++row)
            {
                const unsigned char *g = image->planes[VPX_PLANE_Y] + (ptrdiff_t)row * image->stride[VPX_PLANE_Y];
                const unsigned char *b = image->planes[VPX_PLANE_U] + (ptrdiff_t)(row >> image->y_chroma_shift) * image->stride[VPX_PLANE_U];
                const unsigned char *r = image->planes[VPX_PLANE_V] + (ptrdiff_t)(row >> image->y_chroma_shift) * image->stride[VPX_PLANE_V];
                for (int col = 0; col < width; ++col)
                {
                    const int chroma = col >> image->x_chroma_shift;
                    *out++ = r[chroma];
                    *out++ = g[col];
                    *out++ = b[chroma];
                }
            }
        }
        else
        {
            YUVPlanes planes{};
            planes.y = image->planes[VPX_PLANE_Y];
            planes.u = image->planes[VPX_PLANE_U];
            planes.v = image->planes[VPX_PLANE_V];
            planes.yStride = image->stride[VPX_PLANE_Y];
            planes.uStride = image->stride[VPX_PLANE_U];
            planes.vStride = image->stride[VPX_PLANE_V];
            planes.width = width;
            planes.height = height;
            planes.xShift = (int)image->x_chroma_shift;
            planes.yShift = (int)image->y_chroma_shift;
            switch (image->cs)
            {
            case VPX_CS_BT_709: planes.matrix = YUVMatrix::BT709; break;
            case VPX_CS_BT_2020: planes.matrix = YUVMatrix::BT2020; break;
            default: planes.matrix = YUVMatrix::BT601; break;
            }
            planes.fullRange = image->range == VPX_CR_FULL_RANGE;
            ConvertYUVToRGB(planes, result.GetData());
        }

        if (alpha && alpha->d_w == image->d_w && alpha->d_h == image->d_h)
        {
            result.SetAlpha();
            CopyPlane(alpha->planes[VPX_PLANE_Y], alpha->stride[VPX_PLANE_Y], width, height, result.GetAlpha());
        }
        return result;
    }

    class WebMDecoder : public VideoDecoder
    {
    public:
        ~WebMDecoder() override
        {
            if (m_context) nestegg_destroy(m_context);
        }

        bool Open(const wxString &path, wxString &error)
        {
            if (!m_file.Open(path) || !InitDemuxer())
            {
                error = "not a WebM file";
                return false;
            }

            unsigned int tracks;
            if (nestegg_track_count(m_context, &tracks) != 0) return false;
            m_trackCount = tracks;
            bool unsupportedVideo = false;
            for (unsigned int track = 0; track < tracks; ++track)
            {
                const int type = nestegg_track_type(m_context, track);
                const int codec = nestegg_track_codec_id(m_context, track);
                if (type == NESTEGG_TRACK_VIDEO && m_videoTrack == NO_TRACK)
                {
                    if (codec == NESTEGG_CODEC_VP8 || codec == NESTEGG_CODEC_VP9)
                        m_videoTrack = track;
                    else
                        unsupportedVideo = true;
                }
                else if (type == NESTEGG_TRACK_AUDIO && m_audioTrack == NO_TRACK)
                {
                    if (codec == NESTEGG_CODEC_OPUS || codec == NESTEGG_CODEC_VORBIS)
                        m_audioTrack = track;
                }
            }
            if (m_videoTrack == NO_TRACK)
            {
                error = unsupportedVideo ? "unsupported video codec" : "no video track";
                return false;
            }
            if (nestegg_track_encoding(m_context, m_videoTrack) == NESTEGG_ENCODING_ENCRYPTION)
            {
                error = "encrypted video";
                return false;
            }

            nestegg_video_params params;
            if (nestegg_track_video_params(m_context, m_videoTrack, &params) != 0 ||
                params.width == 0 || params.height == 0 ||
                params.width > VIDEO_MAX_DIMENSION || params.height > VIDEO_MAX_DIMENSION)
            {
                error = "invalid video size";
                return false;
            }
            m_size = wxSize((int)params.width, (int)params.height);
            if (params.display_width > 0 && params.display_height > 0 &&
                params.display_width <= VIDEO_MAX_DIMENSION && params.display_height <= VIDEO_MAX_DIMENSION)
            {
                m_size = wxSize((int)params.display_width, (int)params.display_height);
            }

            const int videoCodec = nestegg_track_codec_id(m_context, m_videoTrack);
            if (!m_video.Init(videoCodec, params.width, params.height))
            {
                error = "can't initialize the video decoder";
                return false;
            }
            if (params.alpha_mode && m_alpha.Init(videoCodec, params.width, params.height))
                m_hasAlpha = true;

            if (m_audioTrack != NO_TRACK && !InitAudio())
                m_audioTrack = NO_TRACK; // play the video without sound

            // Decode the first frame to be sure that we can handle the stream
            for (;;)
            {
                const Status status = ReadPacket(m_pendingFrames, m_pendingAudio);
                if (status != Status::Ok)
                {
                    error = "can't decode the video stream";
                    return false;
                }
                if (!m_pendingFrames.empty()) return true;
            }
        }

        [[nodiscard]] wxSize GetSize() const override { return m_size; }
        [[nodiscard]] int GetAudioRate() const override { return m_audio ? m_audio->GetRate() : 0; }
        [[nodiscard]] int GetAudioChannels() const override { return m_audio ? m_audio->GetChannels() : 0; }

        Status Decode(std::vector<VideoFrame> &frames, std::vector<float> &audio) override
        {
            if (!m_pendingFrames.empty() || !m_pendingAudio.empty())
            {
                std::move(m_pendingFrames.begin(), m_pendingFrames.end(), std::back_inserter(frames));
                audio.insert(audio.end(), m_pendingAudio.begin(), m_pendingAudio.end());
                m_pendingFrames.clear();
                m_pendingAudio.clear();
                return Status::Ok;
            }
            const size_t framesBefore = frames.size();
            const size_t audioBefore = audio.size();
            while (frames.size() == framesBefore && audio.size() == audioBefore)
            {
                if (const Status status = ReadPacket(frames, audio); status != Status::Ok)
                    return status;
            }
            return Status::Ok;
        }

        bool Rewind() override
        {
            m_pendingFrames.clear();
            m_pendingAudio.clear();
            if (m_context)
            {
                nestegg_destroy(m_context);
                m_context = nullptr;
            }
            unsigned int tracks;
            if (!m_file.Rewind() || !InitDemuxer() ||
                nestegg_track_count(m_context, &tracks) != 0 || tracks != m_trackCount)
            {
                return false;
            }
            if (m_audio && !m_audio->Reset())
                m_audio.reset();
            return true;
        }

    private:
        bool InitDemuxer()
        {
            return nestegg_init(&m_context, m_file.GetIO(), nullptr, -1) == 0;
        }

        bool InitAudio()
        {
            if (nestegg_track_encoding(m_context, m_audioTrack) == NESTEGG_ENCODING_ENCRYPTION) return false;
            if (nestegg_track_codec_id(m_context, m_audioTrack) == NESTEGG_CODEC_OPUS)
            {
                unsigned char *data;
                size_t size;
                if (nestegg_track_codec_data(m_context, m_audioTrack, 0, &data, &size) != 0) return false;
                auto opus = std::make_unique<OpusCodec>();
                if (!opus->Init(data, size)) return false;
                m_audio = std::move(opus);
            }
            else
            {
                auto vorbis = std::make_unique<VorbisCodec>();
                if (!vorbis->Init(m_context, m_audioTrack)) return false;
                m_audio = std::move(vorbis);
            }
            return true;
        }

        Status ReadPacket(std::vector<VideoFrame> &frames, std::vector<float> &audio)
        {
            nestegg_packet *packet;
            const int result = nestegg_read_packet(m_context, &packet);
            if (result == 0) return Status::End;
            if (result < 0) return Status::Error;
            const std::unique_ptr<nestegg_packet, decltype(&nestegg_free_packet)> holder(packet, &nestegg_free_packet);

            unsigned int track, count;
            if (nestegg_packet_track(packet, &track) != 0 || nestegg_packet_count(packet, &count) != 0)
                return Status::Error;

            if (track == m_videoTrack)
            {
                uint64_t timestamp = 0;
                nestegg_packet_tstamp(packet, &timestamp);
                for (unsigned int item = 0; item < count; ++item)
                {
                    unsigned char *data;
                    size_t size;
                    if (nestegg_packet_data(packet, item, &data, &size) != 0) return Status::Error;
                    vpx_image_t *image = m_video.Decode(data, size);
                    if (m_video.HasFailed()) return Status::Error;

                    vpx_image_t *alpha = nullptr;
                    unsigned char *alphaData;
                    size_t alphaSize;
                    if (m_hasAlpha && nestegg_packet_additional_data(packet, BLOCK_ADDITIONAL_ALPHA, &alphaData, &alphaSize) == 0)
                    {
                        alpha = m_alpha.Decode(alphaData, alphaSize);
                        if (m_alpha.HasFailed()) m_hasAlpha = false;
                    }

                    if (!image) continue;
                    if (!IsSupportedImage(image)) return Status::Error;
                    if (alpha && !IsSupportedImage(alpha)) alpha = nullptr;
                    VideoFrame frame;
                    frame.image = ConvertImage(image, alpha);
                    if (!frame.image.IsOk()) return Status::Error;
                    frame.time = (double)timestamp / 1e9;
                    frames.push_back(std::move(frame));
                }
            }
            else if (track == m_audioTrack && m_audio)
            {
                int64_t discardPadding = 0;
                nestegg_packet_discard_padding(packet, &discardPadding);
                for (unsigned int item = 0; item < count; ++item)
                {
                    unsigned char *data;
                    size_t size;
                    if (nestegg_packet_data(packet, item, &data, &size) != 0) break;
                    m_audio->Decode(data, size, item + 1 == count ? discardPadding : 0, audio);
                }
            }
            return Status::Ok;
        }

        WebMFile m_file;
        nestegg *m_context{nullptr};
        unsigned int m_trackCount{0};
        unsigned int m_videoTrack{NO_TRACK};
        unsigned int m_audioTrack{NO_TRACK};
        VpxCodec m_video;
        VpxCodec m_alpha;
        bool m_hasAlpha{false};
        std::unique_ptr<AudioCodec> m_audio;
        wxSize m_size;
        std::vector<VideoFrame> m_pendingFrames;
        std::vector<float> m_pendingAudio;
    };
}

std::unique_ptr<VideoDecoder> CreateWebMDecoder(const wxString &path, wxString &error)
{
    auto decoder = std::make_unique<WebMDecoder>();
    if (!decoder->Open(path, error)) return nullptr;
    return decoder;
}
