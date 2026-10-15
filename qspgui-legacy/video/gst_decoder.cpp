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

// System decoder for Linux and other Unix systems.
// GStreamer is loaded at runtime, so it stays an optional dependency: without it
// (or without the codec plugins) only the built-in WebM decoder works.

#include "video_decoder.h"
#include "yuv.h"
#include <dlfcn.h>
#include <cstdint>
#include <cstring>
#include <mutex>

namespace
{
    // The parts of the GStreamer 1.x ABI we need

    using gboolean = int;
    using GstElement = void;
    using GstPad = void;
    using GstCaps = void;
    using GstStructure = void;
    using GstBus = void;
    using GstMessage = void;
    using GstSample = void;
    using GstElementFactory = void;

    struct GError
    {
        uint32_t domain;
        int code;
        char *message;
    };

    struct GstMiniObject
    {
        size_t type;
        int refcount;
        int lockstate;
        unsigned int flags;
        void *copy;
        void *dispose;
        void *free;
        unsigned int privUint;
        void *privPointer;
    };

    struct GstBuffer
    {
        GstMiniObject miniObject;
        void *pool;
        uint64_t pts;
        uint64_t dts;
        uint64_t duration;
        uint64_t offset;
        uint64_t offsetEnd;
    };

    struct GstMapInfo
    {
        void *memory;
        int flags;
        uint8_t *data;
        size_t size;
        size_t maxSize;
        void *userData[4];
        void *reserved[4];
    };

    constexpr int GST_STATE_NULL = 1;
    constexpr int GST_STATE_PAUSED = 3;
    constexpr int GST_STATE_PLAYING = 4;
    constexpr int GST_STATE_CHANGE_FAILURE = 0;
    constexpr int GST_STATE_CHANGE_ASYNC = 2;
    constexpr int GST_MESSAGE_ERROR = 1 << 1;
    constexpr int GST_MAP_READ = 1;
    constexpr int GST_FORMAT_TIME = 3;
    constexpr int GST_SEEK_FLAG_FLUSH = 1 << 0;
    constexpr int GST_SEEK_FLAG_KEY_UNIT = 1 << 2;
    constexpr int GST_PAD_LINK_OK = 0;
    constexpr int GST_AUTOPLUG_SELECT_TRY = 0;
    constexpr int GST_AUTOPLUG_SELECT_SKIP = 2;
    constexpr uint64_t GST_CLOCK_TIME_NONE = UINT64_MAX;
    constexpr uint64_t GST_MSECOND = 1000000;
    constexpr uint64_t OPEN_TIMEOUT = 10000 * GST_MSECOND;
    constexpr uint64_t PULL_TIMEOUT = 20 * GST_MSECOND;

    struct GstApi
    {
        gboolean (*gst_init_check)(int *, char ***, GError **);
        GstElement *(*gst_pipeline_new)(const char *);
        GstElement *(*gst_element_factory_make)(const char *, const char *);
        gboolean (*gst_bin_add)(GstElement *, GstElement *);
        gboolean (*gst_element_link)(GstElement *, GstElement *);
        int (*gst_element_set_state)(GstElement *, int);
        int (*gst_element_get_state)(GstElement *, int *, int *, uint64_t);
        gboolean (*gst_element_sync_state_with_parent)(GstElement *);
        GstPad *(*gst_element_get_static_pad)(GstElement *, const char *);
        GstBus *(*gst_element_get_bus)(GstElement *);
        gboolean (*gst_element_seek_simple)(GstElement *, int, int, int64_t);
        const char *(*gst_element_factory_get_metadata)(GstElementFactory *, const char *);
        int (*gst_pad_link)(GstPad *, GstPad *);
        GstCaps *(*gst_pad_get_current_caps)(GstPad *);
        GstCaps *(*gst_pad_query_caps)(GstPad *, GstCaps *);
        unsigned int (*gst_caps_get_size)(const GstCaps *);
        GstStructure *(*gst_caps_get_structure)(const GstCaps *, unsigned int);
        GstCaps *(*gst_caps_from_string)(const char *);
        const char *(*gst_structure_get_name)(const GstStructure *);
        gboolean (*gst_structure_get_int)(const GstStructure *, const char *, int *);
        void (*gst_mini_object_unref)(void *);
        void (*gst_object_unref)(void *);
        GstMessage *(*gst_bus_pop_filtered)(GstBus *, int);
        void (*gst_message_parse_error)(GstMessage *, GError **, char **);
        GstBuffer *(*gst_sample_get_buffer)(GstSample *);
        GstCaps *(*gst_sample_get_caps)(GstSample *);
        gboolean (*gst_buffer_map)(GstBuffer *, GstMapInfo *, int);
        void (*gst_buffer_unmap)(GstBuffer *, GstMapInfo *);

