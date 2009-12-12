/*
mobblertrack.cpp

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

#include <bautils.h>
#include <metadatafieldcontainer.h>
#include <metadatautility.h>
#include <sendomfragment.h>
#include <senxmlutils.h>

#include "mobblerappui.h"
#include "mobblerliterals.h"
#include "mobblerlogging.h"
#include "mobblerstring.h"
#include "mobblertrack.h"

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

_LIT8(KElementExtraLarge, "extralarge");
_LIT8(KElementImages, "images");
_LIT8(KElementLarge, "large");
_LIT8(KElementSize, "size");
_LIT8(KElementSizes, "sizes");

CMobblerTrack* CMobblerTrack::NewL(const TDesC8& aArtist,
									const TDesC8& aTitle,
									const TDesC8& aAlbum,
									const TDesC8& aMbTrackId,
									const TDesC8& aImage,
									const TDesC8& aMp3Location,
									TTimeIntervalSeconds aTrackLength,
									const TDesC8& aRadioAuth)
	{
	CMobblerTrack* self(new(ELeave) CMobblerTrack(aTrackLength));
	CleanupStack::PushL(self);
	self->ConstructL(aArtist, aTitle, aAlbum, aMbTrackId, aImage, aMp3Location, aRadioAuth);
	CleanupStack::Pop(self);
	return self;
	}

CMobblerTrack::CMobblerTrack(TTimeIntervalSeconds aTrackLength)
	: CMobblerTrackBase(aTrackLength),
	  iTriedButCouldntFindAlbumArt(EFalse),
	  iUsingArtistImage(EFalse)
	{
	Open();
	}

void CMobblerTrack::ConstructL(const TDesC8& aArtist,
		const TDesC8& aTitle,
		const TDesC8& aAlbum,
		const TDesC8& aMbTrackId,
		const TDesC8& aImage,
		const TDesC8& aMp3Location,
		const TDesC8& aRadioAuth)
	{
	BaseConstructL(aTitle, aArtist, aAlbum, aRadioAuth);

	iMbTrackId = CMobblerString::NewL(aMbTrackId);
	iMp3Location = aMp3Location.AllocL();
	iImage = aImage.AllocL();

	if (!iAlbumArt && Album().String().Length() != 0)
		{
		// fetch the album info
		LOG(_L8("0 FetchAlbumInfoL()"));
		FetchAlbumInfoL();
		}
	}

CMobblerTrack::~CMobblerTrack()
	{
	delete iMbTrackId;
	delete iMp3Location;
	iAlbumArt->Close();
	delete iPath;
	delete iImage;

	static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().CancelTransaction(this);
	}

void CMobblerTrack::Open()
	{
	++iRefCount;
	}

void CMobblerTrack::Release()
	{
	if (--iRefCount == 0)
		{
		delete this;
		}
	}

void CMobblerTrack::SetDataSize(TInt aDataSize)
	{
	iDataSize = aDataSize;
	}

TInt CMobblerTrack::DataSize() const
	{
	// if we don't know the data size then return 1
	return (iDataSize == KErrNotFound) || (iDataSize == 0) ? 1 : iDataSize;
	}

void CMobblerTrack::BufferAdded(TInt aBufferSize)
	{
	iBuffered += aBufferSize;
	}

TInt CMobblerTrack::Buffered() const
	{
	// if we don't know the data size then return 0
	return (iDataSize == KErrNotFound) ? 0 : iBuffered;
	}

void CMobblerTrack::SetAlbumL(const TDesC& aAlbum)
	{
	LOG(_L8("CMobblerTrack::SetAlbumL"));
	LOG(aAlbum);
	
	SetAlbumBaseL(aAlbum);

	TFileName fileName;
	TBool found(EFalse);

	// First check for %album%.jpg/gif/png
	if ((!iAlbumArt || iUsingArtistImage)
		&& iPath && Album().String().Length() > 0)
		{
		const TInt arraySize(sizeof(KArtExtensionArray) / sizeof(TPtrC));
		for (TInt i(0); i < arraySize; ++i)
			{
			fileName.Copy(*iPath);
			fileName.Append(Album().SafeFsString(fileName.Length() +
							KArtExtensionArray[i].Length()));
			fileName.Append(KArtExtensionArray[i]);

			if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), fileName))
				{
				LOG(_L8("Found %album%.jpg/gif/png"));
				found = ETrue;
				iUsingArtistImage = EFalse;
				break;
				}
			}
		}

	// Next check the artist image cache
	if (!found && !iAlbumArt && Artist().String().Length() > 0)
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
			found = ETrue;
			iUsingArtistImage = ETrue;
			}
		}

	if (found)
		{
		LOG(fileName);
		iAlbumArt->Close();
		iAlbumArt = CMobblerBitmap::NewL(*this, fileName);
		}

	// Check if there's something better online
	DownloadAlbumArtL();
	}

const TDesC8& CMobblerTrack::Mp3Location() const
	{
	return *iMp3Location;
	}

void CMobblerTrack::BitmapLoadedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->StatusDrawDeferred();
#ifdef __SYMBIAN_SIGNED__
	static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->SetAlbumArtAsWallpaper(ETrue);
#endif
	}

void CMobblerTrack::BitmapResizedL(const CMobblerBitmap* /*aMobblerBitmap*/)
	{
	}

