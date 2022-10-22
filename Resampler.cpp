/* Copyright (c) 2007, Christopher Walton
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the names of 
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Resampler.h"
#include "ResamplerInstance.h"

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
	switch(fdwReason)
	{
	case DLL_PROCESS_ATTACH: 
		{
			if(!Resampler::ms_pInstance) {
				Resampler::ms_pInstance = new Resampler(hInstance);
			}
			Resampler* pr = Resampler::ms_pInstance;

			if(!pr->init()) {
				delete Resampler::ms_pInstance;
				Resampler::ms_pInstance = NULL;
				return FALSE;		
			}
		} break;

	case DLL_PROCESS_DETACH:
		if(Resampler::ms_pInstance) {
			delete Resampler::ms_pInstance;
			Resampler::ms_pInstance = NULL;
		}
		break;
	}

	return TRUE;
}


AEffect* VSTPluginMain(audioMasterCallback audioMaster)
{
	if(!Resampler::ms_pInstance)
		return NULL;

	return Resampler::ms_pInstance->createEffectInstance(audioMaster);
}

AEffect* MAIN(audioMasterCallback audioMaster) 
{
	return VSTPluginMain(audioMaster); 
}

//
// class Resampler
//

Resampler* Resampler::ms_pInstance = NULL;

Resampler::Resampler(HINSTANCE hInstance)
:	m_hInstance(hInstance)
,	m_hClient(NULL)
{

}

Resampler::~Resampler()
{
	if(m_hClient)
	{
		FreeLibrary(m_hClient);
		m_hClient = NULL;
	}
}

VstWrapperInstance* Resampler::createWrapperInstance(audioMasterCallback audioMaster)
{
	return new ResamplerInstance(this, audioMaster);
}

bool Resampler::init()
{
	enum {
		MAX_PATH_SIZE = 1024,
	};

	char	pathName[MAX_PATH_SIZE + 1];

	if(!GetModuleFileName(m_hInstance, pathName, MAX_PATH_SIZE)) {
		MessageBox(0, "Couldn't get own .dll name - unrecoverable error", "ArkeCode VST Resampler", MB_OK | MB_ICONERROR);
		return false;
	}
	pathName[MAX_PATH_SIZE] = '\0'; // make sure the string is null-terminated

	size_t	len = strlen(pathName);
	char*	extension = pathName + (len - 7);

	if(strcmp(extension, ".os.dll") && strcmp(extension, "_os.dll")) {
		MessageBox(0, "Invalid .dll name - please consult Readme.txt", "ArkeCode VST Resampler", MB_OK | MB_ICONERROR);
		return false;
	}

	// transform path
	strcpy(extension, ".dll");

	// Load
	m_hClient = LoadLibrary(pathName);

	if(m_hClient == NULL) {
		MessageBox(0, "Couldn't load associated .dll", "ArkeCode VST Resampler", MB_OK | MB_ICONERROR);
		return false;
	}

	// Get entry point
	VstEntryProc* pfEntry = NULL;
	pfEntry = (VstEntryProc*) GetProcAddress(m_hClient, "VSTPluginMain");
	if(pfEntry == NULL) {
		pfEntry = (VstEntryProc*) GetProcAddress(m_hClient, "MAIN");
	}
	if(pfEntry == NULL) {
		pfEntry = (VstEntryProc*) GetProcAddress(m_hClient, "main");
	}
	if(pfEntry == NULL) {
		MessageBox(0, "Associated .dll does not seem to be a valid VST Plugin", "ArkeCode VST Resampler", MB_OK | MB_ICONERROR);
		return false;
	}

	return VstWrapper::init(pfEntry);
}

