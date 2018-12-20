#pragma once
#include "dr_wav.h"
// A wraped of dr_wav library for easy using.
struct wav
{
	unsigned int channels;
	unsigned int sampleRate;
	drwav_uint64 totalPCMFrameCount;	//	nSamples per cahnnels
	drwav_uint64 totalSampleCount;		//	left samples + ritht samples
	int16_t *pDataS16[2];				//	a pointer point to the S16 format audio data || S16:int16_t[-32768, 32767] is 16 bits PCM data.
										//	pDataS16[0] : left channel, pDataS16[1] : right channel
	float *pDataFloat[2];				//	a pointer point to the Float format audio data || -1 ~ 1
										//	pDataFloat[0] : left channel , pDataFloat[1] : right channel
};
int wavread(const char* filename, wav* wavfile) {
	//	Read the wav files, 

	float *pSampleFloat = drwav_open_file_and_read_pcm_frames_f32(filename, &wavfile->channels, &wavfile->sampleRate, &wavfile->totalPCMFrameCount);
	int16_t *pSampleS16 = drwav_open_file_and_read_pcm_frames_s16(filename, &wavfile->channels, &wavfile->sampleRate, &wavfile->totalPCMFrameCount);
	// The pointer of audio data which is [LRLRLRLR ...] format

	wavfile->totalSampleCount = wavfile->totalPCMFrameCount * wavfile->channels;

	if (pSampleFloat == NULL || pSampleS16 == NULL)
	{
		return -1;
	}
	//	change audio data into [[LLLLLL][RRRRRR]]
	if (wavfile->channels > 1)
	{
		wavfile->pDataS16[0] = (int16_t*)malloc(sizeof(int16_t) * wavfile->totalSampleCount);
		wavfile->pDataFloat[0] = (float*)malloc(sizeof(float) * wavfile->totalSampleCount);

		wavfile->pDataS16[1] = wavfile->pDataS16[0] + wavfile->totalPCMFrameCount;	// totalPCMFrameCount = totalSampleCount / 2
		wavfile->pDataFloat[1] = wavfile->pDataFloat[0] + wavfile->totalPCMFrameCount;

		for (size_t n = 0; n < wavfile->totalPCMFrameCount; n++)
		{
			wavfile->pDataS16[0][n] = pSampleS16[n * 2];
			wavfile->pDataS16[1][n] = pSampleS16[n * 2 + 1];

			wavfile->pDataFloat[0][n] = pSampleFloat[n * 2];
			wavfile->pDataFloat[1][n] = pSampleFloat[n * 2 + 1];

		}
		free(pSampleS16);
		free(pSampleFloat);
	}
	else
	{
		wavfile->pDataS16[0] = pSampleS16;
		wavfile->pDataS16[1] = NULL;
		wavfile->pDataFloat[0] = pSampleFloat;
		wavfile->pDataFloat[1] = NULL;

	}
	return 0;
}
int wavwrite_s16(const char* filename, int16_t * const *pDataS16, size_t nSamples, unsigned int nChannels, unsigned int sampleRate) {

	// write the data S16 data into 16-bits WAV files.
	//	pDataS16 is a pointer which is stereo : [ [L pointer],[R pointer]]  || mono : [ [L pointer],[ NULL ]] 
	//	Note that nSamples is the samples per channels

	drwav_data_format format;
	format.container = drwav_container_riff;     // <-- drwav_container_riff = normal WAV files, drwav_container_w64 = Sony Wave64.
	format.format = DR_WAVE_FORMAT_PCM;          // <-- Any of the DR_WAVE_FORMAT_* codes.
	format.channels = nChannels;
	format.sampleRate = sampleRate;
	format.bitsPerSample = 16;
	if (nChannels > 1 && pDataS16[1] == NULL)
	{
		printf(" Channel 2 data not found\n");
		return -1;
	}
	drwav* pWav = drwav_open_file_write(filename, &format);
	if (pWav == NULL)
	{
		return -1;
	}
	if (nChannels > 1)
	{
		int16_t *data = (int16_t *)malloc(sizeof(int16_t) * nSamples * 2); // nSamples is number per channels 

		for (size_t n = 0; n < nSamples; n++)
		{
			data[n * 2] = pDataS16[0][n];		// even part left channel
			data[n * 2 + 1] = pDataS16[1][n];	//  odd part, ritht channel
		}
		//drwav_uint64 samplesWritten = drwav_write_pcm_frames(pWav, nSamples, data);
		drwav_uint64 samplesWritten = drwav_write_raw(pWav, nSamples * nChannels * 16 / 8, data);
		if (samplesWritten != nSamples * nChannels * 16 / 8)
		{
			printf("written 2ch failed\n");
		}
		free(data);
	}
	else
	{
		//drwav_uint64 samplesWritten = drwav_write_pcm_frames(pWav, nSamples, pDataS16[0]);	// convert
		drwav_uint64 samplesWritten = drwav_write_raw(pWav, nSamples * 2, pDataS16[0]);	// convert
		if (samplesWritten != nSamples * 2)
		{
			printf("written 1ch failed\n");
			//return -1;
		}
	}

	drwav_close(pWav);
	return 0;

}
int wavwrite_float(const char* filename, float * const *pDataFloat, size_t nSamples, unsigned int nChannels, unsigned int sampleRate) {
	
	//	write the data Float data into 16-bits WAV files.
	//	pDataFloat is a pointer which is :  stereo : [ [L pointer],[R pointer]]  || mono : [ [L pointer],[ NULL ]] 
	//	Note that nSamples is the samples per channels

	if (pDataFloat == NULL)
	{
		printf("Input data pointer failed.\n");
		return -1;
	}
	if (pDataFloat[1] == NULL && nChannels > 1)
	{
		printf(" Channel 2 data not found\n");
		return -1;
	}
	int16_t * tmp = (int16_t *)malloc(sizeof(int16_t) * nSamples * nChannels);


	int16_t *pDataS16[2];
	pDataS16[0] = tmp;
	//FloatToS16(pDataFloat[0], nSamples, pDataS16[0]);	//drwav_f32_to_s16
	drwav_f32_to_s16(pDataS16[0], pDataFloat[0], nSamples);

	if (nChannels > 1)
	{
		pDataS16[1] = tmp + nSamples;
		//FloatToS16(pDataFloat[1], nSamples, pDataS16[1]);
		drwav_f32_to_s16(pDataS16[1], pDataFloat[1], nSamples);

	}
	else
	{
		pDataS16[1] = NULL;
	}


	int error = wavwrite_s16(filename, pDataS16, nSamples, nChannels, sampleRate);

	free(tmp);

	if (error != 0)
	{
		printf("Output float wav failed\n");
		return -1;
	}
	return 0;
}