void CMobblerTrack::SetPathL(const TDesC& aPath)
	{
	LOG(_L8("CMobblerTrack::SetPathL"));
	LOG(aPath);

	TParse parse;
	parse.Set(aPath, NULL, NULL);
	TFileName fileName;
	TBool found(EFalse);

	iPath = parse.DriveAndPath().AllocL();
	
	HBufC8* albumArtFromId3(NULL);

#ifndef __WINS__
	// First try reading album art from the ID3 tag
	LOG(_L8("Searching for ID3"));
	
	CMetaDataUtility* metaDataUtility(CMetaDataUtility::NewL());
	CleanupStack::PushL(metaDataUtility);

	RArray<TMetaDataFieldId> wantedFields;
	CleanupClosePushL(wantedFields);
	wantedFields.AppendL(EMetaDataJpeg);

	metaDataUtility->OpenFileL(aPath, wantedFields);

	if (metaDataUtility->MetaDataCount() > 0)
		{
		LOG(_L8("Found ID3 tags"));
		const CMetaDataFieldContainer&  metaDataFieldContainer(metaDataUtility->MetaDataFieldsL());
		albumArtFromId3 = HBufC8::NewL(metaDataFieldContainer.Field(EMetaDataJpeg).Length());
		albumArtFromId3->Des().Copy(metaDataFieldContainer.Field(EMetaDataJpeg));
		if (albumArtFromId3->Length() > 0)
			{
			LOG(_L8("Jpeg present"));
			found = ETrue;
			}
		}

	CleanupStack::PopAndDestroy(2, metaDataUtility);
#endif

	// First check for %album%.jpg/gif/png
	if (Album().String().Length() > 0 && !found)
		{
		const TInt arraySize(sizeof(KArtExtensionArray) / sizeof(TPtrC));
		for (TInt i(0); i < arraySize; ++i)
			{
			fileName.Copy(parse.DriveAndPath());
			fileName.Append(Album().SafeFsString(fileName.Length() +
							KArtExtensionArray[i].Length()));
			fileName.Append(KArtExtensionArray[i]);

			if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), fileName))
				{
				LOG(_L8("Found %album%.jpg/gif/png"));
				found = ETrue;
				break;
				}
			}
		}

	// If not found, check for cover.jpg/gif/png, folder.jpg/gif/png
	const TInt arraySize(sizeof(KArtFileArray) / sizeof(TPtrC));
	for (TInt i(0); i < arraySize && !found; ++i)
		{
		fileName.Copy(parse.DriveAndPath());
		fileName.Append(KArtFileArray[i]);

		if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), fileName))
			{
			LOG(_L8("Found cover.jpg/gif/png, folder.jpg/gif/png"));
			found = ETrue;
			break;
			}
		}

	// If still not found, check for %artist%.jpg/gif/png
	if (!found && Artist().String().Length() > 0)
		{
		const TInt arraySize(sizeof(KArtExtensionArray) / sizeof(TPtrC));
		for (TInt i(0); i < arraySize; ++i)
			{
			fileName.Copy(parse.DriveAndPath());
			fileName.Append(Artist().SafeFsString(fileName.Length() +
											KArtExtensionArray[i].Length()));
			fileName.Append(KArtExtensionArray[i]);

			if (BaflUtils::FileExists(CCoeEnv::Static()->FsSession(), fileName))
				{
				LOG(_L8("Found %artist%.jpg/gif/png"));
				found = ETrue;
				iUsingArtistImage = ETrue;
				break;
				}
			}
		}

	if (found)
		{
		if (albumArtFromId3 && albumArtFromId3->Length() > 0) // Found album art in ID3 tag
			{
			LOG(_L8("Found album art ID3"));
			iAlbumArt->Close();
			CleanupStack::PushL(albumArtFromId3);
			iAlbumArt = CMobblerBitmap::NewL(*this, *albumArtFromId3);
			CleanupStack::PopAndDestroy(albumArtFromId3);
			}
		else
			{
			LOG(fileName);
			iAlbumArt->Close();
			iAlbumArt = CMobblerBitmap::NewL(*this, fileName);
			}
		}
	}

