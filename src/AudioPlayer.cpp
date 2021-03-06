// AudioPlayerByMyself.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "OpenAL/alc.h"
#include "OpenAL/al.h"
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
};
#include <iostream>
#include <string>
int getAudioFrame(AVFormatContext *p_fmtContext, AVCodecContext *aCodec, int stream_index, AVFrame *p_Frame);
void getAudioData(AVFormatContext *p_fmtContext, AVCodecContext *aCodec, int stream_index,
	SwrContext* swr, ALuint uiSource, ALuint alBufferId, uint8_t* outBuffer);

using std::string;
using std::cout;
using std::endl;

bool isFileEnd = false;
int64_t channelLayout = 0;
int sampleRate = 0;
int channels = 0;
AVSampleFormat smpFormat;


int main(int argc, char *argv[])
{
	if (argc != 2) {
		cout << "parameter wrong format!" << endl;
		exit(-1);
	}
	AVFormatContext *formatContext = nullptr;
	AVCodecContext *vCodecContext = nullptr;
	AVCodecContext *aCodecContext = nullptr;

	SwrContext* swr = nullptr;

	int videoIndex = -1, audioIndex = -1;
	if (avformat_open_input(&formatContext, argv[1], nullptr, nullptr) != 0) {
		cout << "Failed to open video file!" << endl;
		exit(-1);
	}
	if (avformat_find_stream_info(formatContext, nullptr) < 0 ) {
		cout << "Fail to find stream information!" << endl;
		exit(-1);
	}
	for (int i = 0; i < formatContext->nb_streams; i++) {
		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoIndex = i;
		}
		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioIndex = i;
			std::cout << "audio stream index = : [" << i << "]" << std::endl;
		}
	}
	AVCodec *aCodec = avcodec_find_decoder(formatContext->streams[audioIndex]->codecpar->codec_id);
	if (aCodec == nullptr) {
		cout << "Failed to find decoder" << endl;
	}
	aCodecContext = avcodec_alloc_context3(aCodec);
	if (avcodec_parameters_to_context(aCodecContext, formatContext->streams[audioIndex]->codecpar) != 0) {
		cout << "Failed to copy audio codec context" << endl;
	}
	if (avcodec_open2(aCodecContext, aCodec, nullptr) < 0) {
		cout << "could not open codec context" << endl;
	}

	channelLayout = aCodecContext->channel_layout;
	sampleRate = aCodecContext->sample_rate;
	channels = aCodecContext->channels;

	smpFormat = AVSampleFormat((int)aCodecContext->sample_fmt);

	swr = swr_alloc_set_opts(nullptr, AV_CH_LAYOUT_STEREO, AV_SAMPLE_FMT_S16, sampleRate, channelLayout,
		smpFormat, sampleRate, 0, nullptr);
	if (swr_init(swr)) {
		throw std::runtime_error("swr_init error.");
	}

	ALuint uiSource;
	ALCdevice* pDevice;
	ALCcontext* pContext;
	pDevice = alcOpenDevice(NULL);
	pContext = alcCreateContext(pDevice, NULL);
	alcMakeContextCurrent(pContext);
	if (alcGetError(pDevice) != ALC_NO_ERROR) return AL_FALSE;
	alGenSources(1, &uiSource);
	if (alGetError() != AL_NO_ERROR) {
		cout << "Fail to get source" << endl;
		return -1;
	}
	ALfloat SourcePos[] = { 0.0, 0.0, 0.0 };
	ALfloat SourceVel[] = { 0.0, 0.0, 0.0 };
	ALfloat ListenerPos[] = { 0.0, 0.0, 0.0 };
	ALfloat ListenerVel[] = { 0.0, 0.0, 0.0 };
	// first 3 elements are "at", second 3 are "up"
	ALfloat ListenerOri[] = { 0.0, 0.0, -1.0, 0.0, 1.0, 0.0 };
	alSourcef(uiSource, AL_PITCH, 1.0);
	alSourcef(uiSource, AL_GAIN, 1.0);
	alSourcefv(uiSource, AL_POSITION, SourcePos);
	alSourcefv(uiSource, AL_VELOCITY, SourceVel);
	alSourcef(uiSource, AL_REFERENCE_DISTANCE, 50.0f);
	alSourcei(uiSource, AL_LOOPING, AL_FALSE);
	alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
	alListener3f(AL_POSITION, 0, 0, 0);
	ALuint alBufferArray[12];
	alGenBuffers(12, alBufferArray);

	uint8_t *outBuffer = nullptr;
	for (int i = 0; i < 12; i++) {
		getAudioData(formatContext, aCodecContext, audioIndex, swr, uiSource, alBufferArray[i], outBuffer);
	}
	alSourcePlay(uiSource);
	ALint iBuffersProcessed;
	ALint iState;
	ALuint bufferId;
	ALint iQueuedBuffers;
	while (true) {
		iBuffersProcessed = 0;
		alGetSourcei(uiSource, AL_BUFFERS_PROCESSED, &iBuffersProcessed);
		while (iBuffersProcessed > 0) {
			bufferId = 0;
			alSourceUnqueueBuffers(uiSource, 1, &bufferId);
			getAudioData(formatContext, aCodecContext, audioIndex, swr, uiSource, bufferId, outBuffer);
			iBuffersProcessed -= 1;
		}
		alGetSourcei(uiSource, AL_SOURCE_STATE, &iState);
		if (iState != AL_PLAYING) {
			alGetSourcei(uiSource, AL_BUFFERS_QUEUED, &iQueuedBuffers);
			if (iQueuedBuffers) {
				alSourcePlay(uiSource);
			}
			else {
				// Finished playing
				break;
			}
		}
	}
    return 0;
}

