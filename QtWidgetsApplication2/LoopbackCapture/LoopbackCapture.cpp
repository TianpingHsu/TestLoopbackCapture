#include <shlobj.h>
#include <wchar.h>
#include <iostream>
#include <audioclientactivationparams.h>

#include "LoopbackCapture.h"
#include <fstream>
#include <thread>
#include <chrono>

#define BITS_PER_BYTE 8

HRESULT CLoopbackCapture::SetDeviceStateErrorIfFailed(HRESULT hr)
{
	if (FAILED(hr))
	{
		m_DeviceState = DeviceState::Error;
		lparam.state = 1;
	}
	return hr;
}

HRESULT CLoopbackCapture::InitializeLoopbackCapture()
{
	LOG_INFO("InitializeLoopbackCapture Starting\n");
	//LOG_INFO("InitializeLoopbackCapture Starting, CoInitilizeEx\n");
	//RETURN_IF_FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED));
	//RETURN_IF_FAILED(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
	//COINIT_APARTMENTTHREADED  = 0x2,      // Apartment model

	// Create events for sample ready or user stop
	LOG_INFO("create sample ready event\n");
	RETURN_IF_FAILED(m_SampleReadyEvent.create(wil::EventOptions::None));

	// Initialize MF
	LOG_INFO("initialize MF\n");
	RETURN_IF_FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE));

	// Register MMCSS work queue
	LOG_INFO("register mmcss work queue\n");
	DWORD dwTaskID = 0;
	RETURN_IF_FAILED(MFLockSharedWorkQueue(L"Capture", 0, &dwTaskID, &m_dwQueueID));

	// Set the capture event work queue to use the MMCSS queue
	m_xSampleReady.SetQueueID(m_dwQueueID);

	// Create the completion event as auto-reset
	LOG_INFO("create the completion event as auto-reset\n");
	RETURN_IF_FAILED(m_hActivateCompleted.create(wil::EventOptions::None));

	// Create the capture-stopped event as auto-reset
	LOG_INFO("create the capture-stopped event as auto-reset\n");
	RETURN_IF_FAILED(m_hCaptureStopped.create(wil::EventOptions::None));

	LOG_INFO("InitializeLoopbackCapture DONE\n");
	return S_OK;
}

CLoopbackCapture::~CLoopbackCapture()
{
	StopCaptureAsync();
	if (m_dwQueueID != 0)
	{
		MFUnlockWorkQueue(m_dwQueueID);
	}
	//CoUninitialize();
}

HRESULT CLoopbackCapture::ActivateAudioInterface(DWORD processId, bool includeProcessTree)
{
	return SetDeviceStateErrorIfFailed([&]() -> HRESULT
		{
			AUDIOCLIENT_ACTIVATION_PARAMS audioclientActivationParams = {};
			audioclientActivationParams.ActivationType = AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;
			audioclientActivationParams.ProcessLoopbackParams.ProcessLoopbackMode = includeProcessTree ?
				PROCESS_LOOPBACK_MODE_INCLUDE_TARGET_PROCESS_TREE : PROCESS_LOOPBACK_MODE_EXCLUDE_TARGET_PROCESS_TREE;
			audioclientActivationParams.ProcessLoopbackParams.TargetProcessId = processId;

			PROPVARIANT activateParams = {};
			activateParams.vt = VT_BLOB;
			activateParams.blob.cbSize = sizeof(audioclientActivationParams);
			activateParams.blob.pBlobData = (BYTE*)&audioclientActivationParams;

			LOG_INFO("start activation...\n");
			wil::com_ptr_nothrow<IActivateAudioInterfaceAsyncOperation> asyncOp;
			RETURN_IF_FAILED(ActivateAudioInterfaceAsync(VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK, __uuidof(IAudioClient), &activateParams, this, &asyncOp));

			LOG_INFO("waiting for completing activation\n");

			// Wait for activation completion
			m_hActivateCompleted.wait();
			
			LOG_INFO("activate result: %d\n", m_activateResult);
			return m_activateResult;
		}());
}

