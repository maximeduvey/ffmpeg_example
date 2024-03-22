// ffmpeg_example.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "ffmpeg_example.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <map>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

}

#define COMMON_AV_FRAME_FORMAT AV_PIX_FMT_YUV420P 

ffmpeg_example::ffmpeg_example()
{

}

ffmpeg_example::~ffmpeg_example()
{

}

void ffmpeg_example::set_codec_params(AVFormatContext*& fctx, AVCodecContext*& codec_ctx, double width, double height, int fps, int bitrate) {
    std::cout << "encoding video for width:" << width << ", height:" << height << ", fps:" << fps << ", bitrate:" << bitrate << std::endl;
    codec_ctx->codec_id = fctx->oformat->video_codec;
    codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->gop_size = 12;
    codec_ctx->pix_fmt = COMMON_AV_FRAME_FORMAT; // is AV_PIX_FMT_YUV420P
    codec_ctx->framerate = AVRational{ fps, 1 };
    codec_ctx->time_base = AVRational{ 1, fps };
    if (bitrate) codec_ctx->bit_rate = bitrate;
    if (fctx->oformat->flags & AVFMT_GLOBALHEADER) {
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
}

AVStream* ffmpeg_example::create_stream(AVFormatContext*& fctx, AVCodecContext*& codec_ctx) {
    AVStream* stream = avformat_new_stream(fctx, nullptr);
    avcodec_parameters_from_context(stream->codecpar, codec_ctx);
    stream->time_base = codec_ctx->time_base;
    //stream->r_frame_rate = codec_ctx->framerate;
    stream->avg_frame_rate = codec_ctx->framerate;

    std::cout << "codec setting num:" << codec_ctx->framerate.num << ", dem:" << codec_ctx->framerate.den << std::endl;
    std::cout << "stream setting num:" << stream->avg_frame_rate.num << ", dem:" << stream->avg_frame_rate.den << std::endl;
    return stream;
}

AVFrame* ffmpeg_example::create_frame(int width, int height) {
    AVFrame* frame = av_frame_alloc();
    frame->format = codec_context->pix_fmt;//COMMON_AV_FRAME_FORMAT;
    frame->width = width;
    frame->height = height;
    av_frame_get_buffer(frame, 0);
    return frame;
}

void ffmpeg_example::fill_fake_yuv_image(AVFrame* frame, int frame_index, int width, int height) {
    /* Ensure the data buffers are writable */
    av_frame_make_writable(frame);

    // This is where you could load actual image data into the frame.
    // For demonstration, we fill the frame with a color.
    // Y
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            frame->data[0][y * frame->linesize[0] + x] = x + y + frame_index * 3;
        }
    }

    // Cb and Cr
    for (int y = 0; y < height / 2; y++) {
        for (int x = 0; x < width / 2; x++) {
            frame->data[1][y * frame->linesize[1] + x] = 128 + y + frame_index * 2;
            frame->data[2][y * frame->linesize[2] + x] = 64 + x + frame_index * 5;
        }
    }
}

void ffmpeg_example::dump_stream_info(AVStream* stream) {
    std::cout << "Stream Information:\n";
    std::cout << "Index: " << stream->index << "\n";
    std::cout << "Time Base: " << stream->time_base.num << "/" << stream->time_base.den << "\n";
    AVCodecParameters* codec_par = stream->codecpar;
    std::cout << "Codec ID: " << avcodec_get_name(codec_par->codec_id) << "\n";
    std::cout << "Codec Type: " << av_get_media_type_string(codec_par->codec_type) << "\n";
    std::cout << "Width x Height: " << codec_par->width << " x " << codec_par->height << "\n";
    std::cout << "Bit Rate: " << codec_par->bit_rate << "\n";
    std::cout << "Frame Rate: " << av_q2d(stream->avg_frame_rate) << " fps\n";
    // Add more fields as needed
}

int ffmpeg_example::initFfmpegcontext()
{
    int ret;

    //encoder = avcodec_find_encoder_by_name("mpeg4");

    // Initialize the AVFormatContext
    if (avformat_alloc_output_context2(&output_format_context, nullptr, _ouput_format, nullptr) < 0 || !output_format_context) {
        std::cout << "Could not allocat output context." << std::endl;
        return -1;
    }

    // Find the encoder
    //encoder = avcodec_find_encoder(output_format_context->oformat->video_codec);
    encoder = avcodec_find_encoder_by_name("libx264");
    if (!encoder) {
        std::cout << "Could not find encoder." << std::endl;
        return -1;
    }

    // Create a new codec context
    codec_context = avcodec_alloc_context3(encoder);
    if (!codec_context) {
        std::cout << "Could not allocate codec context." << std::endl;
        return -1;
    }

    // Set codec parameters
    set_codec_params(output_format_context, codec_context, 1920, 1080, video_fps, 400000);
    av_dump_format(output_format_context, 0, output, 1);

    // Open the codec
    ret = avcodec_open2(codec_context, encoder, nullptr);
    if (ret < 0) {
        std::cout << "Could not open encoder with context." << std::endl;
        return -1;
    }

    // Create a new video stream
    video_stream = create_stream(output_format_context, codec_context);
    if (!video_stream) {
        std::cout << "Could not create stream." << std::endl;
        return -1;
    }

    // Initialize the IO context
    if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
        std::cout << "Init IO, AVFMT_NOFILE not detected." << std::endl;
        if (avio_open(&output_format_context->pb, output, AVIO_FLAG_WRITE) < 0) {
            std::cout << "Could not open output file." << std::endl;
            return -1;
        }
    }

    // Write the file header
    ret = avformat_write_header(output_format_context, nullptr);
    if (ret < 0) {
        std::cout << "Could not write file header." << std::endl;
        return -1;
    }

    // Create a new frame
    frame = create_frame(codec_context->width, codec_context->height);
    if (!frame) {
        std::cout << "Could not create frame." << std::endl;
        return -1;
    }

    // Create a new packet
    pkt = av_packet_alloc();
    if (!pkt) {
        std::cout << "Could not allocate packet." << std::endl;
        return -1;
    }
    return 0;
}

