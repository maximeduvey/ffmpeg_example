#pragma once

#include <vector>
#include <iostream>
#include <string>

class AVFormatContext;
class AVCodecContext;
class AVStream;
class AVCodec;
class AVFrame;
class AVPacket;
class AVOutputFormat;

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

class ffmpeg_example {
private:
    const AVCodec* encoder = nullptr;
    AVCodecContext* encoder_context = nullptr;
    const AVCodec* decoder = nullptr;
    AVCodecContext* decoder_context = nullptr;

    AVFormatContext* output_format_context = nullptr;
    AVStream* video_stream = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;

    const char* _output = "output.264";
    const char* _ouput_format = "h264";

public:

    int _video_fps = 25;
    int _width = 1920;
    int _height = 1080;
    int _bitrate = 400000;

    ffmpeg_example();
    virtual ~ffmpeg_example();

    int example_create_video_with_fake_image();
    int example_create_video_from_images();

    int encode_video_to_x264(std::vector<std::string> frames, const char* ouput);

private:
    int initMPEGDecoder();
    int initFfmpegcontext(const std::string codec_name);
    AVFrame* create_frame(int width, int height);
    void set_codec_params(AVFormatContext*& fctx, AVCodecContext*& codec_ctx, double width, double height, int fps, int bitrate);
    AVFrame* decode_jpeg_to_frame(const std::string& jpegData);
    AVStream* create_stream(AVFormatContext*& fctx, AVCodecContext*& codec_ctx);

    void fill_frame_with_fake_yuv_image(AVFrame* frame, int frame_index, int width, int height);
    void fill_frames_with_image(const std::string &image, AVFrame* frame);
    AVFrame* scale_frame(AVFrame* frame, AVCodecContext* codec_context);

    void cleanup();
    void set_metadata();
    void dump_stream_info(AVStream* stream);
    void dump_frame_info(const AVFrame* frame, int frame_number);


};