//
//  ActivateCompleted()
//
//  Callback implementation of ActivateAudioInterfaceAsync function.  This will be called on MTA thread
//  when results of the activation are available.
//
HRESULT CLoopbackCapture::ActivateCompleted(IActivateAudioInterfaceAsyncOperation* operation)
{
	m_activateResult = SetDeviceStateErrorIfFailed([&]()->HRESULT
		{
			// Check for a successful activation result
			HRESULT hrActivateResult = E_UNEXPECTED;
			wil::com_ptr_nothrow<IUnknown> punkAudioInterface;
			RETURN_IF_FAILED(operation->GetActivateResult(&hrActivateResult, &punkAudioInterface));
			LOG_INFO("operation->GetActiveResult: %d\n", hrActivateResult);
			RETURN_IF_FAILED(hrActivateResult);

			// Get the pointer for the Audio Client
			RETURN_IF_FAILED(punkAudioInterface.copy_to(&m_AudioClient));
			LOG_INFO("m_AudioClient: %p\n", m_AudioClient.get());

			// The app can also call m_AudioClient->GetMixFormat instead to get the capture format.
			// 16 - bit PCM format.
			WAVEFORMATEX* pWfxOut = NULL;
			LOG_INFO("m_AudioClient->GetMixFormat\n");
			//RETURN_IF_FAILED(
			HRESULT tm = m_AudioClient->GetMixFormat(&pWfxOut);
			//);
			LOG_INFO("m_AudioClient->GetMixFormat OK: %d!\n", tm);
			/*
			m_CaptureFormat.wFormatTag = pWfxOut->wFormatTag;
			m_CaptureFormat.nChannels = pWfxOut->nChannels;
			m_CaptureFormat.nSamplesPerSec = pWfxOut->nSamplesPerSec;
			m_CaptureFormat.wBitsPerSample = pWfxOut->wBitsPerSample;
			m_CaptureFormat.nBlockAlign = pWfxOut->nBlockAlign;
			m_CaptureFormat.nAvgBytesPerSec = pWfxOut->nAvgBytesPerSec;
			*/
				m_CaptureFormat.wFormatTag = WAVE_FORMAT_PCM;
				m_CaptureFormat.nChannels = 2;
				m_CaptureFormat.nSamplesPerSec = 48000;
				m_CaptureFormat.wBitsPerSample = 16;
				m_CaptureFormat.nBlockAlign = m_CaptureFormat.nChannels * m_CaptureFormat.wBitsPerSample / BITS_PER_BYTE;
				m_CaptureFormat.nAvgBytesPerSec = m_CaptureFormat.nSamplesPerSec * m_CaptureFormat.nBlockAlign;
				LOG_INFO("format: %d, channel: %d, samplePerSec: %d, bitsPerSample: %d, blockAlign: %d, avgBytesPerSec: %d\n",
				m_CaptureFormat.wFormatTag, 
				m_CaptureFormat.nChannels, 
				m_CaptureFormat.nSamplesPerSec, 
				m_CaptureFormat.wBitsPerSample, 
				m_CaptureFormat.nBlockAlign, 
				m_CaptureFormat.nAvgBytesPerSec);

			// Initialize the AudioClient in Shared Mode with the user specified buffer
			RETURN_IF_FAILED(m_AudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
				AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
				200000,
				AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM,
				&m_CaptureFormat,
				nullptr));
			LOG_INFO("m_AudioClient->Initialize DONE");

			// Get the maximum size of the AudioClient Buffer
			RETURN_IF_FAILED(m_AudioClient->GetBufferSize(&m_BufferFrames));
			LOG_INFO("m_AudioClient->GetBufferSize DONE");

			// Get the capture client
			RETURN_IF_FAILED(m_AudioClient->GetService(IID_PPV_ARGS(&m_AudioCaptureClient)));
			LOG_INFO("m_AudioClient->GetService DONE");

			// Create Async callback for sample events
			RETURN_IF_FAILED(MFCreateAsyncResult(nullptr, &m_xSampleReady, nullptr, &m_SampleReadyAsyncResult));
			LOG_INFO("MFCreateAsyncResult DONE");

			// Tell the system which event handle it should signal when an audio buffer is ready to be processed by the client
			RETURN_IF_FAILED(m_AudioClient->SetEventHandle(m_SampleReadyEvent.get()));
			LOG_INFO("m_AudioClient->SetEventHandle DONE");

			// Creates the WAV file.
			//RETURN_IF_FAILED(CreateWAVFile());

			// Everything is ready.
			m_DeviceState = DeviceState::Initialized;
			lparam.state = 2;

			return S_OK;
		}());

	// Let ActivateAudioInterface know that m_activateResult has the result of the activation attempt.
	m_hActivateCompleted.SetEvent();
	return S_OK;
}

