#include "VstWrapperInstance.h"
#include "VstWrapper.h"

VstWrapperInstance::VstWrapperInstance(VstWrapper* pWrapper, audioMasterCallback audioMaster)
:	m_paeffClient(NULL)
,	m_pWrapper(pWrapper)
,	m_audioMaster(audioMaster)
{
	memset(&m_aeffSelf, 0, sizeof(m_aeffSelf));
}

VstWrapperInstance::~VstWrapperInstance()
{
}

void VstWrapperInstance::rebuildAeffect()
{
	// usually a memcpy with soem special casing would be preferrable, and it's
	// what I did previously. However, some hosts (such as Chainer) seem to store
	// stuff in some weird places in the AEffect, so I changed it to use this 
	// method. This does mean, unfortunately, that future SDK updates could 
	// require an oversampler update (but since Steinborg is VST3-centered right
	// now anyway, ...)

	// copy everything not special
	m_aeffSelf.magic = m_paeffClient->magic;
	m_aeffSelf.numPrograms = m_paeffClient->numPrograms;
	m_aeffSelf.numParams = m_paeffClient->numParams;
	m_aeffSelf.numInputs = m_paeffClient->numInputs;
	m_aeffSelf.numOutputs = m_paeffClient->numOutputs;
	m_aeffSelf.flags = m_paeffClient->flags;
	m_aeffSelf.initialDelay = m_paeffClient->initialDelay;
	m_aeffSelf.realQualities = m_paeffClient->realQualities;
	m_aeffSelf.offQualities = m_paeffClient->offQualities;
	m_aeffSelf.ioRatio = m_paeffClient->ioRatio;
	m_aeffSelf.uniqueID = m_paeffClient->uniqueID;
	m_aeffSelf.version = m_paeffClient->version;
	
	// callbacks
	m_aeffSelf.processReplacing = vst_processReplacing;
	m_aeffSelf.process = vst_processAccumulating;
	m_aeffSelf.dispatcher = vst_dispatcher;
	m_aeffSelf.getParameter = vst_getParameter;
	m_aeffSelf.setParameter = vst_setParameter;
	m_aeffSelf.processDoubleReplacing = vst_processDoubleReplacing;

	// pointers to the wrapper in the AEffects
	// resvd1 is reserved for the host, and since we're the host
	// of the paeffClient store it there. For our own AEffect, we save
	// in user, not object, because some bad hosts (like REAPER)
	// might try to do some AudioEffectX specific stuff if something is there. 
	m_paeffClient->resvd1 = (VstIntPtr) this;
	m_aeffSelf.user = (void*) this;
}


//
// Static VST callbacks
//

VstIntPtr VstWrapperInstance::vst_audioMaster(AEffect* paeffClient, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
	VstWrapperInstance* pwvi = NULL;

	if(paeffClient == NULL) 
	{
		pwvi = VstWrapper::_getCurrentInstance(); 
	} 
	else if(paeffClient->resvd1 == NULL) 
	{
		pwvi = VstWrapper::_getCurrentInstance(); 
		// the client exists, but hasn't been set yet (probably because we're still in
		// the entry point somewhere). Set it manually here.
		pwvi->setClient(paeffClient);
	}
	else 
	{
		pwvi = VstWrapperInstance::getInstanceFromClientAeffect(paeffClient);
	}

	AudioMasterContext amc;
	amc.returnValue = 0;
	amc.opcode = opcode;
	amc.index = index;
	amc.value = value;
	amc.ptr = ptr;
	amc.opt = opt;

	bool bRet = pwvi->handlePreAudioMaster(amc);
	if(bRet) 
	{
		pwvi->performAudioMaster(amc);
		pwvi->handlePostAudioMaster(amc);
	}

	return amc.returnValue;
}

