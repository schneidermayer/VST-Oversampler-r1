#pragma once

#include <windows.h>
#define VST_FORCE_DEPRECATED 0
#include "pluginterfaces/vst2.x/aeffectx.h"

typedef unsigned int uint;

class VstWrapperInstance;

class VstWrapper
{
public:
	typedef AEffect* VstEntryProc(audioMasterCallback);

	VstWrapper();
	~VstWrapper();

	AEffect* createEffectInstance(audioMasterCallback audioMaster);

protected:
	typedef AEffect* VstEntry(audioMasterCallback);

	bool init(VstEntryProc* pfEntry);
	virtual VstWrapperInstance* createWrapperInstance(audioMasterCallback audioMaster) = 0;

	bool m_bInited;
	VstEntryProc* m_pfEntry;


	//
	// static
public:
	static VstWrapperInstance* _getCurrentInstance() { return ms_pvwiCurrentInstance; }
private:
	static CRITICAL_SECTION ms_csEntryLock;
	static VstWrapperInstance* ms_pvwiCurrentInstance;
	static LONG ms_cRef;
	static void _addRef();
	static void _releaseRef();
};
