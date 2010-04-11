/*
mobblersettingitemlistsettings.cpp

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

#include <s32file.h>

#include "mobbler.hrh"
#include "mobbler.rsg.h"
#include "mobbler_strings.rsg.h"
#include "mobblerappui.h"
#include "mobblerstring.h"
#include "mobblerresourcereader.h"
#include "mobblersettingitemlistsettings.h"
#include "mobblertracer.h"

const TInt KDefaultBufferSizeSeconds(5);
const TInt KDefaultScrobblePercent(50);
const TInt KDefaultEqualizerIndex(-1);
#ifdef __WINS__
const TInt KDefaultVolume(0);
#else
const TInt KDefaultVolume(5);
#endif
const TInt KDefaultSleepTimerMinutes(30);
const TInt KDefaultDownloadAlbumArt(CMobblerAppUi::EOnlyRadio);
const TInt KDefaultSleepTimerAction(CMobblerSettingItemListSettings::EGoOffline);
const TInt KDefaultSleepTimerImmediacy(CMobblerSettingItemListSettings::EImmediately);

_LIT(KDefaultAlarmTime, "070000."); // "HHMMSS."
_LIT(KSettingsFile, "c:settings.ini");

CMobblerSettingItemListSettings* CMobblerSettingItemListSettings::NewL()
	{
    TRACER_AUTO;
	CMobblerSettingItemListSettings* self(new(ELeave) CMobblerSettingItemListSettings);
	return self;
	}

CMobblerSettingItemListSettings::CMobblerSettingItemListSettings()
	:iBufferSize(KDefaultBufferSizeSeconds)
	{
    TRACER_AUTO;
	}

CMobblerSettingItemListSettings::~CMobblerSettingItemListSettings()
	{
    TRACER_AUTO;
	}

void CMobblerSettingItemListSettings::LoadSettingValuesL()
	{
    TRACER_AUTO;
	RFile file;
	CleanupClosePushL(file);
	TInt openError(file.Open(CCoeEnv::Static()->FsSession(), KSettingsFile, EFileRead | EFileShareAny));
	
	// Default values if the settings do not already exist
	TBool backlight(EFalse);
	TBool autoUpdatesOn(ETrue);
	TUint32 iapId(0);
	TUint8 bufferSize(KDefaultBufferSizeSeconds);
	TInt equalizerIndex(KDefaultEqualizerIndex);
	TInt scrobblePercent(KDefaultScrobblePercent);
	TInt volume(KDefaultVolume);
	TInt sleepTimerMinutes(KDefaultSleepTimerMinutes);
	TTime nextUpdateCheck;
	nextUpdateCheck.UniversalTime();
	nextUpdateCheck += TTimeIntervalHours(KUpdateIntervalHours); // the default update check should be 24 hours after install
	CMobblerLastFmConnection::TMode mode(CMobblerLastFmConnection::EOffline);
	TInt downloadAlbumArt(KDefaultDownloadAlbumArt);
#ifdef _DEBUG
	TBool accelerometerGestures(ETrue);
#else
	TBool accelerometerGestures(EFalse);
#endif
	TInt sleepTimerAction(KDefaultSleepTimerAction);
	TInt sleepTimerImmediacy(KDefaultSleepTimerImmediacy);
	TBool alarmOn(EFalse);
	TTime alarmTime(KDefaultAlarmTime);
	TUint32 alarmIapId(0);
	TUint8 bitRate(1);
	TUint32 destinationId(0);
	TInt alarmVolume(KDefaultVolume);
	TInt alarmStation(EMobblerCommandRadioPersonal - EMobblerCommandRadioArtist);
	TBool automaticWallpaper(EFalse);
	
	if (openError == KErrNone)
		{
		RFileReadStream readStream(file);
		CleanupClosePushL(readStream);
		
		TBuf<KMaxMobblerTextSize> username;
		TBuf<KMaxMobblerTextSize> password;
		TBuf<KMaxMobblerTextSize> alarmOption;

		 // Ignore KErrEof if these settings are not yet saved in the file
		TRAP_IGNORE(readStream >> username);
		TRAP_IGNORE(readStream >> password);
		TRAP_IGNORE(backlight = readStream.ReadInt8L());
		TRAP_IGNORE(autoUpdatesOn = readStream.ReadInt8L());
		TRAP_IGNORE(iapId = readStream.ReadUint32L());
		TRAP_IGNORE(bufferSize = readStream.ReadUint8L());
		TRAP_IGNORE(equalizerIndex = readStream.ReadInt16L());
		TRAP_IGNORE(scrobblePercent = readStream.ReadInt16L());
#ifdef __WINS__
		TRAP_IGNORE(readStream.ReadInt16L()); // always use default for WINS (zero) and ignore saved value
#else
		TRAP_IGNORE(volume = readStream.ReadInt16L());
#endif
		TRAP_IGNORE(sleepTimerMinutes = readStream.ReadInt16L());
		
		TUint32 high(0);
		TUint32 low(0);
		TRAPD(errorHigh, high = readStream.ReadInt32L());
		TRAPD(errorLow, low = readStream.ReadInt32L());

#ifdef BETA_BUILD
		TTime thisTimeTomorrow(nextUpdateCheck);
#endif
		if (errorHigh == KErrNone && errorLow == KErrNone)
			{
			nextUpdateCheck = TTime(MAKE_TINT64(high, low));
			}
#ifdef BETA_BUILD
		if (nextUpdateCheck > thisTimeTomorrow)
			{
			nextUpdateCheck = thisTimeTomorrow;
			}
#endif
		
		TRAP_IGNORE(mode = static_cast<CMobblerLastFmConnection::TMode>(readStream.ReadInt8L()));
		TRAP_IGNORE(downloadAlbumArt = readStream.ReadInt16L());
		
		TRAP_IGNORE(accelerometerGestures = readStream.ReadInt8L());
		TRAP_IGNORE(sleepTimerAction = readStream.ReadInt16L());
		TRAP_IGNORE(sleepTimerImmediacy = readStream.ReadInt16L());
		TRAP_IGNORE(alarmOn = readStream.ReadInt8L());

		TRAP(errorHigh, high = readStream.ReadInt32L());
		TRAP(errorLow, low = readStream.ReadInt32L());
		if (errorHigh == KErrNone && errorLow == KErrNone)
			{
			alarmTime = TTime(MAKE_TINT64(high, low));
			}
		alarmIapId = iapId;
		TRAP_IGNORE(alarmIapId = readStream.ReadUint32L());
		TRAP_IGNORE(bitRate = readStream.ReadUint8L());
		TRAP_IGNORE(destinationId = readStream.ReadUint32L());
		alarmVolume = Max(1, volume);
		TRAP_IGNORE(alarmVolume = readStream.ReadInt16L());
		TRAP_IGNORE(alarmStation = readStream.ReadInt32L());
		TRAP_IGNORE(readStream >> alarmOption);
		TRAP_IGNORE(automaticWallpaper = readStream.ReadInt8L());
		TBuf8<KMaxMobblerTextSize> twitterAuthToken;
		TRAP_IGNORE(readStream >> twitterAuthToken);
		TBuf8<KMaxMobblerTextSize> twitterAuthTokenSecret;
		TRAP_IGNORE(readStream >> twitterAuthTokenSecret);
		
		iAlarmOption.Copy(alarmOption);
		
		iUsername.Copy(username);
		iPassword.Copy(password);
		
		iTwitterAuthToken.Copy(twitterAuthToken);
		iTwitterAuthTokenSecret.Copy(twitterAuthTokenSecret);
		
		CleanupStack::PopAndDestroy(&readStream);
		}
	else
		{
		// there was no file there so read from the resource file
		iUsername.Copy(static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->ResourceReader().ResourceL(R_MOBBLER_USERNAME));
		iPassword.Copy(_L("password"));
		}

	iBacklight = backlight;
	iCheckForUpdates = autoUpdatesOn;
	if (static_cast<CMobblerAppUi*>(CCoeEnv::Static()->AppUi())->Destinations())
		{
		iIapId = destinationId;
		}
	else
		{
		iIapId = iapId;
		}
	
	iBufferSize = bufferSize;
	iEqualizerIndex = equalizerIndex;
	iScrobblePercent = scrobblePercent;
	iVolume = volume;
	iSleepTimerMinutes = sleepTimerMinutes;
	iNextUpdateCheck = nextUpdateCheck;
	iMode = mode;
	iDownloadAlbumArt = downloadAlbumArt;
	iAccelerometerGestures = accelerometerGestures;
	iSleepTimerAction = sleepTimerAction;
	iSleepTimerImmediacy = sleepTimerImmediacy;
	iAlarmOn = alarmOn;
	iAlarmTime = alarmTime;
	iAlarmIapId = alarmIapId;
	iBitRate = bitRate;
	iAlarmVolume = alarmVolume;
	iAlarmStation = alarmStation;
	iAutomaticWallpaper = automaticWallpaper;
	
	CleanupStack::PopAndDestroy(&file);
	}

void CMobblerSettingItemListSettings::SaveSettingValuesL()
	{
    TRACER_AUTO;
	RFile file;
	CleanupClosePushL(file);
	CCoeEnv::Static()->FsSession().MkDirAll(KSettingsFile);
	TInt error(file.Replace(CCoeEnv::Static()->FsSession(), KSettingsFile, EFileWrite | EFileShareAny));
	
	if (error == KErrNone)
		{
		RFileWriteStream writeStream(file);
		CleanupClosePushL(writeStream);
		
		writeStream << Username();
		writeStream << Password();
		writeStream.WriteInt8L(Backlight());
		writeStream.WriteInt8L(CheckForUpdates());
		writeStream.WriteUint32L(IapId()); // this is for phones without destinations
		writeStream.WriteUint8L(BufferSize());
		writeStream.WriteInt16L(EqualizerIndex());
		writeStream.WriteInt16L(ScrobblePercent());
		writeStream.WriteInt16L(Volume());
		writeStream.WriteInt16L(SleepTimerMinutes());
		writeStream.WriteInt32L(I64HIGH(NextUpdateCheck().Int64()));
		writeStream.WriteInt32L(I64LOW(NextUpdateCheck().Int64()));
		writeStream.WriteInt8L(Mode());
		writeStream.WriteInt16L(DownloadAlbumArt());
		writeStream.WriteInt8L(AccelerometerGestures());
		writeStream.WriteInt16L(SleepTimerAction());
		writeStream.WriteInt16L(SleepTimerImmediacy());
		writeStream.WriteInt8L(AlarmOn());
		writeStream.WriteInt32L(I64HIGH(AlarmTime().Int64()));
		writeStream.WriteInt32L(I64LOW(AlarmTime().Int64()));
		writeStream.WriteUint32L(AlarmIapId());
		writeStream.WriteUint8L(BitRate());
		writeStream.WriteUint32L(IapId()); // this is for phones with destinations
		writeStream.WriteInt16L(AlarmVolume());
		writeStream.WriteInt32L(AlarmStation());
		writeStream << AlarmOption();
		writeStream.WriteInt8L(AutomaticWallpaper());
		writeStream << TwitterAuthToken();
		writeStream << TwitterAuthTokenSecret();
		
		CleanupStack::PopAndDestroy(&writeStream);
		}
	
	CleanupStack::PopAndDestroy(&file);
	}
	
// End of file
