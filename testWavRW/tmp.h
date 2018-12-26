#pragma once
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <vector>
using std::vector;
using std::cin;
using std::cout;
using std::cerr;

const unsigned int nTaps = 48;
float state[nTaps]{};

float coeffs[nTaps] = {
0.00307579439661909,
0.00322721449892899,
0.00367868168493064,
0.00442250164187976,
0.00544597438106291,
0.00673161135242879,
0.00825743457463436,
0.00999735266227356,
0.0119216073144581,
0.0139972826215132,
0.0161888684700299,
0.0184588683993410,
0.0207684415005435,
0.0230780673655622,
0.0253482226984192,
0.0275400580006534,
0.0296160627411369,
0.0315407076174173,
0.0332810529077914,
0.0348073124938926,
0.0360933638926892,
0.0371171955614535,
0.0378612838136254,
0.0383128928891467,
0.0384642930391365,
0.0383128928891467,
0.0378612838136254,
0.0371171955614535,
0.0360933638926892,
0.0348073124938926,
0.0332810529077914,
0.0315407076174173,
0.0296160627411369,
0.0275400580006534,
0.0253482226984192,
0.0230780673655622,
0.0207684415005435,
0.0184588683993410,
0.0161888684700299,
0.0139972826215132,
0.0119216073144581,
0.00999735266227356,
0.00825743457463436,
0.00673161135242879,
0.00544597438106291,
0.00442250164187976,
0.00367868168493064,
0.00322721449892899,
};
namespace {

	const float my_ns_coeffs[] = {
		//     b1           b2           a1           a2
		-1.62666423,  0.79410094,  0.61367127,  0.23311013,  // section 1
		-1.44870017,  0.54196219,  0.03373857,  0.70316556   // section 2
	};

	const int my_ns_soscount = sizeof(my_ns_coeffs) / (sizeof(my_ns_coeffs[0]) * 4);

	inline long myround(float x)
	{
		return static_cast<long>(x + (x >= 0 ? 0.5f : -0.5f));
	}

	template<typename T>
	struct id { typedef T type; };

	template<typename T>
	inline T clip(
		typename id<T>::type min,
		T v,
		typename id<T>::type max)
	{
		if (v<min) return min;
		if (v>max) return max;
		return v;
	}

	inline void write_intel16(unsigned char * ptr, unsigned word)
	{
		ptr[0] = word & 0xFF;
		ptr[1] = (word >> 8) & 0xFF;
	}

	inline void write_intel24(unsigned char * ptr, unsigned long word)
	{
		ptr[0] = word & 0xFF;
		ptr[1] = (word >> 8) & 0xFF;
		ptr[2] = (word >> 16) & 0xFF;
	}

} // anonymous namespace


struct DSD
{
	// uint64_t  a 8 bytes,64 bits unsigned int, 
	// uint32_t  a 4 bytes,32 bits unsigned int, 

	char dsd_chunk_header[4];		// DSD mask
	char chunk_size[8];				// size of this chunk [28]
									//uint64_t chunk_size;			// size of this chunk [28] test 
	uint64_t total_size;			// total file size
	char pMetadataChunk[4];			// the pointer point to metadata if exist,otherwise set 0.
									// 8?
	char fmt_chunk_header[4];		// fmt chunk header
	uint64_t fmt_size;				// size of this fmt chunk,usually [52] bytes
	uint32_t fmt_version;			// gersion of this format [1]
	uint32_t fmt_id;				// [0]: DSD raw
	uint32_t channel_type;			// [1]:mono [1]:stereo [3]: 3 channels [4] quad [5] 4chs [6] 5chs [7] 5.1chs
	uint32_t channel_num;			// [1]-[6] : mono~6 chs
	uint32_t sample_rate;			//  [2822400] or [5644800] hz  64fs or 128fs
	uint32_t bits_per_sample;		// [1] or [8]
	uint64_t sample_count;			// samples per channels, n second data: sample count would be fs * n
	uint32_t block_per_channel;		// fixed [4096] Bytes
	uint32_t reverd;				// fill zero

	char	data_chunk_header[4];	// [ d a t a]
	uint64_t data_size;			// equal to [n] + 12 , n is the next [n] bytes contains data 
								// next n bytes is the data
								//char* pSampleData = 0;
								// m bytes meta data chunk if have
	uint8_t* pSampleData = 0;	// a pointer points to the sample data (byte types)
	unsigned int nBytes;
};

struct  Biquad
{
	float coeffs[2][3]{};
	float state[3]{};
};
void biquad(Biquad *handles, float in, float &out) {
	// wrapped methods
	handles->state[2] = handles->state[1];
	handles->state[1] = handles->state[0];
	handles->state[0] = in + (-handles->coeffs[1][1]) * handles->state[1] + (-handles->coeffs[1][2]) * handles->state[2];
	// caculate the output
	out = handles->coeffs[0][0] * handles->state[0] + handles->coeffs[0][1] * handles->state[1] + handles->coeffs[0][2] * handles->state[2];
}

void fir(unsigned int nTaps, float *coeffs, float *state, float in, float &out,unsigned int pos) {

	float sum = 0.0;
	//for (size_t i = nTaps - 1; i > 2; --i)		// 需要使用循环队列优化
	//{
	//	state[i] = state[i - 1];
	//}
	state[pos++] = in;
	if (pos >nTaps)
	{
		pos = 0;
	}
	unsigned int k = pos;
	for (size_t n = 0; n < nTaps; n++)
	{
		sum += coeffs[n] * state[k];
		if (--k < 0)
		{
			k = nTaps-1;
		}
	}
	out = sum;
}
void RunFIRFilter(double *FirCoeff, int NumTaps, double *Signal, double *FilteredSignal, int NumSigPts)
{
	int j, k, n, Top = 0;
	double y, Reg[256];  // This assumes <= 256 taps.

	for (j = 0; j < NumTaps; j++)Reg[j] = 0.0;

	for (j = 0; j < NumSigPts; j++)
	{
		Reg[Top] = Signal[j];
		y = 0.0;
		n = 0;

		// The FirCoeff index increases while the Reg index decreases.
		for (k = Top; k >= 0; k--)
		{
			y += FirCoeff[n++] * Reg[k];
		}
		for (k = NumTaps - 1; k > Top; k--)
		{
			y += FirCoeff[n++] * Reg[k];
		}
		FilteredSignal[j] = y;

		Top++;
		if (Top >= NumTaps)Top = 0;
	}
}