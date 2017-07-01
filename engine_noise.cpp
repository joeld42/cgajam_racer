
#include <stdio.h>

#include "util.h"

#include "engine_noise.h"

static float my_fabs(float x)
{
    if (x < 0)
        return -x;
    return x;
}

EngineNoiseInstance::EngineNoiseInstance(EngineNoise *aParent)
{
	mParent = aParent;
	mOffset = 0;
	mWaveOffset = 0.0;
	mCurrFreq = 0.0;
}

void EngineNoiseInstance::getAudio(float *aBuffer, unsigned int aSamples)
{
	unsigned int i;
	
	mCurrFreq = (mCurrFreq * 9 + mParent->mFreq) / 10;

	float offsStep = mParent->mFreq;
	//printf("offstep %f waveoffs %f should be %f\n", 
	//	offsStep, mWaveOffset, mCurrFreq * mOffset );

	switch (mParent->mWaveform)
	{			
		case EngineNoise::SINE:
			for (i = 0; i < aSamples; i++)
			{
				//aBuffer[i] = (float)sin(mCurrFreq * mOffset * M_PI * 2);
				float v = (float)sin(mWaveOffset * M_PI * 2);// * pow( RandUniform(), 5.0);				
				//float thresh =  0.8 + pow( RandUniform(), 5.0) * 0.1;

				float drive = 90.0;
				float a = sin(((drive+1)/101.0)*(M_PI/2.0));
				float k = 2*a/(1-a);
				v = ((1+k)*(v)) / (1.0+k*fabs(v));

				float thresh = 0.9;
				if (v > 0.0) {
					aBuffer[i] = minf( v, thresh );
				} else {
					aBuffer[i] = maxf( v, -thresh );
				}
				mWaveOffset += offsStep;
				mOffset++;
				
			}
			break;
		case EngineNoise::SAW:
			for (i = 0; i < aSamples; i++)
			{
				//aBuffer[i] = (1 - (float)fmod(mCurrFreq * mOffset, 1)) * 2 - 1;
				aBuffer[i] = (1 - (float)fmod(mWaveOffset, 1)) * 2 - 1;
				mWaveOffset += offsStep;
				mOffset++;
			}
			break;				
		case EngineNoise::INVERSESAW:
			for (i = 0; i < aSamples; i++)
			{
				//aBuffer[i] = ((float)fmod(mCurrFreq * mOffset, 1)) * 2 - 1;
				aBuffer[i] = (1 - (float)fmod(mWaveOffset, 1)) * 2 - 1;
				mWaveOffset += offsStep;
				mOffset++;
			}
			break;				
		case EngineNoise::SQUARE:
			for (i = 0; i < aSamples; i++)
			{
				//aBuffer[i] = ((float)fmod(mCurrFreq * mOffset, 1.0f) > 0.5f) ? -1.0f : 1.0f;
				aBuffer[i] = ((float)fmod(mWaveOffset, 1.0f) > 0.5f) ? -1.0f : 1.0f;
				mWaveOffset += offsStep;
				mOffset++;
			}
			break;
		case EngineNoise::TRIANGLE:
			for (i = 0; i < aSamples; i++)
			{				
				//aBuffer[i] = my_fabs(0.5f - (float)fmod(mCurrFreq * mOffset, 1)) * 4 - 1;
				aBuffer[i] = my_fabs(0.5f - (float)fmod(mWaveOffset, 1)) * 4 - 1;
				mWaveOffset += offsStep;
				mOffset++;
			}
			break;
	}
}

bool EngineNoiseInstance::hasEnded()
{
	// This audio source never ends.
	return 0;
}

EngineNoise::EngineNoise()
{
	
	mWaveform = SQUARE;
	mPitch = 440.0;
	setSamplerate(44100);
}

EngineNoise::~EngineNoise()
{
	stop();
}

void EngineNoise::setSamplerate(float aSamplerate)
{
	mBaseSamplerate = aSamplerate;
	mFreq = (float)(mPitch / mBaseSamplerate);
}
void EngineNoise::setWaveform(int aWaveform)
{
	mWaveform = aWaveform;
}

void EngineNoise::setPitch(float aPitch)
{
	mPitch = aPitch;
	mFreq = (float)(mPitch / mBaseSamplerate);	
}

SoLoud::AudioSourceInstance * EngineNoise::createInstance() 
{
	return new EngineNoiseInstance(this);
}

