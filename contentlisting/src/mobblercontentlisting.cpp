/*
mobblercontentlisting.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2009  Michael Coffey

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

#include <ContentListingFactory.h>
#include <ecom/implementationproxy.h>
#include <MCLFContentListingEngine.h>
#include <MCLFItem.h>
#include <MCLFItemListModel.h>
#include <MCLFSortingStyle.h>

#include "mobblercontentlisting.h"

#ifdef __SYMBIAN_SIGNED__
const TInt KImplementationUid = {0x2002661E};
#else
const TInt KImplementationUid = {0xA000BEB3};
#endif

const TImplementationProxy ImplementationTable[] =
	{
	{KImplementationUid, TProxyNewLPtr(CMobblerContentListing::NewL)}
	};

EXPORT_C const TImplementationProxy* ImplementationGroupProxy(TInt& aTableCount)
	{
	aTableCount = sizeof(ImplementationTable) / sizeof(TImplementationProxy);
	return ImplementationTable;
	}

CMobblerContentListing::CMobblerClfItem* CMobblerContentListing::CMobblerClfItem::NewLC(const TDesC& aTitle, const TDesC& aAlbum, const TDesC& aArtist, const TDesC& aLocalFile)
	{
	CMobblerClfItem* self(new(ELeave) CMobblerClfItem);
	CleanupStack::PushL(self);
	self->ConstructL(aTitle, aAlbum, aArtist, aLocalFile);
	
	return self;
	}

CMobblerContentListing::CMobblerClfItem::CMobblerClfItem()
	{	
	}

void CMobblerContentListing::CMobblerClfItem::ConstructL(const TDesC& aTitle, const TDesC& aAlbum, const TDesC& aArtist, const TDesC& aLocalFile)
	{
	iTitle = aTitle.AllocL();
	iAlbum = aAlbum.AllocL();
	iArtist = aArtist.AllocL();
	iLocalFile = aLocalFile.AllocL();
	}

CMobblerContentListing::CMobblerClfItem::~CMobblerClfItem()
	{
	delete iTitle;
	delete iAlbum;
	delete iArtist;
	delete iLocalFile;
	}	

TInt CMobblerContentListing::CMobblerClfItem::CompareClfItem(const CMobblerClfItem& aLeft, const CMobblerClfItem& aRight)
	{
	TInt result(aLeft.iTitle->Compare(*aRight.iTitle));
	
	if (result == 0)
		{
		result = aLeft.iArtist->Compare(*aRight.iArtist);
		}
	
	return result;
	}

void DoContentListingRefreshL(CMobblerContentListing::TSharedData* aSharedData)
	{
	TLinearOrder<CMobblerContentListing::CMobblerClfItem> order(CMobblerContentListing::CMobblerClfItem::CompareClfItem);
	
	do
		{
		// Create Content Listing Engine and a list model
		MCLFContentListingEngine* clfEngine = ContentListingFactory::NewContentListingEngineLC();
		MCLFItemListModel* clfModel = clfEngine->CreateListModelLC(*aSharedData->iObserver);
		
		// Create an array for the desired media types
		RArray<TInt> array;
		CleanupClosePushL(array);
		array.AppendL(ECLFMediaTypeMusic);
		
		// Set wanted media types array to the model
		clfModel->SetWantedMediaTypesL(array.Array());
		CleanupStack::PopAndDestroy(&array);
	
		clfModel->RefreshL();
		CActiveScheduler::Start();
		
		// The clf has refreshed so get all the data out of it and sort it
		const TInt KClfItemCount(clfModel->ItemCount());
		for (TInt i(0) ; i < KClfItemCount ; ++i)
			{
			const MCLFItem& clfItem(clfModel->Item(i));
			
			TPtrC title(KNullDesC);
			TPtrC album(KNullDesC);
			TPtrC artist(KNullDesC);
			TPtrC localFile(KNullDesC);
			
			clfItem.GetField(ECLFFieldIdSongName, title);
			clfItem.GetField(ECLFFieldIdAlbum, album);
			clfItem.GetField(ECLFFieldIdArtist, artist);
			clfItem.GetField(ECLFFieldIdFileNameAndPath, localFile);
			
			CMobblerContentListing::CMobblerClfItem* item(CMobblerContentListing::CMobblerClfItem::NewLC(title, album, artist, localFile));
			clfItem.GetField(ECLFFieldIdTrackNumber, item->iTrackNumber);
			aSharedData->iClfItems.InsertInOrder(item, order);
			CleanupStack::Pop(item);
			}
			
		CleanupStack::PopAndDestroy(2);
		
		RThread().Rendezvous(KErrNone);
		RThread().Suspend();
		
		aSharedData->iClfItems.ResetAndDestroy();
		}
		while (aSharedData->iState == CMobblerContentListing::EMobblerClfModelRefreshing);
	}

TInt ThreadFunction(TAny* aRef)
	{
	CTrapCleanup* cleanupStack(CTrapCleanup::New());
	
	CActiveScheduler* activeScheduler(new CActiveScheduler);
	CActiveScheduler::Install(activeScheduler);
	
	TRAPD(error, DoContentListingRefreshL(static_cast< CMobblerContentListing::TSharedData* >(aRef)));

	delete activeScheduler;
	delete cleanupStack;
	
	return error;
	}

CMobblerContentListing* CMobblerContentListing::NewL()
	{
	CMobblerContentListing* self(new(ELeave) CMobblerContentListing());
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
CMobblerContentListing::CMobblerContentListing()
	:CMobblerContentListingInterface(CActive::EPriorityStandard)
	{
	CActiveScheduler::Add(this);
	}
	
void CMobblerContentListing::ConstructL()
	{
	RefreshL();
	}

void CMobblerContentListing::RefreshL()
	{
	if (!iThreadCreated)
		{
		iThreadCreated = ETrue;
		User::LeaveIfError(iThread.Create(_L("Mobbler CLF"), ThreadFunction, KDefaultStackSize, NULL, &iSharedData));
		}

	iSharedData.iState = EMobblerClfModelRefreshing;
	iSharedData.iObserver = this;
	iThread.Rendezvous(iStatus);
	SetActive();
	iThread.Resume();
	}

void CMobblerContentListing::RunL()
	{
	if (iStatus.Int() == KErrNone)
		{
		if (iSharedData.iState == EMobblerClfModelReady)
			{
			if (iOperations.Count() > 0)
				{
				DoFindLocalTrackL();
				}
			}
		else if (iSharedData.iState == EMobblerClfModelError)
			{
			// there was an error refreshing the list so just complete
			// all the requests as if we didn't find anything
	
			for (TInt i(iOperations.Count() - 1) ; i >= 0 ; --i)
				{
				iOperations[i]->iObserver->HandleFindLocalTrackCompleteL(KErrNotFound, KNullDesC, KNullDesC);
				
				delete iOperations[i];
				iOperations.Remove(i);
				}
			}
		}
	else
		{
		iSharedData.iState = EMobblerClfModelError;
		}
	}

void CMobblerContentListing::DoCancel()
	{
	iThread.RendezvousCancel(iStatus);
	}

CMobblerContentListing::~CMobblerContentListing()
	{
	Cancel();
	
	iSharedData.iState = EMobblerClfModelClosing;
	iThread.Logon(iStatus);
	iThread.Resume();
	User::WaitForRequest(iStatus);
	
	iOperations.ResetAndDestroy();
	}

void CMobblerContentListing::FindLocalTrackL(const TDesC& aArtist, const TDesC& aTitle, MMobblerContentListingObserver* aObserver)
	{
	CMobblerContentListing::CMobblerClfItem* operation(CMobblerContentListing::CMobblerClfItem::NewLC(aTitle, KNullDesC, aArtist, KNullDesC));
	operation->iObserver = aObserver;
	iOperations.AppendL(operation);
	CleanupStack::Pop(operation);
	
	DoFindLocalTrackL();
	}

void CMobblerContentListing::CancelFindLocalTrack(MMobblerContentListingObserver* aObserver)
	{
	const TInt KOperationCount(iOperations.Count());
	for (TInt i(0) ; i < KOperationCount ; ++i)
		{
		if (iOperations[i]->iObserver == aObserver)
			{
			delete iOperations[i];
			iOperations.Remove(i);
			break;
			}
		}
	}

void CMobblerContentListing::DoFindLocalTrackL()
	{
	if (iSharedData.iState == EMobblerClfModelOutdated)
		{
		RefreshL();
		}
	else if (iSharedData.iState == EMobblerClfModelError)
		{
		// there was an error when we refreshed so just tell them that we couldn't find it
		iOperations[0]->iObserver->HandleFindLocalTrackCompleteL(KErrNotFound, KNullDesC, KNullDesC);
		}
	else if (iSharedData.iState == EMobblerClfModelReady)
		{
		// the list is sorted by track title and the by artist name
		// so perform a binary search for 
	
		TLinearOrder<CMobblerContentListing::CMobblerClfItem> order(CMobblerContentListing::CMobblerClfItem::CompareClfItem);
		TInt position(KErrNotFound);
				
		const TInt KOperationCount(iOperations.Count());
		
		for (TInt i(0) ; i < KOperationCount ; ++i)
			{
			position = iSharedData.iClfItems.FindInOrder(iOperations[0], order);
			
			if (position != KErrNotFound)
				{
				iOperations[0]->iObserver->HandleFindLocalTrackCompleteL(iSharedData.iClfItems[position]->iTrackNumber,
						*iSharedData.iClfItems[position]->iAlbum,
						*iSharedData.iClfItems[position]->iLocalFile);
				}
			else
				{
				// Always set the album, even if we don't know it
				// CMobblerTrack needs to know that we don't know it 
				iOperations[0]->iObserver->HandleFindLocalTrackCompleteL(KErrNotFound, KNullDesC, KNullDesC);
				}
			
			delete iOperations[0];
			iOperations.Remove(0);
			}
		}
	}

void CMobblerContentListing::HandleOperationEventL(TCLFOperationEvent aOperationEvent, TInt aError)
	{
	if (aOperationEvent == ECLFRefreshComplete)
		{
		// this sould only be called by the other thread
		CActiveScheduler::Stop();
	
		if (aError == KErrNone)
			{
			// We can now look for the album of anything now playing
			iSharedData.iState = EMobblerClfModelReady;
			}
		else
			{
			// there was an error refreshing the list so just complete
			// all the requests as if we didn't find anything
			
			iSharedData.iState = EMobblerClfModelError;
			}
		}
	else if (aOperationEvent == ECLFModelOutdated)
		{
		// Cannot look for any more albums until a refresh
		iSharedData.iState = EMobblerClfModelOutdated;
		}
	}

// End of file
