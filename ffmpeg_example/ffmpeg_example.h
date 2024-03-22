#pragma once

class AVFormatContext;
class AVCodecContext;
class AVStream;
class AVCodec;
class AVFrame;
class AVPacket;
class AVOutputFormat;

class ffmpeg_example {
private:
    const AVCodec* encoder = nullptr;
    AVFormatContext* output_format_context = nullptr;
    AVCodecContext* codec_context = nullptr;
    AVStream* video_stream = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* pkt = nullptr;

    const char* output = "output.264";
    const char* _ouput_format = "h264";

    int video_fps = 30;
public:

    ffmpeg_example();
    virtual ~ffmpeg_example();

    int start();
private:
    int initFfmpegcontext();
    AVFrame* create_frame(int width, int height);
    void fill_fake_yuv_image(AVFrame* frame, int frame_index, int width, int height);
    void set_codec_params(AVFormatContext*& fctx, AVCodecContext*& codec_ctx, double width, double height, int fps, int bitrate);
    AVStream* create_stream(AVFormatContext*& fctx, AVCodecContext*& codec_ctx);

    void cleanup();
    void set_metadata();
    void dump_stream_info(AVStream* stream);
    void dump_frame_info(const AVFrame* frame, int frame_number);


};