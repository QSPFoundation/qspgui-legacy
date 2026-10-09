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

#ifndef SOUND_ENGINE_H
#define SOUND_ENGINE_H

    #include <stddef.h>

    typedef void *ma_sound_file;
    typedef struct sound_stream sound_stream;

    #ifdef __cplusplus
    extern "C"
    {
    #endif

    int sound_init_engine();
    void sound_free_engine();

    int soundfont_init(const char *filePath);
    int soundfont_init_w(const wchar_t *filePath);

    ma_sound_file sound_play_file(const char *file);
    ma_sound_file sound_play_file_w(const wchar_t *file);
    void sound_close_file(ma_sound_file sound);
    void sound_set_volume(ma_sound_file sound, float volume);
    int sound_is_playing(ma_sound_file sound);

    /* A stream of interleaved float samples pushed by the application (e.g. video soundtracks).
       One thread writes, the audio thread reads; the stream plays silence when it runs dry. */
    sound_stream *sound_stream_create(unsigned int channels, unsigned int sampleRate, unsigned int capacityFrames);
    void sound_stream_free(sound_stream *stream);
    unsigned int sound_stream_write(sound_stream *stream, const float *samples, unsigned int frameCount);
    unsigned int sound_stream_get_queued(sound_stream *stream);
    unsigned long long sound_stream_get_played(sound_stream *stream); /* frames played, without the silence */
    void sound_stream_start(sound_stream *stream);
    void sound_stream_stop(sound_stream *stream);
    void sound_stream_set_volume(sound_stream *stream, float volume);

    #ifdef __cplusplus
    }
    #endif

#endif
