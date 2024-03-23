// ffmpeg_example.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "ffmpeg_example.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <map>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>

}
#include "Tools.hpp"

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
	frame->format = encoder_context->pix_fmt;//COMMON_AV_FRAME_FORMAT;
	frame->width = width;
	frame->height = height;
	av_frame_get_buffer(frame, 0);
	return frame;
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

int ffmpeg_example::initFfmpegcontext(const std::string codec_name)
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
	encoder = avcodec_find_encoder_by_name(codec_name.c_str());
	if (!encoder) {
		std::cout << "Could not find encoder." << std::endl;
		return -1;
	}

	// Create a new codec context
	encoder_context = avcodec_alloc_context3(encoder);
	if (!encoder_context) {
		std::cout << "Could not allocate codec context." << std::endl;
		return -1;
	}

	// Set codec parameters
	set_codec_params(output_format_context, encoder_context, _width, _height, _video_fps, _bitrate);
	av_dump_format(output_format_context, 0, _output, 1);

	// Open the codec
	ret = avcodec_open2(encoder_context, encoder, nullptr);
	if (ret < 0) {
		std::cout << "Could not open encoder with context." << std::endl;
		return -1;
	}

	// Create a new video stream
	video_stream = create_stream(output_format_context, encoder_context);
	if (!video_stream) {
		std::cout << "Could not create stream." << std::endl;
		return -1;
	}

	// Initialize the IO context
	if (!(output_format_context->oformat->flags & AVFMT_NOFILE)) {
		std::cout << "Init IO, AVFMT_NOFILE not detected." << std::endl;
		if (avio_open(&output_format_context->pb, _output, AVIO_FLAG_WRITE) < 0) {
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
	frame = create_frame(encoder_context->width, encoder_context->height);
	if (!frame) {
		std::cout << "Could not create frame." << std::endl;
		return -1;
	}

	// we buffer the stream so that it will not skipp the first images
	pkt = av_packet_alloc();

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

void ffmpeg_example::fill_frames_with_image(const std::string& image, AVFrame* frame)
{
	const AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
	if (!codec) {
		std::cerr << "Codec not found\n";
		return;
	}
}

void ffmpeg_example::fill_frame_with_fake_yuv_image(AVFrame* frame, int frame_index, int width, int height) {
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

int ffmpeg_example::initMPEGDecoder()
{
	decoder = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
	if (!decoder) {
		fprintf(stderr, "JPEG decoder not found\n");
		return -1;
	}

	decoder_context = avcodec_alloc_context3(decoder);
	if (avcodec_open2(decoder_context, decoder, nullptr) < 0) {
		fprintf(stderr, "Could not open JPEG decoder\n");
		return -1;
	}
	return 0;
}

AVFrame* ffmpeg_example::scale_frame(AVFrame* frame, AVCodecContext* codec_context) {

	struct SwsContext* sws_ctx = sws_getContext(
		frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
		codec_context->width, codec_context->height, AV_PIX_FMT_YUV420P,
		SWS_BILINEAR, nullptr, nullptr, nullptr);

	AVFrame* scaled_frame = av_frame_alloc();
	scaled_frame->format = codec_context->pix_fmt;
	scaled_frame->width = codec_context->width;
	scaled_frame->height = codec_context->height;
	av_frame_get_buffer(scaled_frame, 0);

	sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height,
		scaled_frame->data, scaled_frame->linesize);

	sws_freeContext(sws_ctx);
	return scaled_frame;  // Remember to free this frame after use
}


AVFrame* ffmpeg_example::decode_jpeg_to_frame(const std::string& jpegData) {
	AVPacket* packet = av_packet_alloc();
	packet->data = (uint8_t*)jpegData.data();  // Unsafe cast; for demonstration only.
	packet->size = jpegData.size();

	if (avcodec_send_packet(decoder_context, packet) < 0) {
		fprintf(stderr, "Error sending packet to decoder\n");
		return nullptr;
	}

	AVFrame* frame = av_frame_alloc();
	if (avcodec_receive_frame(decoder_context, frame) < 0) {
		fprintf(stderr, "Error receiving frame from decoder\n");
		return nullptr;
	}

	av_packet_free(&packet);  // Cleanup packet after use

	return frame;  // Caller is responsible for freeing this frame later
}

int ffmpeg_example::encode_video_to_x264(std::vector<std::string> frames, const char* ouput)
{
	_output = ouput;
	int ret;
	if (initFfmpegcontext("libx264") < 0) {
		std::cout << "quitting" << std::endl;
		return -1;
	}
	bool isFakeImage = false;
	int nbr_frames_to_gen;
	if (nbr_frames_to_gen = frames.size(); nbr_frames_to_gen <= 0) {
		nbr_frames_to_gen = (_video_fps * 5);
		isFakeImage = true;
	}
	else if (initMPEGDecoder() < 0)
		return -1;

	nbr_frames_to_gen = (_video_fps * 5);

	int wtf = 0, nbr = 0;

	dump_stream_info(video_stream);
	set_metadata();
	std::cout << "filling stream with frames data ("<< nbr_frames_to_gen <<")." << std::endl;
	// Write frames
	for (int i = 0; i < nbr_frames_to_gen ; i++) {

		frame->pts = av_rescale_q(i, AVRational{ 1, _video_fps }, encoder_context->time_base);
		//std::cout << "Frame to be used image width:" << frame->width << ", height:" << frame->height << std::endl;
		if (isFakeImage ) {
			std::cout << "fake.\n";
			fill_frame_with_fake_yuv_image(frame, i, encoder_context->width, encoder_context->height);
		}
		else 
		{
			std::cout << "real "<< i << " - " << frame->pts <<".\n";
			if ((frame = decode_jpeg_to_frame(frames[(0)% frames.size()])) == nullptr) {
				std::cout << "Failed to decode image buffer (supposed png)" << std::endl;
				return -1;
			}
			//std::cout << "Decoded image width:" << frame->width << ", height:" << frame->height << std::endl;
			//std::cout << "Codec width:" << encoder_context->width << ", height:" << encoder_context->height << std::endl;
			if (frame = scale_frame(frame, encoder_context); frame == nullptr) {
				std::cout << "Could not scale image." << std::endl;
				return -1;
			}
			//std::cout << "Frame to be used image width:" << frame->width << ", height:" << frame->height << std::endl;
		}
		frame->pts = av_rescale_q(i, AVRational{ 1, _video_fps }, encoder_context->time_base);
		std::cout << frame->pts << "\n";
		


		//// Encode the image
		ret = avcodec_send_frame(encoder_context, frame);
		if (ret != 0) {
			std::cout << "Error sending frame." << std::endl;
			return -1;
		}

		//ret = avcodec_receive_packet(encoder_context, pkt);
		//pkt->stream_index = video_stream->index;
		//av_interleaved_write_frame(output_format_context, pkt);

		//pkt = av_packet_alloc();
		//ret = av_interleaved_write_frame(output_format_context, pkt);
		//pkt->stream_index = video_stream->index;
		//if (ret < 0) {
		//	fprintf(stderr, "Error while writing frame to output\n");
		//	return ret;
		//}
		//ret = avcodec_receive_packet(encoder_context, pkt);
		//av_packet_free(&pkt);


		while (ret >= 0) {
			pkt = av_packet_alloc();
			ret = avcodec_receive_packet(encoder_context, pkt);
			std::cout << "ret " << ret << "\n";
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
				++nbr;
				av_packet_free(&pkt);
				break;
			}
			else if (ret < 0) {
				fprintf(stderr, "Error encoding frame\n");
				av_packet_free(&pkt);
				return ret;
			}
			++wtf;
			pkt->stream_index = video_stream->index;
			ret = av_interleaved_write_frame(output_format_context, pkt);
			av_packet_free(&pkt);  // Cleanup packet after use
			if (ret < 0) {
				fprintf(stderr, "Error while writing frame to output\n");
				return ret;
			}
		}


	}

	av_dump_format(output_format_context, 0, _output, 1);
	std::cout << "closing." << std::endl;
	// Send a null frame to the encoder to flush it
	pkt = av_packet_alloc();
	avcodec_send_frame(encoder_context, NULL);

	ret = avcodec_receive_packet(encoder_context, pkt);
	pkt->stream_index = video_stream->index;
	av_interleaved_write_frame(output_format_context, pkt);

	// Write the file trailer
	av_write_trailer(output_format_context);

	cleanup();
	std::cout << "done " << wtf << " - "  << nbr << std::endl;

	return 0;
}

int ffmpeg_example::example_create_video_with_fake_image()
{
	return encode_video_to_x264(std::vector<std::string>(), "fake_video.264");
}

void ffmpeg_example::cleanup() {
	// Clean up
	avcodec_free_context(&encoder_context);
	av_frame_free(&frame);
	av_packet_free(&pkt);
	avio_closep(&output_format_context->pb);
	avformat_free_context(output_format_context);
}

int ffmpeg_example::example_create_video_from_images()
{
	std::vector<std::string> frames;
	auto filesToLoad = Tools::getAllFilesInFolder("images_test_unique");
	for (const auto& file : filesToLoad)
	{
		std::cout << file << std::endl;
		frames.push_back(Tools::getFileContents(file.c_str()));
	}

	return encode_video_to_x264(frames, "output.264");
}

int main()
{
	std::cout << "Hello World!\n";
	ffmpeg_example test;

	test._video_fps = 25;

	// !!!! Libx264 format is buffering the first frames (environ 34 frames) en interne, c'est fait pour des gros fichier
	// so it mean we are "loosing" the first frames, it may not be suited for low fps, to stream video

	test.example_create_video_with_fake_image();

	//test.example_create_video_from_images();

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
