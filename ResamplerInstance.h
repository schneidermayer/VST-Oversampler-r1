#pragma once

#include "VstWrapperInstance.h"
#include "speex_resampler.h"

class Resampler;
class ResamplerInstance : public VstWrapperInstance
{
public:
	enum ResamplerMode {
		MODE_OS,
		MODE_US,
		MODE_RS,
	};

	ResamplerInstance(Resampler* pMaster, audioMasterCallback audioMaster);
	~ResamplerInstance();

	void setMode(ResamplerMode rm, float fParam);

protected:
	bool handlePreDispatch(DispatchContext& dc);

	void processAccumulating(float** inputs, float** outputs, VstInt32 nFrames);
	void processReplacing(float** inputs, float** outputs, VstInt32 nFrames);

	void resume();
	void suspend();

private:
	void _computeClientRate();

	ResamplerMode m_rmMode;
	float m_fResamplerParam;

	float m_fHostRate;
	float m_fClientRate;
	uint m_cBlockSize;
	uint m_cClientBlockSize;

	float** m_ppfInputBuffers;
	float** m_ppfOutputBuffers;

	SpeexResamplerState* m_pInputResampler;
	SpeexResamplerState* m_pOutputResampler;

};
