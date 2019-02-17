#pragma once

#include "olcPixelGameEngine.h"
#include "olcPGEX_Sound.h"

namespace olc {

	//HEADER
	class AudioConvert : public PGEX {
	public:
		static int LoadAudioSample(std::string sWavFile, olc::ResourcePack *pack = nullptr);

	private:
		static olc::SOUND::AudioSample LoadFromFile(std::string sWavFile, olc::ResourcePack *pack);
		static float convertPCMSample(uint64_t sample, int bitDepth);
		static float convertALAWSample(uint8_t sample);
		static float convertULAWSample(uint8_t sample);
	};
}
//IMPLEMENTATION

#ifdef OLC_PGEX_AUDIOCONVERT
#undef OLC_PGEX_AUDIOCONVERT

int olc::AudioConvert::LoadAudioSample(std::string sWavFile, olc::ResourcePack *pack) {
	olc::SOUND::AudioSample a = LoadFromFile(sWavFile, pack);
	if (a.bSampleValid)
	{
		olc::vecAudioSamples.push_back(a);
		return (unsigned int)vecAudioSamples.size();
	}
	else
		return -1;
}

olc::SOUND::AudioSample olc::AudioConvert::LoadFromFile(std::string sWavFile, olc::ResourcePack *pack) {

	auto ReadWave = [&](std::istream &is)
	{
		SOUND::AudioSample sample;
		sample.bSampleValid = false;
		char dump[4];
		is.read(dump, sizeof(char) * 4); // Read "RIFF"
		if (strncmp(dump, "RIFF", 4) != 0) return sample;
		is.read(dump, sizeof(char) * 4); // Not Interested
		is.read(dump, sizeof(char) * 4); // Read "WAVE"
		if (strncmp(dump, "WAVE", 4) != 0) return sample;

		// Read Wave description chunk
		is.read(dump, sizeof(char) * 4); // Read "fmt "
		unsigned int nHeaderSize = 0;
		is.read((char*)&nHeaderSize, sizeof(unsigned int)); // Not Interested
		is.read((char*)&sample.wavHeader, nHeaderSize);// sizeof(WAVEFORMATEX)); // Read Wave Format Structure chunk
													// Note the -2, because the structure has 2 bytes to indicate its own size
													// which are not in the wav file

		// Search for audio data chunk
		uint32_t nChunksize = 0;
		is.read(dump, sizeof(char) * 4); // Read chunk header
		is.read((char*)&nChunksize, sizeof(uint32_t)); // Read chunk size
		while (strncmp(dump, "data", 4) != 0)
		{
			// Not audio data, so just skip it
			//std::fseek(f, nChunksize, SEEK_CUR);
			is.seekg(nChunksize, std::istream::cur);
			is.read(dump, sizeof(char) * 4);
			is.read((char*)&nChunksize, sizeof(uint32_t));
		}

		// Finally got to data, so read it all in and convert to float samples
		sample.nSamples = nChunksize / (sample.wavHeader.nChannels * (sample.wavHeader.wBitsPerSample >> 3));
		sample.nChannels = sample.wavHeader.nChannels;

		// Create floating point buffer to hold audio sample
		float *fSample = new float[sample.nSamples * sample.nChannels];
		float *pSample = fSample;

		// Read in audio data and normalise
		for (long i = 0; i < sample.nSamples; i++)
		{
			for (int c = 0; c < sample.nChannels; c++)
			{
				if (!is.eof())
				{
					if (sample.wavHeader.wFormatTag == 0x1) { //PCM
						uint64_t s = 0;
						is.read((char*)&s, sample.wavHeader.wBitsPerSample / 8);
						*pSample = convertPCMSample(s, sample.wavHeader.wBitsPerSample);
						pSample++;
					}
					else if (sample.wavHeader.wFormatTag == 0x3) { //IEEE_FLOAT
						if (sample.wavHeader.wBitsPerSample == 32) {
							float s = 0;
							is.read((char*)&s, 4);
							*pSample = s;
							pSample++;
						}
						else if (sample.wavHeader.wBitsPerSample == 64) {
							double s = 0;
							is.read((char*)&s, 8);
							*pSample = float(s);
							pSample++;
						}
					}
					else if (sample.wavHeader.wFormatTag == 0x6) { //A-LAW
						uint8_t s = 0;
						is.read((char*)&s, 1);
						*pSample = convertALAWSample(s);
						pSample++;
					}
					else if (sample.wavHeader.wFormatTag == 0x7) {
						uint8_t s = 0;
						is.read((char*)&s, 1);
						*pSample = convertULAWSample(s);
						pSample++;
					}
				}
			}
		}

		int resampledLength = sample.nSamples / float(sample.wavHeader.nSamplesPerSec) * 44100;

		sample.fSample = new float[resampledLength * sample.nChannels];
		for (int i = 0; i < resampledLength; i++) {
			float samplePos = i / 44100.f * float(sample.wavHeader.nSamplesPerSec);
			int samplePosI = floor(samplePos);
			float samplePosF = samplePos - samplePosI;
			for (int c = 0; c < sample.nChannels; c++) {
				float sampleA = fSample[samplePosI * sample.nChannels + c];
				float sampleB = sampleA;
				if (i != resampledLength - 1)
					sampleB = fSample[(samplePosI + 1) * sample.nChannels + c];
				sample.fSample[i * sample.nChannels + c] = sampleA  * (1 - samplePosF) + sampleB * samplePosF;
			}
		}

		sample.nSamples = resampledLength;

		delete[] fSample;

		// All done, flag sound as valid
		sample.bSampleValid = true;
		return sample;
	};

	if (pack != nullptr)
	{
		olc::ResourcePack::sEntry entry = pack->GetStreamBuffer(sWavFile);
		std::istream is(&entry);
		return ReadWave(is);
	}
	else
	{
		// Read from file
		std::ifstream ifs(sWavFile, std::ifstream::binary);
		if (ifs.is_open())
		{
			return ReadWave(ifs);
		}
		else {
			olc::SOUND::AudioSample sample;
			sample.bSampleValid = false;
			return sample;
		}
	}
}

