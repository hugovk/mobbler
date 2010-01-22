/*
mobblertrack.h

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

#ifndef __MOBBLERTRACK_H__
#define __MOBBLERTRACK_H__

#include <e32base.h>

#include <mobbler/mobblercontentlistinginterface.h>

#include "mobblerbitmap.h"
#include "mobblerdataobserver.h"
#include "mobblertrackbase.h"

class CMobblerString;

class CMobblerTrack : public CMobblerTrackBase,
						public MMobblerBitmapObserver,
						public MMobblerFlatDataObserver,
						public MMobblerContentListingObserver
	{
private:
	enum TMobblerImageType
		{
		EMobblerImageTypeNone,
		EMobblerImageTypeArtistRemote,
		EMobblerImageTypeArtistLocal,
		EMobblerImageTypeAlbumRemote,
		EMobblerImageTypeAlbumLocal
		};
	
public:
	static CMobblerTrack* NewL(const TDesC8& aArtist,
								const TDesC8& aTitle,
								const TDesC8& aAlbum,
								const TDesC8& aMbTrackId,
								const TDesC8& aImage,
								const TDesC8& aMp3Location,
								TTimeIntervalSeconds aTrackLength,
								const TDesC8& aRadioAuth,
								TBool aLoved);
	
	void Open();
	void Release();
	
	const CMobblerString& MbTrackId() const;
	
	const TDesC8& Mp3Location() const;

	void SetDataSize(TInt aDataSize);
	TInt DataSize() const;
	void BufferAdded(TInt aBufferSize);
	TInt Buffered() const;
	
	const TDesC& LocalFile() const;
	TPtrC LocalFilePath() const;
	
	void FindBetterImageL();
	const CMobblerBitmap* Image() const;
	
private:
	CMobblerTrack(TTimeIntervalSeconds aTrackLength, TBool aLoved);
	~CMobblerTrack();
	
	void ConstructL(const TDesC8& aArtist,
						const TDesC8& aTitle,
						const TDesC8& aAlbum,
						const TDesC8& aMbTrackId,
						const TDesC8& aImage,
						const TDesC8& aMp3Location,
						const TDesC8& aRadioAuth);
	
	void FetchAlbumInfoL();
	void FetchArtistInfoL();
	TBool FetchImageL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData);
	void FetchImageL(TMobblerImageType aImageType, const TDesC8& aImageLocation);
	void SaveAlbumArtL(const TDesC8& aData);
	TBool OkToDownloadAlbumArt() const;
	
	void FindLocalAlbumImageL();
	void FindLocalArtistImageL();
	
	TBool DownloadAlbumImageL();
	
private:
	void BitmapLoadedL(const CMobblerBitmap* aMobblerBitmap);
	void BitmapResizedL(const CMobblerBitmap* aMobblerBitmap);
	
private: // MMobblerFlatDataObserver
	void DataL(const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError);
	
private: // from MMobblerFlatDataObserverHelper
	void DataL(CMobblerFlatDataObserverHelper* aObserver, const TDesC8& aData, CMobblerLastFmConnection::TTransactionError aTransactionError);
	
private: // from MMobblerContentListingObserver
	void HandleFindLocalTrackCompleteL(TInt aTrackNumber, const TDesC& aAlbum, const TDesC& aLocalFile);
	
private:
	CMobblerString* iMbTrackId;

	// album art
	HBufC8* iPlaylistImageLocation;
	CMobblerBitmap* iImage;
	
	// mp3 location
	HBufC8* iMp3Location;
	HBufC* iLocalFile;
	
	TInt iRefCount;
	
	TInt iDataSize;
	TInt iBuffered;
	
	TMobblerImageType iImageType;
	
	CMobblerFlatDataObserverHelper* iTrackInfoHelper;
	CMobblerFlatDataObserverHelper* iAlbumInfoHelper;
	CMobblerFlatDataObserverHelper* iArtistInfoHelper;
	};
	
#endif // __MOBBLERTRACK_H__

// End of file
