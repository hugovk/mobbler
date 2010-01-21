/*
mobbleraudiothread.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008  Michael Coffey

http://code.google.com/p/mobbler

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "mobbleraudiothread.h"
#include "mobblershareddata.h"
#include "mobblertrack.h"
#include "mobblerutility.h"

const TInt KBufferSizeInPackets(32);

TInt ThreadFunction(TAny* aData)
	{
	__UHEAP_MARK;
	
	CTrapCleanup* cleanupStack(CTrapCleanup::New());
	
	CActiveScheduler* activeScheduler(new CActiveScheduler);
	CActiveScheduler::Install(activeScheduler);
	
	CMobblerAudioThread* audioThread(NULL);
	
	TRAPD(error, audioThread = CMobblerAudioThread::NewL(aData));
	
	RThread().Rendezvous(KErrNone);
	
	if (error == KErrNone)
		{
		// If we're still here, the active scheduler has been constructed.
		// Start the wait loop which runs until it's time to end the thread.
		CActiveScheduler::Start();
		}
	
	delete audioThread;
	delete activeScheduler;
	delete cleanupStack;
	
	__UHEAP_MARKEND;
	
	return error;
	}

CMobblerAudioThread* CMobblerAudioThread::NewL(TAny* aData)
	{
	CMobblerAudioThread* self(new(ELeave) CMobblerAudioThread(aData));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerAudioThread::CMobblerAudioThread(TAny* aData)
	:CActive(CActive::EPriorityStandard), iShared(*static_cast<TMobblerSharedData*>(aData))
	{
	CActiveScheduler::Add(this);
	
	iShared.iPlaying = EFalse;
	iPreBufferOffset = 0;
	}

void CMobblerAudioThread::ConstructL()
	{
	Request();
	}

CMobblerAudioThread::~CMobblerAudioThread()
	{
	Cancel();
	
	if (iStream)
		{
		iStream->Stop();
		}
	
	delete iEqualizer;
	delete iStream;
	
	iBuffer.ResetAndDestroy();
	}

void CMobblerAudioThread::Request()
	{
	iShared.iCmdStatus = &iStatus;
	iStatus = KRequestPending;
	SetActive();
	}

void CMobblerAudioThread::RunL()
	{
	switch (iStatus.Int())
		{
		case ECmdSetVolume:
			{
			SetVolume();
			break;
			}
		case ECmdSetEqualizer:
			{
			SetEqualizerIndexL();
			break;
			}
		case ECmdWriteData:
			{
			iShared.iTrack->SetDataSize(iShared.iTotalDataSize);
			iShared.iTrack->BufferAdded(iShared.iAudioData.Size());
			
			// Put the data in the buffer
			iBuffer.AppendL(iShared.iAudioData.AllocLC());
			CleanupStack::Pop(); // aData.AllocLC()
			
			FillBufferL(ETrue);
			}
			break;
		case ECmdSetCurrent:
			{
			iStream = CMdaAudioOutputStream::NewL(*this);
			iStream->Open(&iShared.iAudioDataSettings);
			
			if (MobblerUtility::EqualizerSupported())
				{
				TRAP_IGNORE(iEqualizer = CAudioEqualizerUtility::NewL(*iStream));
				}
			}
			break;
		case ECmdDestroyAudio:
			{
			DestroyAudio();
			break;
			}
		}
	
	// Let the main thread know that we are ready for another command
	Request();
	RThread().Rendezvous(KErrNone);
	}

void CMobblerAudioThread::DoCancel()
	{
	TRequestStatus* status(&iStatus);
	User::RequestComplete(status, KErrCancel);
	}

TInt CMobblerAudioThread::RunError(TInt /*aError*/)
	{
	// There was an error in the RunL so just end the thread
	
	if (!iActiveSchedulerStopped)
		{
		iActiveSchedulerStopped = ETrue;
		CActiveScheduler::Stop();
		}
	return KErrNone;
	}