const CMobblerBitmap* CMobblerTrack::AlbumArt() const
	{
	return iAlbumArt;
	}

const CMobblerString& CMobblerTrack::MbTrackId() const
	{
	return *iMbTrackId;
	}

void CMobblerTrack::DataL(const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError)
	{
	switch (iState)
		{
		case EFetchingAlbumInfo:
			{
			LOG(_L8("5 DataL album info fetched"));
			DUMPDATA(aData, _L("albumgetinfo.xml"));

			TBool found(EFalse);
			if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
				{
				LOG(_L8("6 FetchImageL(album)"));
				found = FetchImageL(aData);
				}

			if (!found)
				{
				// we failed to fetch the album details
				// so try to fetch the album art from the playlist
				if (iImage->Length() > 0)
					{
					LOG(_L8("7 FetchImageL(album)"));
					FetchImageL(EFetchingAlbumArt, *iImage);
					}
				else if (!iUsingArtistImage)
					{
					// Couldn't fetch album art, try artist image instead
					iTriedButCouldntFindAlbumArt = ETrue;
					LOG(_L8("8 FetchArtistInfoL()"));
					FetchArtistInfoL();
					}
				}
			break;
			}

		case EFetchingAlbumArt:
			{
			LOG(_L8("9 DataL album art fetched"));
			if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
				{
				SaveAlbumArtL(aData);
				}
			else
				{
				// Couldn't fetch album art, try artist image instead
				LOG(_L8("10 FetchArtistInfoL()"));
				iTriedButCouldntFindAlbumArt = ETrue;
				FetchArtistInfoL();
				}
			break;
			}

		case EFetchingArtistInfo:
			{
			LOG(_L8("11 DataL artist info fetched"));
			DUMPDATA(aData, _L("artistgetimage.xml"));

			if (iAlbumArt)
				{
				iState = ENone;
				}
			else if (!iTriedButCouldntFindAlbumArt && Album().String().Length() != 0)
				{
				// Just got artist info but there's now an album name, give up and try album art
				LOG(_L8("12 FetchAlbumInfoL()"));
				FetchAlbumInfoL();
				}
			else if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
				{
				FetchImageL(aData);
				}
			break;
			}

		case EFetchingArtistImage:
			{
			LOG(_L8("13 DataL artist image fetched"));
			if (iAlbumArt)
				{
				iState = ENone;
				}
			else if (aTransactionError == CMobblerLastFmConnection::ETransactionErrorNone)
				{
				SaveAlbumArtL(aData);

				if (!iTriedButCouldntFindAlbumArt &&
					Album().String().Length() != 0)
					{
					// Just got artist art but there's now an album name, try album image
					LOG(_L8("14 FetchAlbumInfoL()"));
					FetchAlbumInfoL();
					}
				}
			break;
			}

		default:
			break;
		}
	}

TBool CMobblerTrack::OkToDownloadAlbumArt() const
	{
	TInt downloadAlbumArt(static_cast<CMobblerAppUi*>(CEikonEnv::Static()->AppUi())->DownloadAlbumArt());

	TBool okToDownloadAlbumArt((downloadAlbumArt == CMobblerAppUi::EOnlyRadio && !IsMusicPlayerTrack())
								|| (downloadAlbumArt == CMobblerAppUi::EAlwaysWhenOnline));

	return (okToDownloadAlbumArt &&
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().Mode() == CMobblerLastFmConnection::EOnline);
	}

void CMobblerTrack::FetchAlbumInfoL()
	{
	if (OkToDownloadAlbumArt())
		{
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().AlbumGetInfoL(Album().String8(), Artist().String8(), *this);
		iState = EFetchingAlbumInfo;
		}
	}

void CMobblerTrack::FetchArtistInfoL()
	{
	if (OkToDownloadAlbumArt())
		{
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().ArtistGetImageL(Artist().String8(), *this);
		iState = EFetchingArtistInfo;
		}
	}