HRESULT CLoopbackCapture::StartCaptureAsync()
{
	RETURN_IF_FAILED(InitializeLoopbackCapture());
	DWORD processId = GetCurrentProcessId();
	bool includeProcessTree = false;
	LOG_INFO("StartCaptureAsync starting...\n");
	RETURN_IF_FAILED(ActivateAudioInterface(processId, includeProcessTree));
	LOG_INFO("StartCaptureAsync DONE\n");

	// We should be in the initialzied state if this is the first time through getting ready to capture.
	if (m_DeviceState == DeviceState::Initialized)
	{
		m_DeviceState = DeviceState::Starting;
		lparam.state = 3;
		return MFPutWorkItem2(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, 0, &m_xStartCapture, nullptr);
	}

	return S_OK;
}

//
//  OnStartCapture()
//
//  Callback method to start capture
//
HRESULT CLoopbackCapture::OnStartCapture(IMFAsyncResult* pResult)
{
	return SetDeviceStateErrorIfFailed([&]()->HRESULT
		{
			// Start the capture
			RETURN_IF_FAILED(m_AudioClient->Start());

			m_DeviceState = DeviceState::Capturing;
			lparam.state = 4;
			MFPutWaitingWorkItem(m_SampleReadyEvent.get(), 0, m_SampleReadyAsyncResult.get(), &m_SampleReadyKey);

			return S_OK;
		}());
}


//
//  StopCaptureAsync()
//
//  Stop capture asynchronously via MF Work Item
//
HRESULT CLoopbackCapture::StopCaptureAsync()
{
	RETURN_HR_IF(E_NOT_VALID_STATE, (m_DeviceState != DeviceState::Capturing) &&
		(m_DeviceState != DeviceState::Error));

	m_DeviceState = DeviceState::Stopping;
	lparam.state = 5;
	RETURN_IF_FAILED(MFPutWorkItem2(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, 0, &m_xStopCapture, nullptr));

	// Wait for capture to stop
	m_hCaptureStopped.wait();

	return S_OK;
}

//
//  OnStopCapture()
//
//  Callback method to stop capture
//
HRESULT CLoopbackCapture::OnStopCapture(IMFAsyncResult* pResult)
{
	// Stop capture by cancelling Work Item
	// Cancel the queued work item (if any)
	if (0 != m_SampleReadyKey)
	{
		MFCancelWorkItem(m_SampleReadyKey);
		m_SampleReadyKey = 0;
	}

	if (m_AudioClient) m_AudioClient->Stop();
	m_SampleReadyAsyncResult.reset();

	return FinishCaptureAsync();
}

//
//  FinishCaptureAsync()
//
//  Finalizes WAV file on a separate thread via MF Work Item
//
HRESULT CLoopbackCapture::FinishCaptureAsync()
{
	// We should be flushing when this is called
	return MFPutWorkItem2(MFASYNC_CALLBACK_QUEUE_MULTITHREADED, 0, &m_xFinishCapture, nullptr);
}

//
//  OnFinishCapture()
//
//  Because of the asynchronous nature of the MF Work Queues and the DataWriter, there could still be
//  a sample processing.  So this will get called to finalize the WAV header.
//
HRESULT CLoopbackCapture::OnFinishCapture(IMFAsyncResult* pResult)
{
	m_DeviceState = DeviceState::Stopped;
	lparam.state = 6;

	m_hCaptureStopped.SetEvent();

	return S_OK;
}