        GstSample *(*gst_app_sink_try_pull_sample)(GstElement *, uint64_t);
        gboolean (*gst_app_sink_is_eos)(GstElement *);
        void (*gst_app_sink_set_caps)(GstElement *, const GstCaps *);
        void (*gst_app_sink_set_max_buffers)(GstElement *, unsigned int);

        void (*g_object_set)(void *, const char *, ...);
        unsigned long (*g_signal_connect_data)(void *, const char *, void (*)(), void *, void *, int);
        void (*g_error_free)(GError *);
        void (*g_free)(void *);

        bool isLoaded{false};
        wxString error;
    };

    template <typename Function>
    bool LoadSymbol(void *library, const char *name, Function &function)
    {
        function = reinterpret_cast<Function>(dlsym(library, name));
        return function != nullptr;
    }

    const GstApi &GetApi()
    {
        static GstApi api{};
        static std::once_flag once;
        std::call_once(once, [] {
            void *gst = dlopen("libgstreamer-1.0.so.0", RTLD_NOW | RTLD_LOCAL);
            void *app = gst ? dlopen("libgstapp-1.0.so.0", RTLD_NOW | RTLD_LOCAL) : nullptr;
            void *gobject = gst ? dlopen("libgobject-2.0.so.0", RTLD_NOW | RTLD_LOCAL) : nullptr;
            void *glib = gst ? dlopen("libglib-2.0.so.0", RTLD_NOW | RTLD_LOCAL) : nullptr;
            if (!gst || !app || !gobject || !glib)
            {
                api.error = "GStreamer isn't installed";
                return;
            }
            const bool isComplete =
                LoadSymbol(gst, "gst_init_check", api.gst_init_check) &&
                LoadSymbol(gst, "gst_pipeline_new", api.gst_pipeline_new) &&
                LoadSymbol(gst, "gst_element_factory_make", api.gst_element_factory_make) &&
                LoadSymbol(gst, "gst_bin_add", api.gst_bin_add) &&
                LoadSymbol(gst, "gst_element_link", api.gst_element_link) &&
                LoadSymbol(gst, "gst_element_set_state", api.gst_element_set_state) &&
                LoadSymbol(gst, "gst_element_get_state", api.gst_element_get_state) &&
                LoadSymbol(gst, "gst_element_sync_state_with_parent", api.gst_element_sync_state_with_parent) &&
                LoadSymbol(gst, "gst_element_get_static_pad", api.gst_element_get_static_pad) &&
                LoadSymbol(gst, "gst_element_get_bus", api.gst_element_get_bus) &&
                LoadSymbol(gst, "gst_element_seek_simple", api.gst_element_seek_simple) &&
                LoadSymbol(gst, "gst_element_factory_get_metadata", api.gst_element_factory_get_metadata) &&
                LoadSymbol(gst, "gst_pad_link", api.gst_pad_link) &&
                LoadSymbol(gst, "gst_pad_get_current_caps", api.gst_pad_get_current_caps) &&
                LoadSymbol(gst, "gst_pad_query_caps", api.gst_pad_query_caps) &&
                LoadSymbol(gst, "gst_caps_get_size", api.gst_caps_get_size) &&
                LoadSymbol(gst, "gst_caps_get_structure", api.gst_caps_get_structure) &&
                LoadSymbol(gst, "gst_caps_from_string", api.gst_caps_from_string) &&
                LoadSymbol(gst, "gst_structure_get_name", api.gst_structure_get_name) &&
                LoadSymbol(gst, "gst_structure_get_int", api.gst_structure_get_int) &&
                LoadSymbol(gst, "gst_mini_object_unref", api.gst_mini_object_unref) &&
                LoadSymbol(gst, "gst_object_unref", api.gst_object_unref) &&
                LoadSymbol(gst, "gst_bus_pop_filtered", api.gst_bus_pop_filtered) &&
                LoadSymbol(gst, "gst_message_parse_error", api.gst_message_parse_error) &&
                LoadSymbol(gst, "gst_sample_get_buffer", api.gst_sample_get_buffer) &&
                LoadSymbol(gst, "gst_sample_get_caps", api.gst_sample_get_caps) &&
                LoadSymbol(gst, "gst_buffer_map", api.gst_buffer_map) &&
                LoadSymbol(gst, "gst_buffer_unmap", api.gst_buffer_unmap) &&
                LoadSymbol(app, "gst_app_sink_try_pull_sample", api.gst_app_sink_try_pull_sample) &&
                LoadSymbol(app, "gst_app_sink_is_eos", api.gst_app_sink_is_eos) &&
                LoadSymbol(app, "gst_app_sink_set_caps", api.gst_app_sink_set_caps) &&
                LoadSymbol(app, "gst_app_sink_set_max_buffers", api.gst_app_sink_set_max_buffers) &&
                LoadSymbol(gobject, "g_object_set", api.g_object_set) &&
                LoadSymbol(gobject, "g_signal_connect_data", api.g_signal_connect_data) &&
                LoadSymbol(glib, "g_error_free", api.g_error_free) &&
                LoadSymbol(glib, "g_free", api.g_free);
            if (!isComplete)
            {
                api.error = "unsupported GStreamer version";
                return;
            }
            GError *error = nullptr;
            if (!api.gst_init_check(nullptr, nullptr, &error))
            {
                api.error = wxString::Format("can't initialize GStreamer: %s", error ? error->message : "");
                if (error) api.g_error_free(error);
                return;
            }
            api.isLoaded = true;
        });
        return api;
    }