void CMobblerAudioThread::DestroyAudio()
	{
	if (!iActiveSchedulerStopped)
		{
		iActiveSchedulerStopped = ETrue;
		CActiveScheduler::Stop();
		}
	}

void CMobblerAudioThread::SetVolume()
	{
	if (iStream)
		{
		iStream->SetVolume(iShared.iAudioDataSettings.iVolume);
		}
	}

void CMobblerAudioThread::SetEqualizerIndexL()
	{
	if (iEqualizer)
		{
		if (iShared.iEqualizerIndex < 0)
			{
			iEqualizer->Equalizer().DisableL();
			}
		else
			{
			iEqualizer->Equalizer().EnableL();
			iEqualizer->ApplyPresetL(iShared.iEqualizerIndex);
			}
		}
	}

TBool CMobblerAudioThread::PreBufferFilled() const
	{
	TBool preBufferedFilled(EFalse);
	
	if (iShared.iTrack->DataSize() != 1)
		{
		TInt byteRate(iShared.iTrack->DataSize() / iShared.iTrack->TrackLength().Int());
		TInt buffered(iShared.iTrack->Buffered() - iPreBufferOffset.Int());
		
		if (byteRate != 0)
			{
			TInt secondsBuffered(buffered / byteRate);
			
			// either buffered amount of time or the track has finished downloading
			preBufferedFilled = secondsBuffered >= iShared.iPreBufferSize.Int() || iShared.iTrack->Buffered() == iShared.iTrack->DataSize();
			}
		}
	else
		{
		preBufferedFilled = iBuffer.Count() >= KBufferSizeInPackets;
		}
	
	return preBufferedFilled;
	}

void CMobblerAudioThread::FillBufferL(TBool aDataAdded)
	{
	if (iShared.iPlaying)
		{
		if (aDataAdded)
			{
			// we are already playing so add the last
			//piece of the buffer to the stream
			iStream->WriteL(*iBuffer[iBuffer.Count() - 1]);
			}
		}
	else if (!iShared.iPlaying && iOpen && iShared.iCurrent && PreBufferFilled())
		{
		// Start playing the track
		iShared.iPlaying = ETrue;
		
		// write all the data we have to the audio output stream
		const TInt KBufferCount(iBuffer.Count());
		for (TInt i(0); i < KBufferCount; ++i)
			{
			iStream->WriteL(*iBuffer[i]);
			}
		}
	}

void CMobblerAudioThread::MaoscBufferCopied(TInt /*aError*/, const TDesC8& /*aBuffer*/)
	{
	delete iBuffer[0];
	iBuffer.Remove(0);
	
	const TInt KMicrosecondsInOneSecond(1000000);
	iShared.iTrack->SetPlaybackPosition(iStream->Position().Int64() / KMicrosecondsInOneSecond);
	
	if (iBuffer.Count() == 0)
		{
		if (iShared.iDownloadComplete)
			{
			// We don't have anything to write to the audio output stream
			// and the mp3 download has finished so exit
			
			if (!iActiveSchedulerStopped)
				{
				iActiveSchedulerStopped = ETrue;
				CActiveScheduler::Stop();
				}
			}
		else
			{
			// There is more data on its way so start buffering again
			iShared.iPlaying = EFalse;
			iPreBufferOffset = iShared.iTrack->Buffered();
			}
		}
	}

void CMobblerAudioThread::MaoscOpenComplete(TInt /*aError*/)
	{
	TRAP_IGNORE(iStream->SetDataTypeL(KMMFFourCCCodeMP3));
	iStream->SetAudioPropertiesL(iShared.iAudioDataSettings.iSampleRate, iShared.iAudioDataSettings.iChannels);
	iStream->SetVolume(iShared.iAudioDataSettings.iVolume);
	iShared.iMaxVolume = iStream->MaxVolume();
	
	SetEqualizerIndexL();
	
	iOpen = ETrue;
	FillBufferL(EFalse);
	}

void CMobblerAudioThread::MaoscPlayComplete(TInt /*aError*/)
	{
	}

// End of file
