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

#include "mobblercontentlisting.h"
#include "mobblercontentlistingobserver.h"

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

CMobblerContentListing* CMobblerContentListing::NewL()
	{
	CMobblerContentListing* self(new(ELeave) CMobblerContentListing());
	CleanupStack::PushL(self);
	self->ConstructL();
	CleanupStack::Pop(self);
	return self;
	}
	
CMobblerContentListing::CMobblerContentListing()
	{
	}
	
void CMobblerContentListing::ConstructL()
	{
	// Create Content Listing Engine and a list model
	iClfEngine = ContentListingFactory::NewContentListingEngineLC();
	CleanupStack::Pop();	// iClfEngine
	iClfModel = iClfEngine->CreateListModelLC(*this);
	CleanupStack::Pop();	// iClfModel

	// Create an array for the desired media types
	RArray<TInt> array;
	CleanupClosePushL(array);
	array.AppendL(ECLFMediaTypeMusic);

	// Set wanted media types array to the model
	iClfModel->SetWantedMediaTypesL(array.Array());
	CleanupStack::PopAndDestroy(&array);

	iClfModelReady = EFalse;
	}

CMobblerContentListing::~CMobblerContentListing()
	{
	if(iClfModel)
		{
		iClfModel->CancelRefresh();
		delete iClfModel;
		}
	delete iClfEngine;
	}

void CMobblerContentListing::SetObserver(MMobblerContentListingObserver& aObserver)
	{
	iObserver = &aObserver;
	}

void CMobblerContentListing::FindAndSetAlbumNameL(const TDesC& aArtist, 
												  const TDesC& aTitle)
	{
	iArtist = aArtist;
	iTitle  = aTitle;
	FindAndSetAlbumNameL();
	}

void CMobblerContentListing::FindAndSetAlbumNameL()
	{
	if (!iClfModelReady)
		{
		iClfModel->RefreshL();
		}
	else if (iClfModelReady && iObserver && iArtist.Length() > 0)
		{
		TBool found(EFalse);
		
		// Is it worth post filtering these results with MCLFPostFilter? Probably not.
		TInt numberOfItems(iClfModel->ItemCount());
		for(TInt i(0); i < numberOfItems; ++i)
			{
			const MCLFItem& item(iClfModel->Item(i));
			TPtrC artist;
			TPtrC title;
			TPtrC album;
			TInt32 trackNumber;
			TPtrC path;
			TInt artistError(item.GetField(ECLFFieldIdArtist,  artist));
			TInt titleError(item.GetField(ECLFFieldIdSongName, title));
			TInt albumError(item.GetField(ECLFFieldIdAlbum,    album));
			TInt trackNumberError(item.GetField(ECLFFieldIdTrackNumber, 
												trackNumber));
			TInt pathError(item.GetField(ECLFFieldIdFileNameAndPath, path));

			// Only if title and artist tags were found
			if (artistError == KErrNone && titleError == KErrNone)
				{
				if ((artist.Compare(iArtist) == 0) &&
					(title.Compare(iTitle) == 0))
					{
					if (trackNumberError == KErrNone)
						{
						iObserver->SetTrackNumber(trackNumber);
						}
					
					if (pathError == KErrNone)
						{
						iObserver->SetPathL(path);
						}
					
					if (albumError == KErrNone)
						{
						iObserver->SetAlbumL(album);
						// This *could* be a mismatch (e.g. same artist-song 
						// combo but wrong album). Can we confirm with any 
						// other data? Alas, no duration in enum 
						// TCLFDefaultFieldId. Luckily Compare() is 
						// case-sensitive and that'll discriminate on the often
						// different "the"/"The", "In"/"in", "Of"/"of" etc.
						found = ETrue;
						}
					
					break;
					}
				}
			}
		
		if (!found)
			{
			// Always set the album, even if we don't know it
			// CMobblerTrack needs to know that we don't know it 
			iObserver->SetAlbumL(KNullDesC);
			}
		
		iArtist = KNullDesC;
		iTitle  = KNullDesC;
		}
	}

void CMobblerContentListing::HandleOperationEventL(
											TCLFOperationEvent aOperationEvent,
											TInt /*aError*/)
	{
	if (aOperationEvent == ECLFRefreshComplete)
		{
		// We can now look for the album of anything now playing
		iClfModelReady = ETrue;
		if (iArtist.Length() > 0)
			{
			FindAndSetAlbumNameL();
			}
		}
	else if (aOperationEvent == ECLFModelOutdated)
		{
		// Cannot look for any more albums until a refresh
		iClfModelReady = EFalse;
		}
	}

// End of file