void ffmpeg_example::dump_frame_info(const AVFrame* frame, int frame_number) {
    std::cout << "Frame Number: " << frame_number << "\n";
    std::cout << "Format: " << av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format)) << "\n";
    std::cout << "Width : " << frame->width << "  Height " << frame->height << "\n";
    std::cout << "PTS: " << frame->pts << "\n";
    //std::cout << "Keyframe: " << (frame->key_frame ? "Yes" : "No") << "\n";
    std::cout << "Picture Type: " << av_get_picture_type_char(frame->pict_type) << "\n";
    // Add more fields as needed
}


void ffmpeg_example::set_metadata() {
    // Ensure the format context exists
    if (!output_format_context) return;
    std::cout << "Setting metadata." << std::endl;

    // Create or get the existing metadata dictionary
    AVDictionary** metadata = &output_format_context->metadata;

    // Set various metadata fields
    av_dict_set(metadata, "title", "Your Title Here", 0);
    av_dict_set(metadata, "author", "Author Name", 0);
    av_dict_set(metadata, "copyright", "Copyright Information", 0);
    av_dict_set(metadata, "comment", "Your Comment Here", 0);
    av_dict_set(metadata, "rating", "5", 0); // Assuming rating is a simple numeric value

    // Note: The last parameter of av_dict_set is a flag; 0 means the entry will be added to the dictionary
    // if it doesn't exist, or the existing entry will be updated if it does exist.
}

int ffmpeg_example::start()
{
    int ret;
    if (initFfmpegcontext() < 0) {
        std::cout << "quitting" << std::endl;
        return -1;
    }
    //std::cout << "!!! 0 !!! " << std::endl;
    //av_dump_format(output_format_context, 0, output, 1);
    dump_stream_info(video_stream);
    set_metadata();
    std::cout << "filling stream with frames data." << std::endl;
    // Write frames
    for (int i = 0; i < (video_fps * 15); i++) { // 5 seconds at video_fps fps
        //frame->pts = (1.0 / video_fps) * video_fps * i;
        frame->pts = i;
        fill_fake_yuv_image(frame, i, codec_context->width, codec_context->height);


        // Encode the image
        ret = avcodec_send_frame(codec_context, frame);

        ret = avcodec_receive_packet(codec_context, pkt);
        pkt->stream_index = video_stream->index;
        av_interleaved_write_frame(output_format_context, pkt);

        //while (ret >= 0) {
        //    ret = avcodec_receive_packet(codec_context, pkt);
        //    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        //        break;
        //    }
        //    pkt->stream_index = video_stream->index;
        //    av_interleaved_write_frame(output_format_context, pkt);
        //    av_packet_unref(pkt);
        //}
        // 
        //std::cout << "-------" << std::endl;
        //dump_frame_info(frame, i);
    }
    std::cout << "!!! 1 !!! " << std::endl;
    av_dump_format(output_format_context, 0, output, 1);
    std::cout << "closing." << std::endl;
    // Send a null frame to the encoder to flush it

    avcodec_send_frame(codec_context, NULL);

    ret = avcodec_receive_packet(codec_context, pkt);
    pkt->stream_index = video_stream->index;
    av_interleaved_write_frame(output_format_context, pkt);

    //while (ret >= 0) {
    //    ret = avcodec_receive_packet(codec_context, pkt);
    //    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
    //        break;
    //    }
    //    pkt->stream_index = video_stream->index;
    //    av_interleaved_write_frame(output_format_context, pkt);
    //    av_packet_unref(pkt);
    //}

    // Write the file trailer
    av_write_trailer(output_format_context);

    //std::cout << "!!! 1 !!! " << std::endl;
    //av_dump_format(output_format_context, 0, output, 1);
    //dump_stream_info(video_stream);
    cleanup();
    std::cout << "done." << std::endl;

    return 0;
}

void ffmpeg_example::cleanup() {
    // Clean up
    avcodec_free_context(&codec_context);
    av_frame_free(&frame);
    av_packet_free(&pkt);
    avio_closep(&output_format_context->pb);
    avformat_free_context(output_format_context);
}
int main()
{
    std::cout << "Hello World!\n";
    ffmpeg_example test;
    test.start();
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