// ==============================//
//static inline float S16ToFloat_(int16_t v) {
//	static const float kMaxInt16Inverse = 1.f / 32767;
//	static const float kMinInt16Inverse = 1.f / -32768;
//	return v * (v > 0 ? kMaxInt16Inverse : -kMinInt16Inverse);
//}
//int S16ToFloat(int16_t *pS16Samples, size_t nSamples, float *pFloatSamples) {
//	if (pFloatSamples == NULL || nSamples < 0 || pS16Samples == NULL)
//	{
//		return -1;
//	}
//	for (size_t n = 0; n < nSamples; n++)
//	{
//		pFloatSamples[n] = S16ToFloat_(pS16Samples[n]);
//	}
//	return 0;
//}
//
//static inline int16_t FloatToS16_(float v) {
//	//S16:      int16_t[-32768, 32767]
//	if (v > 0)
//		return v >= 1 ? 32767
//		: (int16_t)(v * 32767 + 0.5f);
//	return v <= -1 ? -32768
//		: (int16_t)(-v * -32768 - 0.5f);
//}
//int FloatToS16(float * pFloatSamples, size_t nSamples, int16_t *pS16Samples) {
//
//	if (pFloatSamples == NULL || nSamples < 0 || pS16Samples == NULL)
//	{
//		return -1;
//	}
//	for (size_t n = 0; n < nSamples; n++)
//	{
//		pS16Samples[n] = FloatToS16_(pFloatSamples[n]);
//	}
//	return 0;
//}