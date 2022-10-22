#include "VstWrapper.h"
#include "VstWrapperInstance.h"

CRITICAL_SECTION VstWrapper::ms_csEntryLock;
VstWrapperInstance* VstWrapper::ms_pvwiCurrentInstance = NULL;
LONG VstWrapper::ms_cRef = 0;

void VstWrapper::_addRef()
{
	if(!ms_cRef) 
	{
		InitializeCriticalSection(&ms_csEntryLock);
	}
	ms_cRef++;
}

void VstWrapper::_releaseRef()
{
	ms_cRef--;
	if(!ms_cRef)
	{
		DeleteCriticalSection(&ms_csEntryLock);
	}
}

//
//
//

VstWrapper::VstWrapper()
:	m_bInited(false)
{
	_addRef();
}

VstWrapper::~VstWrapper()
{
	_releaseRef();
}

bool VstWrapper::init(VstEntryProc* pfEntry)
{
	m_pfEntry = pfEntry;
	m_bInited = true;
	return true;
}

AEffect* VstWrapper::createEffectInstance(audioMasterCallback audioMaster)
{
	if(!m_bInited) return NULL;

	VstWrapperInstance* pwvi = createWrapperInstance(audioMaster);
	if(!pwvi) return NULL;

	AEffect* paeff = NULL;

	EnterCriticalSection(&ms_csEntryLock);
	{
		ms_pvwiCurrentInstance = pwvi;
		paeff = m_pfEntry(audioMaster);
		ms_pvwiCurrentInstance = NULL;
	}
	LeaveCriticalSection(&ms_csEntryLock);

	if(!paeff)
	{
		delete pwvi;
		return NULL;
	}

	pwvi->setClient(paeff);
	return pwvi->getAeffect();
}

