/*
Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009  Steve Punter
Copyright (C) 2009, 2010, 2011  Michael Coffey
Copyright (C) 2009, 2010  Hugo van Kemenade

http://code.google.com/p/mobbler

This file is part of Mobbler.

Mobbler is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

Mobbler is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Mobbler.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mobbleraudiothread.h"
#include "mobblershareddata.h"
#include "mobblertracer.h"
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
    TRACER_AUTO;
	CMobblerAudioThread* self(new(ELeave) CMobblerAudioThread(aData));
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}

CMobblerAudioThread::CMobblerAudioThread(TAny* aData)
	:CActive(CActive::EPriorityStandard), iShared(*static_cast<TMobblerSharedData*>(aData))
	{
    TRACER_AUTO;
	CActiveScheduler::Add(this);
	
	iShared.iPlaying = EFalse;
	iShared.iPaused = EFalse;
	iPreBufferOffset = 0;
	iSavedPosition = 0;
	}

void CMobblerAudioThread::ConstructL()
	{
    TRACER_AUTO;
	Request();
	}

CMobblerAudioThread::~CMobblerAudioThread()
	{
    TRACER_AUTO;
	iShared.iAbnormalTermination = EFalse; // Hooray!
	Cancel();
	
	if (iStream)
		{
		iStream->Stop();
		}
	else if (iPlayer)
		{
		iPlayer->Stop();
		}
	
	delete iEqualizer;
	delete iStream;
	delete iPlayer;
	delete iPeriodic;
	
	iBuffer.ResetAndDestroy();
	}

void CMobblerAudioThread::Request()
	{
//	TRACER_AUTO;
	iShared.iCmdStatus = &iStatus;
	iStatus = KRequestPending;
	SetActive();
	}

void CMobblerAudioThread::RunL()
	{
    TRACER_AUTO;
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
		case ECmdPause:
			{
			if (iStream)
				{
				iShared.iPaused = !iShared.iPaused;
				
				if (iShared.iPaused)
					{
					iStream->Stop();
					}
				else if (iBuffer.Count() > 0)
					{
					iStream->WriteL(*iBuffer[0]);
					}
				}
			
			if (iPlayer)
				{
				if (iShared.iPaused)
					{
					iPlayer->Play();
					}
				else
					{
					User::LeaveIfError(iPlayer->Pause());
					}
				
				iShared.iPaused = !iShared.iPaused;
				}
			}
			break;
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
			if (iShared.iTrack->LocalFile().Length() > 0)
				{
				// it is a local track so just play it
				iPlayer = CMdaAudioPlayerUtility::NewFilePlayerL(iShared.iTrack->LocalFile(), *this);
				
				if (MobblerUtility::EqualizerSupported())
					{
					TRAP_IGNORE(iEqualizer = CAudioEqualizerUtility::NewL(*iPlayer));
					}
				}
			else
				{
				iStream = CMdaAudioOutputStream::NewL(*this);
				iStream->Open(&iShared.iAudioDataSettings);
				
				if (MobblerUtility::EqualizerSupported())
					{
					TRAP_IGNORE(iEqualizer = CAudioEqualizerUtility::NewL(*iStream));
					}
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
    TRACER_AUTO;
	TRequestStatus* status(&iStatus);
	User::RequestComplete(status, KErrCancel);
	}

TInt CMobblerAudioThread::RunError(TInt /*aError*/)
	{
    TRACER_AUTO;
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
    TRACER_AUTO;
	if (!iActiveSchedulerStopped)
		{
		iActiveSchedulerStopped = ETrue;
		CActiveScheduler::Stop();
		}
	}

void CMobblerAudioThread::SetVolume()
	{
    TRACER_AUTO;
	if (iStream)
		{
		iStream->SetVolume(iShared.iAudioDataSettings.iVolume);
		}
	else if (iPlayer)
		{
		iPlayer->SetVolume(iShared.iAudioDataSettings.iVolume);
		}
	}

void CMobblerAudioThread::SetEqualizerIndexL()
	{
    TRACER_AUTO;
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
    TRACER_AUTO;
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
//	TRACER_AUTO;
	if (iShared.iPlaying)
		{
		if (aDataAdded)
			{
			// we are already playing so add the last
			// piece of the buffer to the stream
			//iStream->WriteL(*iBuffer[iBuffer.Count() - 1]);
			}
		}
	else if (!iShared.iPlaying && iOpen && iShared.iCurrent && PreBufferFilled())
		{
		// Start playing the track
		iShared.iPlaying = ETrue;
		
		// write all the data we have to the audio output stream
		if (iBuffer.Count() > 0)
			{
			if (!iShared.iPaused)
				{
				iStream->WriteL(*iBuffer[0]);
				}
			}
		}
	}

void CMobblerAudioThread::MaoscBufferCopied(TInt /*aError*/, const TDesC8& /*aBuffer*/)
	{
    TRACER_AUTO;
	delete iBuffer[0];
	iBuffer.Remove(0);
	
	const TInt KMicrosecondsInOneSecond(1000000);
	iShared.iTrack->SetPlaybackPosition( (iSavedPosition + iStream->Position().Int64()) / KMicrosecondsInOneSecond);
	
	if (iShared.iPaused)
		{
		iSavedPosition += iStream->Position().Int64();
		}
	
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
	else
		{
		//  There are still things in the buffer
		if (!iShared.iPaused)
			{
			iStream->WriteL(*iBuffer[0]);
			}
		}
	}

void CMobblerAudioThread::MaoscOpenComplete(TInt /*aError*/)
	{
//	TRACER_AUTO;
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
    TRACER_AUTO;
	}

TInt CMobblerAudioThread::UpdatePlayerPosition(TAny* aRef)
	{
    TRACER_AUTO;
	TTimeIntervalMicroSeconds playerPosition;
	TInt error(static_cast<CMobblerAudioThread*>(aRef)->iPlayer->GetPosition(playerPosition));
	if (error == KErrNone)
		{
		static_cast<CMobblerAudioThread*>(aRef)->iShared.iTrack->SetPlaybackPosition(playerPosition.Int64() / 1000000);
		}
	
	return KErrNone;
	}

void CMobblerAudioThread::MapcInitComplete(TInt aError, const TTimeIntervalMicroSeconds& aDuration)
	{
    TRACER_AUTO;
	if (aError == KErrNone)
		{
		iShared.iPlaying = ETrue;
		iShared.iDownloadComplete = ETrue;
		iShared.iTrack->SetTrackLength(aDuration.Int64() / 1000000);
		iPlayer->SetVolume(iShared.iAudioDataSettings.iVolume);
		iShared.iMaxVolume = iPlayer->MaxVolume();
		iPlayer->Play();
		
		iPeriodic = CPeriodic::NewL(CActive::EPriorityStandard);
		iCallBack = TCallBack(UpdatePlayerPosition, this);
		iPeriodic->Start(1000000, 1000000, iCallBack);
		}
	else
		{
		if (!iActiveSchedulerStopped)
			{
			iActiveSchedulerStopped = ETrue;
			CActiveScheduler::Stop();
			}
		}
	}

void CMobblerAudioThread::MapcPlayComplete(TInt /*aError*/)
	{
    TRACER_AUTO;
	if (!iActiveSchedulerStopped)
		{
		iActiveSchedulerStopped = ETrue;
		CActiveScheduler::Stop();
		}
	}

// End of file
