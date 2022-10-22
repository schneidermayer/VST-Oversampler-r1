#include "ResamplerInstance.h"
#include "Resampler.h"
#include <assert.h>

#define SPEEX_RESAMPLER_QUALITY SPEEX_RESAMPLER_QUALITY_MIN

ResamplerInstance::ResamplerInstance(Resampler* pMaster, audioMasterCallback audioMaster)
:	VstWrapperInstance(pMaster, audioMaster)
,	m_fHostRate(44100.0f)
,	m_rmMode(MODE_OS)
,	m_fResamplerParam(2.0f)
,	m_cBlockSize(1024)
,	m_ppfInputBuffers(NULL)
,	m_ppfOutputBuffers(NULL)
,	m_pInputResampler(NULL)
,	m_pOutputResampler(NULL)
{
	
}

ResamplerInstance::~ResamplerInstance()
{

}

void ResamplerInstance::_computeClientRate()
{
	switch(m_rmMode)
	{
	case MODE_US:
		m_fClientRate = m_fHostRate / m_fResamplerParam;
		break;

	case MODE_OS:
		m_fClientRate = m_fHostRate * m_fResamplerParam;
		break;

	case MODE_RS:
		m_fClientRate = m_fResamplerParam;
		break;
	}
}

void ResamplerInstance::setMode(ResamplerMode rm, float fParam)
{
	m_rmMode = rm;
	m_fResamplerParam = fParam;
	_computeClientRate();
}

bool ResamplerInstance::handlePreDispatch(DispatchContext& dc)
{
	bool bRet = true;

	switch(dc.opcode)
	{
	case effSetSampleRate:
		m_fHostRate = dc.opt;
		_computeClientRate();
		dc.opt = m_fClientRate;
		break;

	case effSetBlockSize:
		m_cBlockSize = dc.value;
		m_cClientBlockSize = (uint) ((m_fClientRate / m_fHostRate) * (float) m_cBlockSize) + 1;
		break;

	case effMainsChanged:
		if(dc.value) 
			resume();
		else
			suspend();
		bRet = false;
		break;
	}

	return bRet;
}

void ResamplerInstance::resume()
{
	if(!m_ppfInputBuffers)
	{
		m_ppfInputBuffers = new float*[m_paeffClient->numInputs];

		for(uint i = 0; i < m_paeffClient->numInputs; i++)
		{
			m_ppfInputBuffers[i] = new float[m_cClientBlockSize];
		}
	}

	if(!m_ppfOutputBuffers)
	{
		m_ppfOutputBuffers = new float*[m_paeffClient->numOutputs];

		for(uint i = 0; i < m_paeffClient->numOutputs; i++)
		{
			m_ppfOutputBuffers[i] = new float[m_cClientBlockSize];
		}
	}
	
	DispatchContext dc;
	dc.opcode = effMainsChanged;
	dc.value = 1;
	performDispatch(dc);

	if(!m_pInputResampler)
	{
		m_pInputResampler = speex_resampler_init(m_paeffClient->numInputs, 
			(spx_uint32_t) m_fHostRate, (spx_uint32_t) m_fClientRate, SPEEX_RESAMPLER_QUALITY, NULL);
	}
	if(!m_pOutputResampler)
	{
		m_pOutputResampler = speex_resampler_init(m_paeffClient->numOutputs, 
			(spx_uint32_t) m_fClientRate, (spx_uint32_t) m_fHostRate, SPEEX_RESAMPLER_QUALITY, NULL);
	}
}

void ResamplerInstance::suspend()
{
	DispatchContext dc;
	dc.opcode = effMainsChanged;
	dc.value = 0;
	performDispatch(dc);

	if(m_ppfInputBuffers)
	{
		for(uint i = 0; i < m_paeffClient->numInputs; i++)
		{
			if(m_ppfInputBuffers[i])
			{
				delete[] m_ppfInputBuffers[i];
				m_ppfInputBuffers[i] = NULL;
			}
		}

		delete[] m_ppfInputBuffers;
		m_ppfInputBuffers = NULL;
	}

	if(m_ppfOutputBuffers)
	{
		for(uint i = 0; i < m_paeffClient->numOutputs; i++)
		{
			if(m_ppfOutputBuffers[i])
			{
				delete[] m_ppfOutputBuffers[i];
				m_ppfOutputBuffers[i] = NULL;
			}
		}

		delete[] m_ppfOutputBuffers;
		m_ppfOutputBuffers = NULL;
	}

	speex_resampler_destroy(m_pInputResampler);
	m_pInputResampler = NULL;
	speex_resampler_destroy(m_pOutputResampler);
	m_pOutputResampler = NULL;
}

void ResamplerInstance::processAccumulating(float** inputs, float** outputs, VstInt32 nFrames)
{
	__asm int 3;
	VstWrapperInstance::processAccumulating(inputs, outputs, nFrames);
}

//void ResamplerInstance::processReplacing(float** hostInputs, float** hostOutputs, VstInt32 nFrames)
void ResamplerInstance::processReplacing(float** inputs, float** outputs, VstInt32 nFrames)
{
/*	// create our own copies so that we can write in them
	float* inputs[m_paeffClient->numInputs];
	memcpy(&inputs, &hostInputs, sizeof(float*) * m_paeffClient->numInputs);
	float* outputs[m_paeffClient->numOutputs];
	memcpy(&outputs, &hostOutputs, sizeof(float*) * m_paeffClient->numOutputs);*/

	while(nFrames)
	{
		spx_uint32_t up_in_len = nFrames;
		spx_uint32_t up_out_len = m_cClientBlockSize;

		for(uint i = 0; i < m_paeffClient->numInputs; i++) 
		{
			speex_resampler_process_float(m_pInputResampler, i, inputs[i], &up_in_len, m_ppfInputBuffers[i], &up_out_len);
		}

		assert(up_in_len == nFrames);

		VstWrapperInstance::processReplacing(m_ppfInputBuffers, m_ppfOutputBuffers, up_out_len);

		spx_uint32_t dn_in_len = up_out_len;
		spx_uint32_t dn_out_len = nFrames;

		for(uint i = 0; i < m_paeffClient->numOutputs; i++) 
		{
			speex_resampler_process_float(m_pOutputResampler, i, m_ppfOutputBuffers[i], &dn_in_len, outputs[i], &dn_out_len);
		}

		nFrames -= dn_out_len;
		if(nFrames)
		{
			for(uint i = 0; i < m_paeffClient->numInputs; i++) 
			{
				inputs[i] += dn_out_len;
			}
			for(uint i = 0; i < m_paeffClient->numOutputs; i++) 
			{
				outputs[i] += dn_out_len;
			}
		}
	}
}

