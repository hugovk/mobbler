/*
mobblersettingitemlistsettings.h

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

#ifndef __MOBBLERSETTINGITEMLISTSETTINGS_H__
#define __MOBBLERSETTINGITEMLISTSETTINGS_H__

#include <e32std.h>

#include "mobblerlastfmconnection.h"

class CMobblerSettingItemListSettings : public CBase
	{
public:
	enum TSleepTimerAction
		{
		EStopPlaying,
		EGoOffline,
		ExitMobber
		};

	enum TSleepTimerImmediacy
		{
		EImmediately,
		EEndOfTrack
		};

public:
	static CMobblerSettingItemListSettings* NewL();
	~CMobblerSettingItemListSettings();

private:
	CMobblerSettingItemListSettings();

public:
	void LoadSettingValuesL();
	void SaveSettingValuesL();
	
public:
	TDes& Username() { return iUsername; }
	void SetUsername(const TDesC& aUsername) { iUsername.Copy(aUsername); SaveSettingValuesL(); }
	
	TDes& Password() { return iPassword; }
	void SetPassword(const TDesC& aPassword) { iPassword.Copy(aPassword); SaveSettingValuesL(); }
	
	TBool& Backlight() { return iBacklight; }
	void SetBacklight(TBool aBacklight) { iBacklight = aBacklight; SaveSettingValuesL(); }
	
	TBool& CheckForUpdates() { return iCheckForUpdates; }
	void SetCheckForUpdates(TBool aCheckForUpdates) { iCheckForUpdates = aCheckForUpdates; SaveSettingValuesL(); }
	
	TInt& IapId() { return iIapId; }
	void SetIapId(TInt aIapId) { iIapId = aIapId; SaveSettingValuesL(); }
	
	TInt& BufferSize() { return iBufferSize; }
	void SetBufferSize(TInt aBufferSize) { iBufferSize = aBufferSize; SaveSettingValuesL(); }
	
	TInt& EqualizerIndex() { return iEqualizerIndex; }
	void SetEqualizerIndex(TInt aIndex) { iEqualizerIndex = aIndex; SaveSettingValuesL(); }
	
	TInt& ScrobblePercent() { return iScrobblePercent; }
	void SetScrobblePercent(TInt aPercent) { iScrobblePercent = aPercent; SaveSettingValuesL(); }
	
	TInt& Volume() { return iVolume; }
	void SetVolume(TInt aVolume) { iVolume = aVolume; SaveSettingValuesL(); }
	
	TInt& SleepTimerMinutes() { return iSleepTimerMinutes; }
	void SetSleepTimerMinutes(TInt aSleepTimerMinutes) { iSleepTimerMinutes = aSleepTimerMinutes; SaveSettingValuesL(); }
	
	TTime& NextUpdateCheck() { return iNextUpdateCheck; }
	void SetNextUpdateCheck(TTime aNextUpdateCheck) { iNextUpdateCheck = aNextUpdateCheck; SaveSettingValuesL(); }
	
	CMobblerLastFmConnection::TMode Mode() { return iMode; }
	void SetMode(CMobblerLastFmConnection::TMode aMode) { iMode = aMode; SaveSettingValuesL(); }

	TInt& DownloadAlbumArt() { return iDownloadAlbumArt; }
	void SetDownloadAlbumArt(TInt aDownloadAlbumArt) { iDownloadAlbumArt = aDownloadAlbumArt; SaveSettingValuesL(); }

	TBool& AccelerometerGestures() { return iAccelerometerGestures; }
	void SetAccelerometerGestures(TBool aAccelerometerGestures) { iAccelerometerGestures = aAccelerometerGestures; SaveSettingValuesL(); }
	
	TInt& SleepTimerAction() { return iSleepTimerAction; }
	void SetSleepTimerAction(TInt aSleepTimerAction) { iSleepTimerAction = aSleepTimerAction; SaveSettingValuesL(); }
	
	TInt& SleepTimerImmediacy() { return iSleepTimerImmediacy; SaveSettingValuesL(); }
	void SetSleepTimerImmediacy(TInt aSleepTimerImmediacy) { iSleepTimerImmediacy = aSleepTimerImmediacy; SaveSettingValuesL(); }
	
	TBool& AlarmOn() { return iAlarmOn; }
	void SetAlarmOn(TBool aAlarmOn) { iAlarmOn = aAlarmOn; SaveSettingValuesL(); };
	
	TTime& AlarmTime() { return iAlarmTime; }
	void SetAlarmTime(TTime aAlarmTime) { iAlarmTime = aAlarmTime; SaveSettingValuesL(); }

	TInt& AlarmIapId() { return iAlarmIapId; }
	void SetAlarmIapId(TInt aAlarmIapId) { iAlarmIapId = aAlarmIapId; SaveSettingValuesL(); }
	
	TInt& BitRate() { return iBitRate; }
	void SetBitRate(TInt aBitRate) { iBitRate = aBitRate; SaveSettingValuesL(); }
	
	TInt& AlarmVolume() { return iAlarmVolume; }
	void SetAlarmVolume(TInt aAlarmVolume) { iAlarmVolume = aAlarmVolume; SaveSettingValuesL(); }
	
	TInt& AlarmStation() { return iAlarmStation; }
	void SetAlarmStation(TInt aAlarmStation) { iAlarmStation = aAlarmStation; SaveSettingValuesL(); }
	
	TDes& AlarmOption() { return iAlarmOption; }
	void SetAlarmOption(const TDesC& aAlarmOption) { iAlarmOption.Copy(aAlarmOption); SaveSettingValuesL(); }
	
	TBool& AutomaticWallpaper() { return iAutomaticWallpaper; }
	void SetAutomaticWallpaper(TBool aAutomaticWallpaper) { iAutomaticWallpaper = aAutomaticWallpaper; SaveSettingValuesL(); }
	
	TDesC8& TwitterAuthToken() { return iTwitterAuthToken; }
	void SetTwitterAuthToken(const TDesC8& aTwitterAuthToken) { iTwitterAuthToken.Copy(aTwitterAuthToken); SaveSettingValuesL(); }
	
	TDesC8& TwitterAuthTokenSecret() { return iTwitterAuthTokenSecret; }
	void SetTwitterAuthTokenSecret(const TDesC8& aTwitterAuthTokenSecret) { iTwitterAuthTokenSecret.Copy(aTwitterAuthTokenSecret); SaveSettingValuesL(); }

protected:
	TBuf<KMaxMobblerTextSize> iUsername;
	TBuf<KMaxMobblerTextSize> iPassword;
	TBool iBacklight;
	TBool iCheckForUpdates;
	TInt iIapId;
	TInt iBufferSize;
	TInt iEqualizerIndex;
	TInt iScrobblePercent;
	TInt iVolume;
	TInt iSleepTimerMinutes;
	TTime iNextUpdateCheck;
	CMobblerLastFmConnection::TMode iMode;
	TInt iDownloadAlbumArt;
	TBool iAccelerometerGestures;
	TInt iSleepTimerAction;
	TInt iSleepTimerImmediacy;
	TBool iAlarmOn;
	TTime iAlarmTime;
	TInt iAlarmIapId;
	TInt iBitRate;
	TInt iAlarmVolume;
	TInt iAlarmStation;
	TBuf<KMaxMobblerTextSize> iAlarmOption;
	TBool iAutomaticWallpaper;
	TBuf8<KMaxMobblerTextSize> iTwitterAuthToken;
	TBuf8<KMaxMobblerTextSize> iTwitterAuthTokenSecret;
	};

#endif // __MOBBLERSETTINGITEMLISTSETTINGS_H__

// End of file