//
//  OnSampleReady()
//
//  Callback method when ready to fill sample buffer
//
HRESULT CLoopbackCapture::OnSampleReady(IMFAsyncResult* pResult)
{
	if (SUCCEEDED(OnAudioSampleRequested()))
	{
		// Re-queue work item for next sample
		if (m_DeviceState == DeviceState::Capturing)
		{
			// Re-queue work item for next sample
			return MFPutWaitingWorkItem(m_SampleReadyEvent.get(), 0, m_SampleReadyAsyncResult.get(), &m_SampleReadyKey);
		}
	}
	else
	{
		m_DeviceState = DeviceState::Error;
		lparam.state = 1;
	}

	return S_OK;
}

//
//  OnAudioSampleRequested()
//
//  Called when audio device fires m_SampleReadyEvent
//
HRESULT CLoopbackCapture::OnAudioSampleRequested()
{
	UINT32 FramesAvailable = 0;
	BYTE* Data = nullptr;
	DWORD dwCaptureFlags;
	UINT64 u64DevicePosition = 0;
	UINT64 u64QPCPosition = 0;
	DWORD cbBytesToCapture = 0;

	auto lock = m_CritSec.lock();

	// If this flag is set, we have already queued up the async call to finialize the WAV header
	// So we don't want to grab or write any more data that would possibly give us an invalid size
	if (m_DeviceState == DeviceState::Stopping)
	{
		return S_OK;
	}
	if (lparam.len != 0) {
		//return S_OK;
	}

	// A word on why we have a loop here;
	// Suppose it has been 10 milliseconds or so since the last time
	// this routine was invoked, and that we're capturing 48000 samples per second.
	//
	// The audio engine can be reasonably expected to have accumulated about that much
	// audio data - that is, about 480 samples.
	//
	// However, the audio engine is free to accumulate this in various ways:
	// a. as a single packet of 480 samples, OR
	// b. as a packet of 80 samples plus a packet of 400 samples, OR
	// c. as 48 packets of 10 samples each.
	//
	// In particular, there is no guarantee that this routine will be
	// run once for each packet.
	//
	// So every time this routine runs, we need to read ALL the packets
	// that are now available;
	//
	// We do this by calling IAudioCaptureClient::GetNextPacketSize
	// over and over again until it indicates there are no more packets remaining.
	while (SUCCEEDED(m_AudioCaptureClient->GetNextPacketSize(&FramesAvailable)) && FramesAvailable > 0)
	{
		cbBytesToCapture = FramesAvailable * m_CaptureFormat.nBlockAlign;

		// WAV files have a 4GB (0xFFFFFFFF) size limit, so likely we have hit that limit when we
		// overflow here.  Time to stop the capture
		if (lparam.len > lparam.MAX_LEN)
		{
			//StopCaptureAsync();
			//LOG_INFO("bad len: %d : %d\n", lparam.len.load(), lparam.MAX_LEN);
			//break;
		}

		// Get sample buffer
		RETURN_IF_FAILED(m_AudioCaptureClient->GetBuffer(&Data, &FramesAvailable, &dwCaptureFlags, &u64DevicePosition, &u64QPCPosition));


		// Write File
		if (m_DeviceState != DeviceState::Stopping)
		{
			/*
			CopyMemory((lparam.data + lparam.len), Data, cbBytesToCapture);
			lparam.len += cbBytesToCapture;
			*/
			std::ofstream ofs;
			ofs.open("C:\\AudioCapture.pcm", std::ios::app | std::ios::binary);
			ofs.write(reinterpret_cast<const char*>(Data), cbBytesToCapture);
			ofs.close();
		}

		// Release buffer back
		m_AudioCaptureClient->ReleaseBuffer(FramesAvailable);
	}
	lparam.ready = true;

	return S_OK;
}
