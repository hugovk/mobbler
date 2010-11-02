/*
mobblertrack.cpp

Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
Copyright (C) 2008, 2009, 2010  Michael Coffey
Copyright (C) 2008, 2009, 2010  Hugo van Kemenade
Copyright (C) 2009, 2010  gw111zz

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

#include <bautils.h>
#include <metadatafieldcontainer.h>
#include <metadatautility.h>
#include <sendomfragment.h>
#include <senxmlutils.h>

#include "mobblerappui.h"
#include "mobblerliterals.h"
#include "mobblerlogging.h"
#include "mobblerstring.h"
#include "mobblertracer.h"
#include "mobblertrack.h"
#include "mobblerutility.h"

const TPtrC KArtExtensionArray[] =
	{
	_L(".jpg"),
	_L(".gif"),
	_L(".png"),
	};

const TPtrC KArtFileArray[] =
	{
	_L("cover.jpg"),
	_L("cover.gif"),
	_L("cover.png"),
	_L("folder.jpg"),
	_L("folder.gif"),
	_L("folder.png"),
	};

_LIT(KArtistImageCache, "E:\\System\\Data\\Mobbler\\cache\\");

_LIT8(KExtraLarge, "extralarge");
_LIT8(KImages, "images");
_LIT8(KSizes, "sizes");
_LIT8(KTotal, "total");
_LIT8(KUserLoved, "userloved");

CMobblerTrack* CMobblerTrack::NewL(const TDesC8& aArtist,
									const TDesC8& aTitle,
									const TDesC8& aAlbum,
									const TDesC8& aMbTrackId,
									const TDesC8& aImage,
									const TDesC8& aMp3Location,
									TTimeIntervalSeconds aTrackLength,
									const TDesC8& aRadioAuth,
									TBool aLoved,
									TBool aGetInfo)
	{
    TRACER_AUTO;
	CMobblerTrack* self(new(ELeave) CMobblerTrack(aTrackLength, aLoved));
	CleanupStack::PushL(self);
	self->ConstructL(aArtist, aTitle, aAlbum, aMbTrackId, aImage, aMp3Location, aRadioAuth, aGetInfo);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTrack::CMobblerTrack(TTimeIntervalSeconds aTrackLength, TBool aLoved)
	: CMobblerTrackBase(aTrackLength, aLoved),
	iOnTour(EFalse)
	{
    TRACER_AUTO;
	Open();
	}

void CMobblerTrack::ConstructL(const TDesC8& aArtist,
		const TDesC8& aTitle,
		const TDesC8& aAlbum,
		const TDesC8& aMbTrackId,
		const TDesC8& aImage,
		const TDesC8& aMp3Location,
		const TDesC8& aRadioAuth,
		TBool aGetInfo)
	{
    TRACER_AUTO;
	BaseConstructL(aTitle, aArtist, aAlbum, aRadioAuth);
	
	iMbTrackId = CMobblerString::NewL(aMbTrackId);
	iMp3Location = aMp3Location.AllocL();
	iPlaylistImageLocation = aImage.AllocL();
	
	if (aGetInfo)
		{
		if (aRadioAuth.Length() == 0)
			{
			// This is a music player track
		
			if (static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().Mode() == CMobblerLastFmConnection::EOnline)
				{
				// when in online mode fetch the track details so we
				// can see if the track has been loved by this user
				delete iTrackInfoHelper;
				iTrackInfoHelper = CMobblerFlatDataObserverHelper::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection(), *this, EFalse);
				static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().GetInfoL(EMobblerCommandTrackGetInfo, Artist().String8(), KNullDesC8, Title().String8(), aMbTrackId, *iTrackInfoHelper);
				}
			}
	
		if (static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().Mode() == CMobblerLastFmConnection::EOnline)
			{
			delete iEventsInfoHelper;
			iEventsInfoHelper = CMobblerFlatDataObserverHelper::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection(), *this, EFalse);
			static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().QueryLastFmL(EMobblerCommandArtistEventSingle, Artist().String8(), KNullDesC8, KNullDesC8, KNullDesC8, *iEventsInfoHelper);
			}
		}
	}

CMobblerTrack::~CMobblerTrack()
	{
    TRACER_AUTO;
	delete iTrackInfoHelper;
	delete iAlbumInfoHelper;
	delete iArtistInfoHelper;
	delete iEventsInfoHelper;
	
	if (static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ContentListing())
		{
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ContentListing()->CancelFindLocalTrack(this);
		}
	
	delete iMbTrackId;
	delete iMp3Location;
	iImage->Close();
	delete iLocalFile;
	delete iPlaylistImageLocation;

	static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().CancelTransaction(this);
	}

void CMobblerTrack::Open()
	{
    TRACER_AUTO;
	++iRefCount;
	}

void CMobblerTrack::Release()
	{
    TRACER_AUTO;
	if (--iRefCount == 0)
		{
		delete this;
		}
	}
void CMobblerTrack::FindLocalTrackL()
	{
    TRACER_AUTO;
	if (static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ContentListing()
			&& Title().String().Length() != 0
			&& Artist().String().Length() != 0)
		{
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ContentListing()->FindLocalTrackL(Artist().String(), Title().String(), this);
		}
	}

void CMobblerTrack::SetDataSize(TInt aDataSize)
	{
//	TRACER_AUTO;
	iDataSize = aDataSize;
	}

TInt CMobblerTrack::DataSize() const
	{
    TRACER_AUTO;
	// if we don't know the data size then return 1
	return (iDataSize == KErrNotFound) || (iDataSize == 0) ? 1 : iDataSize;
	}

void CMobblerTrack::BufferAdded(TInt aBufferSize)
	{
    TRACER_AUTO;
	iBuffered += aBufferSize;
	}

TInt CMobblerTrack::Buffered() const
	{
    TRACER_AUTO;
	// if we don't know the data size then return 0
	return (iDataSize == KErrNotFound) ? 0 : iBuffered;
	}

void CMobblerTrack::FindBetterImageL()
	{
    TRACER_AUTO;
	if (iImageType == EMobblerImageTypeArtistLocal
			|| iImageType == EMobblerImageTypeNone)
		{
		// We currently have an artist image that we found locally
		// or we don't have an image at all
		// so try to find some album art online
		if (!DownloadAlbumImageL())
			{
			// We could not download an album image
		
			if (iImageType != EMobblerImageTypeArtistLocal)
				{
				// We haven't already got a local artist image
				// so try to fetch one online
				FetchArtistInfoL();
				}
			}
		}
	}

void CMobblerTrack::FindLocalAlbumImageL()
	{
    TRACER_AUTO;
	if (iLocalFile && iLocalFile->Length() > 0)
		{
		// This track was found on the phone!

#ifndef __WINS__
		// First try reading album art from the ID3 tag
		LOG(_L8("Searching for ID3"));
		
		CMetaDataUtility* metaDataUtility(CMetaDataUtility::NewL());
		CleanupStack::PushL(metaDataUtility);

		RArray<TMetaDataFieldId> wantedFields;
		CleanupClosePushL(wantedFields);
		wantedFields.AppendL(EMetaDataJpeg);

		metaDataUtility->OpenFileL(*iLocalFile, wantedFields);

		if (metaDataUtility->MetaDataCount() > 0)
			{
			LOG(_L8("Found ID3 tags"));
			const CMetaDataFieldContainer&  metaDataFieldContainer(metaDataUtility->MetaDataFieldsL());
			
			if (metaDataFieldContainer.Field(EMetaDataJpeg).Length() > 0)
				{
				LOG(_L8("Jpeg present"));
				LOG(_L8("Found album art ID3"));
				
				HBufC8* albumArtFromId3(HBufC8::NewLC(metaDataFieldContainer.Field(EMetaDataJpeg).Length()));
				albumArtFromId3->Des().Copy(metaDataFieldContainer.Field(EMetaDataJpeg));
                iImage = CMobblerBitmap::NewL(*this, *albumArtFromId3);
                CleanupStack::PopAndDestroy(albumArtFromId3);
                
                iImageType = EMobblerImageTypeAlbumLocal;
				}
			}

		CleanupStack::PopAndDestroy(2, metaDataUtility);
#endif

		// If not found, check for %album%.jpg/gif/png
		TFileName fileName;
		if (!iImage && Album().String().Length() > 0)
			{
			const TInt arraySize(sizeof(KArtExtensionArray) / sizeof(TPtrC));
			for (TInt i(0); i < arraySize; ++i)
				{
				fileName.Copy(LocalFilePath());
				fileName.Append(Album().SafeFsString(fileName.Length() + KArtExtensionArray[i].Length()));
				fileName.Append(KArtExtensionArray[i]);

				if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), fileName))
					{
					LOG(_L8("Found %album%.jpg/gif/png"));
					iImage = CMobblerBitmap::NewL(*this, fileName);
					iImageType = EMobblerImageTypeAlbumLocal;
					break;
					}
				}
			}

		// If not found, check for cover.jpg/gif/png, folder.jpg/gif/png
		const TInt arraySize(sizeof(KArtFileArray) / sizeof(TPtrC));
		for (TInt i(0); i < arraySize && !iImage ; ++i)
			{
			fileName.Copy(LocalFilePath());
			fileName.Append(KArtFileArray[i]);

			if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), fileName))
				{
				LOG(_L8("Found cover.jpg/gif/png, folder.jpg/gif/png"));
				iImage = CMobblerBitmap::NewL(*this, fileName);
				iImageType = EMobblerImageTypeAlbumLocal;
				break;
				}
			}
		}
	}

void CMobblerTrack::FindLocalArtistImageL()
	{
    TRACER_AUTO;
	LOG(_L8("CMobblerTrack::FindLocalArtistImageL"));
	TFileName fileName;
	
	// If still not found, check for %artist%.jpg/gif/png
	if (!iImage && Artist().String().Length() > 0)
		{
		const TInt arraySize(sizeof(KArtExtensionArray) / sizeof(TPtrC));
		for (TInt i(0); i < arraySize; ++i)
			{
			fileName.Copy(LocalFilePath());
			fileName.Append(Artist().SafeFsString(fileName.Length() +
											KArtExtensionArray[i].Length()));
			fileName.Append(KArtExtensionArray[i]);

			if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), fileName))
				{
				LOG(_L8("Found %artist%.jpg/gif/png"));
				iImage = CMobblerBitmap::NewL(*this, fileName);
				iImageType = EMobblerImageTypeArtistLocal;
				break;
				}
			}
		}

	// Next check the artist image cache
	if (!iImage && Artist().String().Length() > 0)
		{
		// Not found track's path, check for %artist%.jpg in the cache
		fileName.Copy(KArtistImageCache);
		fileName.Append(Artist().SafeFsString(fileName.Length() +
											  KArtExtensionArray[0].Length()));
		fileName.Append(KArtExtensionArray[0]);
		if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), fileName))
			{
			LOG(_L8("Found %artist%.jpg in cache"));
			LOG(fileName);
			iImage = CMobblerBitmap::NewL(*this, fileName);
			iImageType = EMobblerImageTypeArtistLocal;
			}
		}
	}

void CMobblerTrack::HandleFindLocalTrackCompleteL(TInt aTrackNumber, const TDesC& aAlbum, const TDesC& aLocalFile)
	{
    TRACER_AUTO;
	LOG(_L8("CMobblerTrack::HandleFindLocalTrackCompleteL"));
	LOG(aAlbum);
	LOG(aLocalFile);
	
	if (aLocalFile.Length() > 0)
		{
		delete iLocalFile;
		iLocalFile = aLocalFile.AllocL();
		}
	
	if (aAlbum.Length() > 0)
		{
		SetAlbumL(aAlbum);
		}
	
	if (aTrackNumber != KErrNotFound)
		{
		SetTrackNumber(aTrackNumber);
		}
	
	FindLocalAlbumImageL();
	
	if (!iImage)
		{
		// We didn't find the album art locally
	
		if (static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().Mode() == CMobblerLastFmConnection::EOnline
				&& static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().State() == CMobblerLastFmConnection::ENone)
			{
			// We're online so look try to find the album art online
			TBool downloading(DownloadAlbumImageL());
			
			if (!downloading)
				{
				// We were unable to start downloading an album image
				// so see if we have local artist art
				FindLocalArtistImageL();
				
				if (!iImage)
					{
					// We were unable to find a local artist image
					// so start looking for one online
					FetchArtistInfoL();
					}
				}
			}
		else
			{
			// We're in offline mode so just look for the artist image locally
			FindLocalArtistImageL();
			}
		}
	}

const TDesC8& CMobblerTrack::Mp3Location() const
	{
    TRACER_AUTO;
	return *iMp3Location;
	}

void CMobblerTrack::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
    TRACER_AUTO;
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
#ifdef __SYMBIAN_SIGNED__
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->SetAlbumArtAsWallpaper(ETrue);
#endif
	}

void CMobblerTrack::BitmapResizedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
    TRACER_AUTO;
	}

const TDesC& CMobblerTrack::LocalFile() const
	{
    TRACER_AUTO;
	if (iLocalFile)
		{
		return *iLocalFile;
		}
	
	return KNullDesC;
	}

TPtrC CMobblerTrack::LocalFilePath() const
	{
    TRACER_AUTO;
	if (iLocalFile)
		{
		TParse parse;
		parse.Set(*iLocalFile, NULL, NULL);
		return parse.DriveAndPath();
		}
	
	return TPtrC(KNullDesC);
	}

const CMobblerBitmap* CMobblerTrack::Image() const
	{
    TRACER_AUTO;
	return iImage;
	}

const CMobblerString& CMobblerTrack::MbTrackId() const
	{
    TRACER_AUTO;
	return *iMbTrackId;
	}

void CMobblerTrack::DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, TInt aTransactionError)
	{
    TRACER_AUTO;
	if (aObserver == iTrackInfoHelper)
		{
		if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
			{
			// find out if the track has been loved
			// Parse the XML
			CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
			CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aData));
			
			CSenElement* userLovedElement(domFragment->AsElement().Element(KTrack)->Element(KUserLoved));
			
			if (userLovedElement)
				{
				iLove = userLovedElement->Content().Compare(K0) != 0 ? ELoved : ENoLove;
				}
			
			CleanupStack::PopAndDestroy(2);
			}
		
		}
	else if (aObserver == iEventsInfoHelper)
		{
		if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
			{
			DUMPDATA(aData, _L("artistgetevents.xml"));
			
			// find out if the track has been loved
			// Parse the XML
			CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
			CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aData));
			
			CSenElement* eventsElement(domFragment->AsElement().Element(KEvents));
			if (eventsElement && eventsElement->AttrValue(KTotal))
				{
				if (eventsElement->AttrValue(KTotal)->Compare(K0))
					{
					iOnTour = ETrue;
					LOG(_L8("On tour"));
					}
				}
			
			CleanupStack::PopAndDestroy(2);
			}
		}
	else if (aObserver == iAlbumInfoHelper)
		{
		LOG(_L8("5 DataL album info fetched"));
		DUMPDATA(aData, _L("albumgetinfo.xml"));

		if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
			{
			LOG(_L8("6 FetchImageL(album)"));

			if (!FetchImageL(aObserver, aData))
				{
				// We failed to get the album image from the album info
			
				if (iPlaylistImageLocation->Length() > 0)
					{
					// Try to fetch the album image from the playlist
					LOG(_L8("7 FetchImageL(album)"));
					FetchImageL(EMobblerImageTypeAlbumRemote, *iPlaylistImageLocation);
					}
				else if (iImageType != EMobblerImageTypeArtistRemote
						&& iImageType != EMobblerImageTypeArtistLocal)
					{
					// Couldn't fetch album image and we haven't already got an artist image
					
					// check to see if we can find one locally
					FindLocalArtistImageL();
					
					if (!iImage)
						{
						// we still can't find an image so look for it online
						LOG(_L8("8 FetchArtistInfoL()"));
						FetchArtistInfoL();
						}
					}
				}
			}
		else
			{
			// There was a transaction error while fetching the album info
			FindLocalArtistImageL();
			}
		}
	else if (aObserver == iArtistInfoHelper)
		{
		LOG(_L8("11 DataL artist info fetched"));
		DUMPDATA(aData, _L("artistgetimage.xml"));

		if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
			{
			FetchImageL(aObserver, aData);
			}
		}
	else
		{
		// check if it was the base class observer
		CMobblerTrackBase::DataL(aObserver, aData, aTransactionError);
		}
	}

void CMobblerTrack::DataL(const TDesC8& aData, TInt aTransactionError)
	{
    TRACER_AUTO;
	if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
		{
		LOG(_L8("9 DataL album art fetched"));
		SaveAlbumArtL(aData);
		}
	else
		{
		LOG(_L8("There was a transaction error while fetching the album or artist image"));
		
		// In this case we may not have looked for a local artist image 
		FindLocalArtistImageL();
		}
	}

TBool CMobblerTrack::OkToDownloadAlbumArt() const
	{
    TRACER_AUTO;
	TInt downloadAlbumArt(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->DownloadAlbumArt());

	TBool okToDownloadAlbumArt((downloadAlbumArt == CMobblerAppUi::EOnlyRadio && !IsMusicPlayerTrack())
								|| ((downloadAlbumArt == CMobblerAppUi::EAlwaysAndKeep) ||
									(downloadAlbumArt == CMobblerAppUi::EAlwaysAndDitch)));

	return (okToDownloadAlbumArt &&
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().Mode() == CMobblerLastFmConnection::EOnline);
	}

void CMobblerTrack::FetchAlbumInfoL()
	{
    TRACER_AUTO;
	if (OkToDownloadAlbumArt())
		{
		delete iAlbumInfoHelper;
		iAlbumInfoHelper = CMobblerFlatDataObserverHelper::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection(), *this, EFalse);
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().GetInfoL(EMobblerCommandAlbumGetInfo, Artist().String8(), Album().String8(), KNullDesC8, KNullDesC8, *iAlbumInfoHelper);
		}
	}

void CMobblerTrack::FetchArtistInfoL()
	{
    TRACER_AUTO;
	if (OkToDownloadAlbumArt())
		{
		delete iArtistInfoHelper;
		iArtistInfoHelper = CMobblerFlatDataObserverHelper::NewL(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection(), *this, EFalse);
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().ArtistGetImageL(Artist().String8(), *iArtistInfoHelper);
		}
	}

void CMobblerTrack::FetchImageL(TMobblerImageType aImageType, const TDesC8& aImageLocation)
	{
    TRACER_AUTO;
	if (OkToDownloadAlbumArt())
		{
		iImageType = aImageType;
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().RequestImageL(this, aImageLocation);
		}
	}

TBool CMobblerTrack::FetchImageL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData)
	{
    TRACER_AUTO;
	TBool found(EFalse);

	// Parse the XML
	CSenXmlReader* xmlReader(CSenXmlReader::NewLC());
	CSenDomFragment* domFragment(MobblerUtility::PrepareDomFragmentLC(*xmlReader, aData));

	RPointerArray<CSenElement> imageArray;
	CleanupClosePushL(imageArray);

	if (aObserver == iAlbumInfoHelper)
		{
		User::LeaveIfError(domFragment->AsElement().Element(KAlbum)->ElementsL(imageArray, KImage));
		}
	else
		{
		CSenElement* element(domFragment->AsElement().Element(KImages));

		if (element)
			{
			element = element->Element(KImage);

			if (element)
				{
				element = element->Element(KSizes);

				if (element)
					{
					User::LeaveIfError(element->ElementsL(imageArray, KSize));
					}
				}
			}
		}

	const TInt KImageCount(imageArray.Count());
	for (TInt i(KImageCount-1); i >= 0; --i)
		{
		if ((aObserver == iAlbumInfoHelper &&
			 imageArray[i]->AttrValue(KSize)->Compare(KExtraLarge) == 0)
				||
			(aObserver == iAlbumInfoHelper &&
			 imageArray[i]->AttrValue(KSize)->Compare(KLarge) == 0)
				||
			(aObserver == iArtistInfoHelper &&
			 imageArray[i]->AttrValue(KName)->Compare(KExtraLarge) == 0)
				||
			(aObserver == iArtistInfoHelper &&
			 imageArray[i]->AttrValue(KName)->Compare(KLarge) == 0))
			{
			if (imageArray[i]->Content().Length() > 0)
				{
				found = ETrue;
				
				if (aObserver == iAlbumInfoHelper)
					{
					LOG(_L8("15 FetchImageL(album)"));
					FetchImageL(EMobblerImageTypeAlbumRemote, imageArray[i]->Content());
					}
				else
					{
					LOG(_L8("16 FetchImageL(artist)"));
					FetchImageL(EMobblerImageTypeArtistRemote, imageArray[i]->Content());
					}
				break;
				}
			}
		}

	CleanupStack::PopAndDestroy(&imageArray);
	CleanupStack::PopAndDestroy(domFragment);
	CleanupStack::PopAndDestroy(xmlReader);

	return found;
	}

void CMobblerTrack::SaveAlbumArtL(const TDesC8& aData)
	{
    TRACER_AUTO;
	if (((iLocalFile && iLocalFile->Length() > 0) ||
		(iImageType == EMobblerImageTypeArtistRemote && IsMusicPlayerTrack())) &&
		(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->DownloadAlbumArt() ==
				CMobblerAppUi::EAlwaysAndKeep)
		)
		{
		// Try to save the album art in the album folder
		LOG(_L8("SaveAlbumArtL() saving album art"));

		TFileName albumArtFileName;
		if (iLocalFile && iLocalFile->Length() > 0)
			{
			albumArtFileName.Append(LocalFilePath());
			}
		else
			{
			albumArtFileName.Append(KArtistImageCache());
			CCoeEnv::Static()->FsSession().MkDirAll(albumArtFileName);
			}

		TInt knownPathLength(albumArtFileName.Length() +
							 KArtExtensionArray[0].Length());
		if (iImageType == EMobblerImageTypeAlbumRemote)
			{
			albumArtFileName.Append(Album().SafeFsString(knownPathLength));
			}
		else
			{
			albumArtFileName.Append(Artist().SafeFsString(knownPathLength));
			}
		albumArtFileName.Append(KArtExtensionArray[0]);
		LOG(albumArtFileName);

		RFile albumArtFile;
		CleanupClosePushL(albumArtFile);
		TInt createError(albumArtFile.Create(CCoeEnv::Static()->FsSession(), albumArtFileName, EFileWrite));

		if (createError == KErrNone)
			{
			TInt writeError(albumArtFile.Write(aData));
			if (writeError != KErrNone)
				{
				albumArtFile.Close();
				CCoeEnv::Static()->FsSession().Delete(albumArtFileName);
				}
			}

		CleanupStack::PopAndDestroy(&albumArtFile);
		}

	if (iImage)
		{
		iImage->Close();
		}
	
	iImage = CMobblerBitmap::NewL(*this, aData);
	}

TBool CMobblerTrack::DownloadAlbumImageL()
	{
    TRACER_AUTO;
	TBool fetching(EFalse);
	
	if (Album().String().Length() != 0)
		{
		// There is an album name!

		// Try to fetch the album info. Once this is fetched
		// we will try to fetch the album art in the callback.
		LOG(_L8("2 FetchAlbumInfoL()"));
		FetchAlbumInfoL();
		fetching = ETrue;
		}
	else if (iPlaylistImageLocation->Length() != 0)
		{
		// We don't know the album name, but there was album art in the playlist
		LOG(_L8("3 FetchImageL(album)"));
		FetchImageL(EMobblerImageTypeAlbumRemote, *iPlaylistImageLocation);
		fetching = ETrue;
		}
	
	return fetching;
	}

// End of file
