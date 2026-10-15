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

// System decoder for macOS, based on AVAssetReader. Compiled with ARC.

#include "video_decoder.h"
#include "yuv.h"
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>

namespace
{
    constexpr int AUDIO_RATE = 48000;
    constexpr int AUDIO_CHANNELS = 2;

    class AVFDecoder : public VideoDecoder
    {
    public:
        ~AVFDecoder() override
        {
            @autoreleasepool
            {
                [m_reader cancelReading];
            }
        }

        bool Open(const wxString &path, wxString &error)
        {
            @autoreleasepool
            {
                NSString *filePath = [NSString stringWithUTF8String:path.utf8_str()];
                if (!filePath)
                {
                    error = "invalid file name";
                    return false;
                }
                m_asset = [AVURLAsset URLAssetWithURL:[NSURL fileURLWithPath:filePath] options:nil];
                m_videoTrack = [[m_asset tracksWithMediaType:AVMediaTypeVideo] firstObject];
                m_audioTrack = [[m_asset tracksWithMediaType:AVMediaTypeAudio] firstObject];
                if (!m_videoTrack)
                {
                    error = "AVFoundation found no video in the file";
                    return false;
                }
                const CGSize size = m_videoTrack.naturalSize;
                if (size.width < 1 || size.height < 1 || size.width > VIDEO_MAX_DIMENSION || size.height > VIDEO_MAX_DIMENSION)
                {
                    error = "invalid video size";
                    return false;
                }
                m_size = wxSize((int)size.width, (int)size.height);
                if (!StartReading())
                {
                    error = "AVFoundation can't decode the file";
                    return false;
                }
                return true;
            }
        }

        [[nodiscard]] wxSize GetSize() const override { return m_size; }
        [[nodiscard]] int GetAudioRate() const override { return m_audioTrack ? AUDIO_RATE : 0; }
        [[nodiscard]] int GetAudioChannels() const override { return m_audioTrack ? AUDIO_CHANNELS : 0; }

        Status Decode(std::vector<VideoFrame> &frames, std::vector<float> &audio) override
        {
            @autoreleasepool
            {
                // Read the track that is behind, so that neither of them stalls the reader
                const bool toReadAudio = m_audioOutput && !m_isAudioDone && (m_isVideoDone || m_audioTime <= m_videoTime);
                if (toReadAudio)
                {
                    CMSampleBufferRef buffer = [m_audioOutput copyNextSampleBuffer];
                    if (buffer)
                    {
                        ReadAudio(buffer, audio);
                        CFRelease(buffer);
                    }
                    else
                    {
                        m_isAudioDone = true;
                    }
                }
                else if (!m_isVideoDone)
                {
                    CMSampleBufferRef buffer = [m_videoOutput copyNextSampleBuffer];
                    if (buffer)
                    {
                        const bool isRead = ReadVideo(buffer, frames);
                        CFRelease(buffer);
                        if (!isRead) return Status::Error;
                    }
                    else
                    {
                        m_isVideoDone = true;
                    }
                }

                if (m_reader.status == AVAssetReaderStatusFailed) return Status::Error;
                if (m_isVideoDone && (!m_audioOutput || m_isAudioDone)) return Status::End;
                return Status::Ok;
            }
        }

        bool Rewind() override
        {
            @autoreleasepool
            {
                // AVAssetReader can't seek, so read the asset again
                [m_reader cancelReading];
                return StartReading();
            }
        }

