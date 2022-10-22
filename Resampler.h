#pragma once

#include "VstWrapper.h"

class Resampler : public VstWrapper
{
public:
	static Resampler* ms_pInstance;

	Resampler(HINSTANCE m_hInstance);
	~Resampler();

	bool init();

protected:
	VstWrapperInstance* createWrapperInstance(audioMasterCallback audioMaster);

private:
	HINSTANCE m_hInstance;
	HMODULE m_hClient;

};