void CMobblerTrack::FetchImageL(TState aState, const TDesC8& aImageLocation)
	{
	if (OkToDownloadAlbumArt())
		{
		static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->LastFmConnection().RequestImageL(this, aImageLocation);
		iState = aState;
		}
	}

TBool CMobblerTrack::FetchImageL(const TDesC8& aData)
	{
	TBool found(EFalse);

	// create the XML reader and DOM fragment and associate them with each other
	CSenXmlReader* xmlReader(CSenXmlReader::NewL());
	CleanupStack::PushL(xmlReader);
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	xmlReader->SetContentHandler(*domFragment);
	domFragment->SetReader(*xmlReader);

	// parse the XML into the DOM fragment
	xmlReader->ParseL(aData);

	RPointerArray<CSenElement> imageArray;
	CleanupClosePushL(imageArray);

	if (iState == EFetchingAlbumInfo)
		{
		User::LeaveIfError(domFragment->AsElement().Element(KElementAlbum)->ElementsL(imageArray, KElementImage));
		}
	else
		{
		CSenElement* element(domFragment->AsElement().Element(KElementImages));

		if (element)
			{
			element = element->Element(KElementImage);

			if (element)
				{
				element = element->Element(KElementSizes);

				if (element)
					{
					User::LeaveIfError(element->ElementsL(imageArray, KElementSize));
					}
				}
			}
		}

	const TInt KImageCount(imageArray.Count());
	for (TInt i(0); i < KImageCount; ++i)
		{
		if ((iState == EFetchingAlbumInfo &&
			 imageArray[i]->AttrValue(KElementSize)->Compare(KElementExtraLarge) == 0)
				||
			(iState == EFetchingArtistInfo &&
			 imageArray[i]->AttrValue(KElementName)->Compare(KElementLarge) == 0))
			{
			if (imageArray[i]->Content().Length() > 0)
				{
				found = ETrue;
				iState == EFetchingAlbumInfo ?
					LOG(_L8("15 FetchImageL(album)")):
					LOG(_L8("16 FetchImageL(artist)"));
				iState == EFetchingAlbumInfo ?
					FetchImageL(EFetchingAlbumArt, imageArray[i]->Content()):
					FetchImageL(EFetchingArtistImage, imageArray[i]->Content());
				}
			break;
			}
		}

	CleanupStack::PopAndDestroy(&imageArray);
	CleanupStack::PopAndDestroy(domFragment);
	CleanupStack::PopAndDestroy(xmlReader);

	return found;
	}

void CMobblerTrack::SaveAlbumArtL(const TDesC8& aData)
	{
	if ((iPath && iPath->Length() > 0)
			||
		(iState == EFetchingArtistImage && IsMusicPlayerTrack()))
		{
		// try to save the album art in the album folder
		LOG(_L8("SaveAlbumArtL()"));

		TFileName albumArtFileName;
		if (iPath && iPath->Length() > 0)
			{
			albumArtFileName.Append(*iPath);
			}
		else
			{
			albumArtFileName.Append(KArtistImageCache());
			CCoeEnv::Static()->FsSession().MkDirAll(albumArtFileName);
			}

		TInt knownPathLength(albumArtFileName.Length() +
							 KArtExtensionArray[0].Length());
		if (iState == EFetchingAlbumArt)
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

	iAlbumArt = CMobblerBitmap::NewL(*this, aData);
	iState = ENone;
	}

void CMobblerTrack::DownloadAlbumArtL()
	{
	if ((!iAlbumArt || iUsingArtistImage)
				&& iState == ENone)
		{
		if (Album().String().Length() != 0)
			{
			// There is an album name!

			// Try to fetch the album info. Once this is fetched
			// we will try to fetch the album art in the callback.
			LOG(_L8("2 FetchAlbumInfoL()"));
			FetchAlbumInfoL();
			}
		else if (iImage->Length() != 0)
			{
			// We don't know the album name, but there was album art in the playlist
			LOG(_L8("3 FetchImageL(album)"));
			FetchImageL(EFetchingAlbumArt, *iImage);
			}
		else if (!iUsingArtistImage)
			{
			 // No album art, let's try the artist image instead.
			// Try to fetch the artist info. Once this is fetched
			// we will try to fetch the album art in the callback.
			LOG(_L8("4 FetchArtistInfoL()"));
			FetchArtistInfoL();
			}
		}
	}

// End of file