    private:
        bool StartReading()
        {
            m_reader = nil;
            m_videoOutput = nil;
            m_audioOutput = nil;
            m_isVideoDone = m_isAudioDone = false;
            m_videoTime = m_audioTime = 0.0;

            NSError *readerError = nil;
            m_reader = [AVAssetReader assetReaderWithAsset:m_asset error:&readerError];
            if (!m_reader) return false;

            NSDictionary *videoSettings = @{ (id)kCVPixelBufferPixelFormatTypeKey: @(kCVPixelFormatType_32BGRA) };
            m_videoOutput = [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:m_videoTrack outputSettings:videoSettings];
            m_videoOutput.alwaysCopiesSampleData = NO;
            if (![m_reader canAddOutput:m_videoOutput]) return false;
            [m_reader addOutput:m_videoOutput];

            if (m_audioTrack)
            {
                NSDictionary *audioSettings = @{
                    AVFormatIDKey: @(kAudioFormatLinearPCM),
                    AVSampleRateKey: @(AUDIO_RATE),
                    AVNumberOfChannelsKey: @(AUDIO_CHANNELS),
                    AVLinearPCMBitDepthKey: @32,
                    AVLinearPCMIsFloatKey: @YES,
                    AVLinearPCMIsNonInterleaved: @NO,
                    AVLinearPCMIsBigEndianKey: @NO
                };
                AVAssetReaderTrackOutput *audioOutput =
                    [AVAssetReaderTrackOutput assetReaderTrackOutputWithTrack:m_audioTrack outputSettings:audioSettings];
                if ([m_reader canAddOutput:audioOutput])
                {
                    [m_reader addOutput:audioOutput];
                    m_audioOutput = audioOutput;
                }
            }
            if (!m_audioOutput) m_audioTrack = nil;

            return [m_reader startReading];
        }

        bool ReadVideo(CMSampleBufferRef buffer, std::vector<VideoFrame> &frames)
        {
            const CMTime time = CMSampleBufferGetPresentationTimeStamp(buffer);
            if (CMTIME_IS_NUMERIC(time)) m_videoTime = CMTimeGetSeconds(time);

            CVImageBufferRef image = CMSampleBufferGetImageBuffer(buffer);
            if (!image) return true; // some samples carry no picture
            if (CVPixelBufferLockBaseAddress(image, kCVPixelBufferLock_ReadOnly) != kCVReturnSuccess) return false;

            const int width = (int)CVPixelBufferGetWidth(image);
            const int height = (int)CVPixelBufferGetHeight(image);
            const int stride = (int)CVPixelBufferGetBytesPerRow(image);
            const uint8_t *data = static_cast<const uint8_t *>(CVPixelBufferGetBaseAddress(image));
            bool isRead = false;
            if (data && width > 0 && height > 0 && width <= VIDEO_MAX_DIMENSION && height <= VIDEO_MAX_DIMENSION && stride >= width * 4)
            {
                VideoFrame frame;
                frame.time = m_videoTime;
                frame.image.Create(width, height, false);
                if (frame.image.IsOk())
                {
                    ConvertPackedToRGB(data, stride, width, height, true, frame.image.GetData());
                    frames.push_back(std::move(frame));
                    isRead = true;
                }
            }
            CVPixelBufferUnlockBaseAddress(image, kCVPixelBufferLock_ReadOnly);
            return isRead;
        }

        void ReadAudio(CMSampleBufferRef buffer, std::vector<float> &audio)
        {
            const CMTime time = CMSampleBufferGetPresentationTimeStamp(buffer);
            if (CMTIME_IS_NUMERIC(time)) m_audioTime = CMTimeGetSeconds(time);

            CMBlockBufferRef block = CMSampleBufferGetDataBuffer(buffer);
            if (!block) return;
            const size_t length = CMBlockBufferGetDataLength(block);
            const size_t count = length / sizeof(float) / AUDIO_CHANNELS * AUDIO_CHANNELS;
            const size_t start = audio.size();
            audio.resize(start + count);
            if (CMBlockBufferCopyDataBytes(block, 0, count * sizeof(float), audio.data() + start) != kCMBlockBufferNoErr)
                audio.resize(start);
        }

        AVURLAsset *m_asset{nil};
        AVAssetTrack *m_videoTrack{nil};
        AVAssetTrack *m_audioTrack{nil};
        AVAssetReader *m_reader{nil};
        AVAssetReaderTrackOutput *m_videoOutput{nil};
        AVAssetReaderTrackOutput *m_audioOutput{nil};
        wxSize m_size;
        bool m_isVideoDone{false};
        bool m_isAudioDone{false};
        double m_videoTime{0.0};
        double m_audioTime{0.0};
    };
}

std::unique_ptr<VideoDecoder> CreateSystemDecoder(const wxString &path, wxString &error)
{
    auto decoder = std::make_unique<AVFDecoder>();
    if (!decoder->Open(path, error)) return nullptr;
    return decoder;
}
