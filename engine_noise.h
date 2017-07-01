#ifndef ENGINE_NOISE_H
#define ENGINE_NOISE_H

#include "soloud.h"

// bastardized version of the basicwave example

class EngineNoise;

class EngineNoiseInstance : public SoLoud::AudioSourceInstance
{
	EngineNoise *mParent;
	int mOffset;
	float mWaveOffset;
	float mCurrFreq;
public:
	EngineNoiseInstance(EngineNoise *aParent);
	virtual void getAudio(float *aBuffer, unsigned int aSamples);
	virtual bool hasEnded();
};

class EngineNoise : public SoLoud::AudioSource
{
public:
	enum WAVEFORMS
	{
		SINE,
		TRIANGLE,
		SQUARE,
		SAW,
		INVERSESAW
	};
	float mPitch;
	float mFreq;
	int mWaveform;
	EngineNoise();
	virtual ~EngineNoise();
	void setSamplerate(float aSamplerate);
	void setWaveform(int aWaveform);
	void setPitch(float aPitch);
	virtual SoLoud::AudioSourceInstance *createInstance();	
};

#endif