inline float olc::AudioConvert::convertPCMSample(uint64_t sample, int bitDepth)
{
	if (bitDepth == 8)
		return float(sample) / UCHAR_MAX;
	bool sign = sample & (1 << (bitDepth - 1));
	uint64_t newSample = sample & ~(1 << (bitDepth - 1));
	if (sign)
		newSample = ((~newSample) & (((1 << (bitDepth - 1)) - 1))) + 1;
	float result = (!sign * 2 - 1) * float(newSample) / ((1 << bitDepth - 1) - 1);
	if (result < -1)
		return -1;
	if (result > 1)
		return 1;
	return result;
}

inline float olc::AudioConvert::convertALAWSample(uint8_t sample)
{
	uint16_t t, seg;

	sample ^= 0x55;
	t = sample & 0x7f;
	if (t < 16)
		t = (t << 4) + 8;
	else {
		seg = (t >> 4) & 0x07;
		t = ((t & 0x0f) << 4) + 0x108;
		t <<= seg - 1;
	}
	return ((sample & 0x80) ? t : -t) / float(SHRT_MAX);
}

inline float olc::AudioConvert::convertULAWSample(uint8_t sample)
{
	uint8_t sign = 0, position = 0;
	int16_t decoded = 0;
	sample = ~sample;
	if (sample & 0x80) {
		sample &= ~(1 << 7);
		sign = -1;
	}
	position = ((sample & 0xf0) >> 4) + 5;
	decoded = ((1 << position) | ((sample & 0x0f) << (position - 4)) | (1 << (position - 5))) - 33;
	return ((sign == 0) ? decoded : -decoded) / float((1 << 13) - 1);
}

#endif