    class GstDecoder : public VideoDecoder
    {
    public:
        explicit GstDecoder(const GstApi &api) : m_api(api) {}

        ~GstDecoder() override
        {
            if (m_pipeline)
            {
                m_api.gst_element_set_state(m_pipeline, GST_STATE_NULL);
                m_api.gst_object_unref(m_pipeline);
            }
        }

        bool Open(const wxString &path, wxString &error)
        {
            m_pipeline = m_api.gst_pipeline_new(nullptr);
            GstElement *source = m_api.gst_element_factory_make("filesrc", nullptr);
            GstElement *decoder = m_api.gst_element_factory_make("decodebin", nullptr);
            if (!m_pipeline || !source || !decoder)
            {
                error = "GStreamer base plugins aren't installed";
                return false;
            }
            m_api.gst_bin_add(m_pipeline, source);
            m_api.gst_bin_add(m_pipeline, decoder);
            m_api.g_object_set(source, "location", static_cast<const char *>(path.fn_str()), nullptr);
            m_api.g_signal_connect_data(decoder, "autoplug-select", reinterpret_cast<void (*)()>(&OnAutoplugSelect), this, nullptr, 0);
            m_api.g_signal_connect_data(decoder, "pad-added", reinterpret_cast<void (*)()>(&OnPadAdded), this, nullptr, 0);
            if (!m_api.gst_element_link(source, decoder))
            {
                error = "can't build the GStreamer pipeline";
                return false;
            }

            // Wait until both branches have the first data
            int state = m_api.gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
            if (state != GST_STATE_CHANGE_FAILURE)
                state = m_api.gst_element_get_state(m_pipeline, nullptr, nullptr, OPEN_TIMEOUT);
            if (state == GST_STATE_CHANGE_FAILURE || state == GST_STATE_CHANGE_ASYNC)
            {
                error = GetBusError();
                if (error.IsEmpty()) error = "GStreamer can't play the file";
                return false;
            }

            std::lock_guard lock(m_mutex);
            if (!m_videoSink || !ReadVideoCaps())
            {
                error = "GStreamer found no video in the file";
                return false;
            }
            if (m_audioSink && !ReadAudioCaps())
                m_audioSink = nullptr;
            if (m_api.gst_element_set_state(m_pipeline, GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE)
            {
                error = GetBusError();
                return false;
            }
            return true;
        }

        [[nodiscard]] wxSize GetSize() const override { return m_size; }
        [[nodiscard]] int GetAudioRate() const override { return m_audioSink ? m_audioRate : 0; }
        [[nodiscard]] int GetAudioChannels() const override { return m_audioSink ? m_audioChannels : 0; }

        Status Decode(std::vector<VideoFrame> &frames, std::vector<float> &audio) override
        {
            bool isPulled = false;
            if (m_audioSink && !m_isAudioDone)
                isPulled |= PullAudio(0, audio);
            if (!m_isVideoDone)
                isPulled |= PullVideo(isPulled ? 0 : PULL_TIMEOUT, frames);
            if (isPulled) return Status::Ok;

            if (const wxString error = GetBusError(); !error.IsEmpty()) return Status::Error;
            m_isVideoDone = m_api.gst_app_sink_is_eos(m_videoSink);
            m_isAudioDone = !m_audioSink || m_api.gst_app_sink_is_eos(m_audioSink);
            if (m_isVideoDone && m_isAudioDone) return Status::End;
            if (m_isVideoDone) PullAudio(PULL_TIMEOUT, audio);
            return Status::Ok;
        }

        bool Rewind() override
        {
            m_isVideoDone = m_isAudioDone = false;
            return m_api.gst_element_seek_simple(m_pipeline, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 0);
        }

    private:
        static int OnAutoplugSelect(GstElement *, GstPad *, GstCaps *, GstElementFactory *factory, void *data)
        {
            // Games may only play local files: no network sources and no HLS/DASH playlists
            const auto *decoder = static_cast<GstDecoder *>(data);
            const char *klass = decoder->m_api.gst_element_factory_get_metadata(factory, "klass");
            if (klass && (std::strstr(klass, "Source") || std::strstr(klass, "Network") || std::strstr(klass, "Adaptive")))
                return GST_AUTOPLUG_SELECT_SKIP;
            return GST_AUTOPLUG_SELECT_TRY;
        }

        static void OnPadAdded(GstElement *, GstPad *pad, void *data)
        {
            static_cast<GstDecoder *>(data)->AddBranch(pad);
        }

        void AddBranch(GstPad *pad)
        {
            const GstApi &api = m_api;
            GstCaps *caps = api.gst_pad_get_current_caps(pad);
            if (!caps) caps = api.gst_pad_query_caps(pad, nullptr);
            if (!caps) return;
            wxString name;
            if (api.gst_caps_get_size(caps) > 0)
                name = api.gst_structure_get_name(api.gst_caps_get_structure(caps, 0));
            api.gst_mini_object_unref(caps);

            std::lock_guard lock(m_mutex);
            const bool isVideo = name.StartsWith("video/");
            if (isVideo ? m_videoSink != nullptr : (!name.StartsWith("audio/") || m_audioSink != nullptr))
                return;

            GstElement *convert = api.gst_element_factory_make(isVideo ? "videoconvert" : "audioconvert", nullptr);
            GstElement *resample = isVideo ? nullptr : api.gst_element_factory_make("audioresample", nullptr);
            GstElement *sink = api.gst_element_factory_make("appsink", nullptr);
            if (!convert || !sink || (!isVideo && !resample)) return;

            GstCaps *sinkCaps = api.gst_caps_from_string(isVideo ?
                "video/x-raw,format=RGBA" :
                "audio/x-raw,format=F32LE,layout=interleaved,channels=(int)[1,8]");
            api.gst_app_sink_set_caps(sink, sinkCaps);
            api.gst_mini_object_unref(sinkCaps);
            api.gst_app_sink_set_max_buffers(sink, isVideo ? 4 : 64);
            api.g_object_set(sink, "sync", 0, nullptr);

            api.gst_bin_add(m_pipeline, convert);
            api.gst_bin_add(m_pipeline, sink);
            if (resample)
            {
                api.gst_bin_add(m_pipeline, resample);
                if (!api.gst_element_link(convert, resample) || !api.gst_element_link(resample, sink)) return;
            }
            else if (!api.gst_element_link(convert, sink))
            {
                return;
            }
            api.gst_element_sync_state_with_parent(sink);
            if (resample) api.gst_element_sync_state_with_parent(resample);
            api.gst_element_sync_state_with_parent(convert);

            GstPad *sinkPad = api.gst_element_get_static_pad(convert, "sink");
            const bool isLinked = sinkPad && api.gst_pad_link(pad, sinkPad) == GST_PAD_LINK_OK;
            if (sinkPad) api.gst_object_unref(sinkPad);
            if (isLinked) (isVideo ? m_videoSink : m_audioSink) = sink;
        }

        bool ReadCaps(GstElement *sink, const char *first, int &firstValue, const char *second, int &secondValue) const
        {
            GstPad *pad = m_api.gst_element_get_static_pad(sink, "sink");
            if (!pad) return false;
            GstCaps *caps = m_api.gst_pad_get_current_caps(pad);
            m_api.gst_object_unref(pad);
            if (!caps) return false;
            bool isRead = false;
            if (m_api.gst_caps_get_size(caps) > 0)
            {
                const GstStructure *structure = m_api.gst_caps_get_structure(caps, 0);
                isRead = m_api.gst_structure_get_int(structure, first, &firstValue) &&
                    m_api.gst_structure_get_int(structure, second, &secondValue);
            }
            m_api.gst_mini_object_unref(caps);
            return isRead;
        }

        bool ReadVideoCaps()
        {
            int width, height;
            if (!ReadCaps(m_videoSink, "width", width, "height", height)) return false;
            if (width <= 0 || height <= 0 || width > VIDEO_MAX_DIMENSION || height > VIDEO_MAX_DIMENSION) return false;
            m_size = wxSize(width, height);
            return true;
        }

        bool ReadAudioCaps()
        {
            return ReadCaps(m_audioSink, "rate", m_audioRate, "channels", m_audioChannels) &&
                m_audioRate > 0 && m_audioChannels > 0 && m_audioChannels <= 8;
        }

        bool PullVideo(const uint64_t timeout, std::vector<VideoFrame> &frames)
        {
            GstSample *sample = m_api.gst_app_sink_try_pull_sample(m_videoSink, timeout);
            if (!sample) return false;
            GstBuffer *buffer = m_api.gst_sample_get_buffer(sample);
            GstCaps *caps = m_api.gst_sample_get_caps(sample);
            int width = 0, height = 0;
            if (caps && m_api.gst_caps_get_size(caps) > 0)
            {
                const GstStructure *structure = m_api.gst_caps_get_structure(caps, 0);
                m_api.gst_structure_get_int(structure, "width", &width);
                m_api.gst_structure_get_int(structure, "height", &height);
            }
            GstMapInfo map{};
            if (buffer && width > 0 && height > 0 && width <= VIDEO_MAX_DIMENSION && height <= VIDEO_MAX_DIMENSION &&
                m_api.gst_buffer_map(buffer, &map, GST_MAP_READ))
            {
                if (map.size >= (size_t)width * height * 4)
                {
                    VideoFrame frame;
                    frame.image.Create(width, height, false);
                    if (frame.image.IsOk())
                    {
                        ConvertPackedToRGB(map.data, width * 4, width, height, false, frame.image.GetData());
                        if (buffer->pts != GST_CLOCK_TIME_NONE) m_lastTime = (double)buffer->pts / 1e9;
                        frame.time = m_lastTime;
                        frames.push_back(std::move(frame));
                    }
                }
                m_api.gst_buffer_unmap(buffer, &map);
            }
            m_api.gst_mini_object_unref(sample);
            return true;
        }

        bool PullAudio(const uint64_t timeout, std::vector<float> &audio)
        {
            GstSample *sample = m_api.gst_app_sink_try_pull_sample(m_audioSink, timeout);
            if (!sample) return false;
            GstBuffer *buffer = m_api.gst_sample_get_buffer(sample);
            GstMapInfo map{};
            if (buffer && m_api.gst_buffer_map(buffer, &map, GST_MAP_READ))
            {
                const size_t count = map.size / sizeof(float) / m_audioChannels * m_audioChannels;
                const size_t start = audio.size();
                audio.resize(start + count);
                std::memcpy(audio.data() + start, map.data, count * sizeof(float));
                m_api.gst_buffer_unmap(buffer, &map);
            }
            m_api.gst_mini_object_unref(sample);
            return true;
        }

        wxString GetBusError() const
        {
            GstBus *bus = m_api.gst_element_get_bus(m_pipeline);
            if (!bus) return wxEmptyString;
            wxString result;
            if (GstMessage *message = m_api.gst_bus_pop_filtered(bus, GST_MESSAGE_ERROR))
            {
                GError *error = nullptr;
                char *debug = nullptr;
                m_api.gst_message_parse_error(message, &error, &debug);
                result = error && error->message ? wxString::FromUTF8(error->message) : wxString("GStreamer error");
                if (error) m_api.g_error_free(error);
                if (debug) m_api.g_free(debug);
                m_api.gst_mini_object_unref(message);
            }
            m_api.gst_object_unref(bus);
            return result;
        }

        const GstApi &m_api;
        std::mutex m_mutex; // the branches are added from a streaming thread
        GstElement *m_pipeline{nullptr};
        GstElement *m_videoSink{nullptr};
        GstElement *m_audioSink{nullptr};
        wxSize m_size;
        int m_audioRate{0};
        int m_audioChannels{0};
        bool m_isVideoDone{false};
        bool m_isAudioDone{false};
        double m_lastTime{0.0};
    };
}

std::unique_ptr<VideoDecoder> CreateSystemDecoder(const wxString &path, wxString &error)
{
    const GstApi &api = GetApi();
    if (!api.isLoaded)
    {
        error = api.error;
        return nullptr;
    }
    auto decoder = std::make_unique<GstDecoder>(api);
    if (!decoder->Open(path, error)) return nullptr;
    return decoder;
}
