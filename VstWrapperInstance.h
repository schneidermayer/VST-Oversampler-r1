#pragma once

#include "VstWrapper.h"

class VstWrapperInstance
{
public:
	static VstWrapperInstance* getInstanceFromClientAeffect(AEffect* paeff) { return (VstWrapperInstance*) paeff->resvd1; }
	static VstWrapperInstance* getInstanceFromWrapperAeffect(AEffect* paeff) { return (VstWrapperInstance*) paeff->user; }

public:
	VstWrapperInstance(VstWrapper* pWrapper, audioMasterCallback audioMaster);
	~VstWrapperInstance();

	void setClient(AEffect* paeff);
	AEffect* getAeffect() { return &m_aeffSelf; }

protected:
	struct DispatchContext
	{
		VstIntPtr returnValue;
		VstInt32 opcode;
		VstInt32 index;
		VstIntPtr value;
		void* ptr;
		float opt;
	};
	typedef DispatchContext AudioMasterContext;

	/**
	 * Is called before a dispatch on the hosted plugin is called.
	 * @return	true if the dispatch should be done, false otherwise
	 */
	virtual bool handlePreDispatch(DispatchContext& dc);
	virtual void handlePostDispatch(DispatchContext& dc);
	void performDispatch(DispatchContext& dc);

	virtual bool handlePreAudioMaster(AudioMasterContext& amc);
	virtual void handlePostAudioMaster(AudioMasterContext& amc);
	void performAudioMaster(AudioMasterContext& dc);

	virtual float getParameter(VstInt32 index);
	virtual void setParameter(VstInt32 index, float value);
	virtual void processAccumulating(float** inputs, float** outputs, VstInt32 sampleFrames);
	virtual void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames);
	virtual void processDoubleReplacing(double** inputs, double** outputs, VstInt32 sampleFrames);

	virtual void rebuildAeffect();

	AEffect m_aeffSelf;
	AEffect* m_paeffClient;
	VstWrapper* m_pWrapper;
	audioMasterCallback m_audioMaster;

private:

	static VstIntPtr vst_audioMaster(AEffect* paeffSelf, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
	static VstIntPtr vst_dispatcher(AEffect* paeffSelf, VstInt32 opcode, VstInt32 index, VstIntPtr value, void* ptr, float opt);
	static void vst_processReplacing(AEffect* paeffSelf, float** inputs, float** outputs, VstInt32 sampleFrames);
	static void vst_processAccumulating(AEffect* paeffSelf, float** inputs, float** outputs, VstInt32 sampleFrames);
	static void vst_processDoubleReplacing(AEffect* paeffSelf, double** inputs, double** outputs, VstInt32 sampleFrames);
	static float vst_getParameter(AEffect* paeffSelf, VstInt32 index);
	static void vst_setParameter(AEffect* paeffSelf, VstInt32 index, float value);

};