VstIntPtr VstWrapperInstance::vst_dispatcher(AEffect* paeffSelf, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt)
{
	VstWrapperInstance* pwvi = getInstanceFromWrapperAeffect(paeffSelf);

	DispatchContext dc;
	dc.returnValue = 0;
	dc.opcode = opcode;
	dc.index = index;
	dc.value = value;
	dc.ptr = ptr;
	dc.opt = opt;

	bool bRet = pwvi->handlePreDispatch(dc);
	if(bRet) 
	{
		pwvi->performDispatch(dc);
		pwvi->handlePostDispatch(dc);
	}

	return dc.returnValue;
}

void VstWrapperInstance::vst_processReplacing(AEffect* paeffSelf, float** inputs, float** outputs, VstInt32 sampleFrames)
{
	VstWrapperInstance* pwvi = getInstanceFromWrapperAeffect(paeffSelf);
	pwvi->processReplacing(inputs, outputs, sampleFrames);
}

void VstWrapperInstance::vst_processAccumulating(AEffect* paeffSelf, float** inputs, float** outputs, VstInt32 sampleFrames)
{
	VstWrapperInstance* pwvi = getInstanceFromWrapperAeffect(paeffSelf);
	pwvi->processAccumulating(inputs, outputs, sampleFrames);
}

void VstWrapperInstance::vst_processDoubleReplacing(AEffect* paeffSelf, double** inputs, double** outputs, VstInt32 sampleFrames)
{
	VstWrapperInstance* pwvi = getInstanceFromWrapperAeffect(paeffSelf);
	pwvi->processDoubleReplacing(inputs, outputs, sampleFrames);
}

float VstWrapperInstance::vst_getParameter(AEffect* paeffSelf, VstInt32 index)
{
	VstWrapperInstance* pwvi = getInstanceFromWrapperAeffect(paeffSelf);
	return pwvi->getParameter(index);
}

void VstWrapperInstance::vst_setParameter(AEffect* paeffSelf, VstInt32 index, float value)
{
	VstWrapperInstance* pwvi = getInstanceFromWrapperAeffect(paeffSelf);
	pwvi->setParameter(index, value);
}

//
// Default Handlers
//

bool VstWrapperInstance::handlePreDispatch(DispatchContext& dc)
{
	return true;
}

void VstWrapperInstance::handlePostDispatch(DispatchContext& dc)
{
	if(dc.opcode == effClose)
	{
		delete this;
		return;
	}
}

void VstWrapperInstance::performDispatch(DispatchContext& dc)
{
	dc.returnValue = m_paeffClient->dispatcher(m_paeffClient, dc.opcode, dc.index, dc.value, dc.ptr, dc.opt);
}

float VstWrapperInstance::getParameter(VstInt32 index)
{
	return m_paeffClient->getParameter(m_paeffClient, index);
}

void VstWrapperInstance::setParameter(VstInt32 index, float value)
{
	m_paeffClient->setParameter(m_paeffClient, index, value);
}

void VstWrapperInstance::processAccumulating(float** inputs, float** outputs, VstInt32 sampleFrames)
{
	m_paeffClient->process(m_paeffClient, inputs, outputs, sampleFrames);
}

void VstWrapperInstance::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
	m_paeffClient->processReplacing(m_paeffClient, inputs, outputs, sampleFrames);
}

void VstWrapperInstance::processDoubleReplacing(double** inputs, double** outputs, VstInt32 sampleFrames)
{
	m_paeffClient->processDoubleReplacing(m_paeffClient, inputs, outputs, sampleFrames);
}

void VstWrapperInstance::setClient(AEffect* paeff)
{
	m_paeffClient = paeff;
	rebuildAeffect();
}

bool VstWrapperInstance::handlePreAudioMaster(AudioMasterContext& amc)
{
	return true;
}

void VstWrapperInstance::handlePostAudioMaster(AudioMasterContext& amc)
{
}

void VstWrapperInstance::performAudioMaster(AudioMasterContext& amc)
{
	amc.returnValue = m_audioMaster(&m_aeffSelf, amc.opcode, amc.index, amc.value, amc.ptr, amc.opt);
}