int getAudioFrame(AVFormatContext *p_fmtContext, AVCodecContext *aCodec, int stream_index, AVFrame *p_Frame) {
	if ( avcodec_receive_frame(aCodec, p_Frame) == 0 ) {
		cout << "find" << endl;
		return 0;
	}
	int pktIndex;
	AVPacket *pkt = (AVPacket*)av_malloc(sizeof(AVPacket));
	while (!isFileEnd) {
		if (av_read_frame(p_fmtContext, pkt) == 0) {
			pktIndex = pkt->stream_index;
			if (pktIndex == stream_index) {
				int ret = avcodec_send_packet(aCodec, pkt);
				if (ret == 0) {
					av_packet_unref(pkt);
					ret = avcodec_receive_frame(aCodec, p_Frame);
					if ( ret == 0 ) {
						return 0;
					}
					else {
						return -1;
					}
				}
				else if(ret == AVERROR(EAGAIN)){
					cout << "Could not decode, buffer is full!" << endl;
					return -1;
				}
				else {
					cout << "Fail to send packet to decoder!" << endl;
					return -1;
				}
			}
		}
		else {
			isFileEnd = true;
			if (aCodec != nullptr) avcodec_send_packet(aCodec, nullptr);
			cout << "End of the file!" << endl;
			return -1;
		}
	}
}

void getAudioData(AVFormatContext *p_fmtContext, AVCodecContext *aCodec, int stream_index, 
	SwrContext* swr, ALuint uiSource, ALuint alBufferId, uint8_t* outBuffer) {
	AVFrame* aFrame = av_frame_alloc();
	int bufferSize;
	int ret = getAudioFrame(p_fmtContext, aCodec, stream_index, aFrame);
	bufferSize = av_rescale_rnd(aFrame->nb_samples, sampleRate, sampleRate, AV_ROUND_UP) * 2 * 2 * 1.2;
	if (ret == 0) {
		outBuffer= (uint8_t*)av_malloc(sizeof(uint8_t) * bufferSize);
		unsigned long ulFormat = 0;
		int outSamples = swr_convert(swr, &outBuffer, bufferSize,
			(const uint8_t**)&aFrame->data[0], aFrame->nb_samples);
		int outDataSize =
			av_samples_get_buffer_size(nullptr, 2, outSamples, AV_SAMPLE_FMT_S16, 1);
		if (aFrame->channels == 1) {
			ulFormat = AL_FORMAT_MONO16;
		}
		else {
			ulFormat = AL_FORMAT_STEREO16;
		}
		alBufferData(alBufferId, ulFormat, outBuffer, outDataSize, sampleRate);
		alSourceQueueBuffers(uiSource, 1, &alBufferId);
	}
}



