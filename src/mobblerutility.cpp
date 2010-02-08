/*
mobblerutility.cpp

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
#include <coemain.h>
#include <hal.h>
#include <hash.h>
#include <sendomfragment.h>

#include "mobblerlogging.h"
#include "mobblerutility.h"

_LIT8(KChineseLangCode, "cn");
_LIT8(KEnglishLangCode, "en");
_LIT8(KFrenchLangCode, "fr");
_LIT8(KGermanLangCode, "de");
_LIT8(KItalianLangCode, "it");
_LIT8(KJapaneseLangCode, "jp");
_LIT8(KPolishLangCode, "pl");
_LIT8(KPortugueseLangCode, "pt");
_LIT8(KRussianLangCode, "ru");
_LIT8(KSpanishLangCode, "es");
_LIT8(KSwedishLangCode, "sv");
_LIT8(KTurkishLangCode, "tu");

// _LIT(KChineseUrl,		"http://cn.last.fm/"); /// No known Chinese mobile site
_LIT(KEnglishUrl,		"http://m.last.fm/");
_LIT(KFrenchUrl,		"http://m.lastfm.fr/");
_LIT(KGermanUrl,		"http://m.lastfm.de/");
_LIT(KItalianUrl,		"http://m.lastfm.it/");
_LIT(KJapaneseUrl, 		"http://m.lastfm.jp/");
_LIT(KPolishUrl,		"http://m.lastfm.pl/");
_LIT(KPortugueseUrl,	"http://m.lastfm.com.br/");
_LIT(KRussianUrl,		"http://m.lastfm.ru/");
_LIT(KSpanishUrl,		"http://m.lastfm.es/");
_LIT(KSwedishUrl,		"http://m.lastfm.se/");
_LIT(KTurkishUrl,		"http://m.lastfm.com.tr/");

_LIT8(KFormat1, "%02x");
_LIT8(KFormat2, "%%%2x");

const TInt KNokiaE52MachineUid(0x20014DCC);
const TInt KNokiaE55MachineUid(0x20014DCF);
const TInt KNokiaE72MachineUid(0x20014DD0);

TBool MobblerUtility::EqualizerSupported()
	{
	TInt machineUid(0);
	TInt error(HAL::Get(HALData::EMachineUid, machineUid));
	return (error == KErrNone) && !(machineUid == KNokiaE52MachineUid || 
									machineUid == KNokiaE55MachineUid || 
									machineUid == KNokiaE72MachineUid);
	}

HBufC8* MobblerUtility::MD5LC(const TDesC8& aSource)
	{
	CMD5* md5(CMD5::NewL());
	CleanupStack::PushL(md5);
	
	TPtrC8 hash(md5->Hash(aSource));
	HBufC8* hashResult(HBufC8::NewLC(hash.Length() * 2));
	
	for (TInt i(0); i < hash.Length(); ++i)
		{
		hashResult->Des().AppendFormat(KFormat1, hash[i]);
		}
	
	CleanupStack::Pop(hashResult);
	CleanupStack::PopAndDestroy(md5);
	CleanupStack::PushL(hashResult);
	return hashResult;
	}

HBufC8* MobblerUtility::URLEncodeLC(const TDesC8& aString)
	{
	HBufC8* urlEncoded(HBufC8::NewLC(aString.Length() * 3));
	// sanitise the input string
	const TInt KCharCount(aString.Length());
	for (TInt i(0); i < KCharCount; ++i)
		{
		urlEncoded->Des().AppendFormat(KFormat2, aString[i]);
		}
	
	return urlEncoded;
	}

TBuf8<2> MobblerUtility::LanguageL()
	{
	TBuf8<2> language;
	language.Copy(KEnglishLangCode); // default to English
	
	RArray<TLanguage> downgradePath;
	CleanupClosePushL(downgradePath);
	BaflUtils::GetDowngradePathL(CCoeEnv::Static()->FsSession(), User::Language(), downgradePath);
	
	TBool languageFound(EFalse);
	
	const TInt KLanguageCount(downgradePath.Count());
	for (TInt i(0); i < KLanguageCount && !languageFound; ++i)
		{
		languageFound = ETrue;
		
		switch (downgradePath[i])
			{
			case ELangEnglish:
				language.Copy(KEnglishLangCode);
				break;
			case ELangFrench:
				language.Copy(KFrenchLangCode);
				break;
			case ELangGerman: 
				language.Copy(KGermanLangCode);
				break;
			case ELangSpanish: 
				language.Copy(KSpanishLangCode);
				break;
			case ELangItalian: 
				language.Copy(KItalianLangCode);
				break;
			case ELangSwedish: 
				language.Copy(KSwedishLangCode);
				break;
			case ELangPortuguese: 
				language.Copy(KPortugueseLangCode);
				break;
			case ELangRussian: 
				language.Copy(KRussianLangCode);
				break;
			case ELangPolish:
				language.Copy(KPolishLangCode);
				break;
			case ELangPrcChinese: 
				language.Copy(KChineseLangCode);
				break;
			case ELangJapanese: 
				language.Copy(KJapaneseLangCode);
				break;
			case ELangTurkish:
				language.Copy(KTurkishLangCode);
				break;
			default:
				// carry on iterating through the downgrade path
				languageFound = EFalse;
				break;
			};
		}
	
	CleanupStack::PopAndDestroy(&downgradePath);
	
	return language;
	}

TBuf<30> MobblerUtility::LocalLastFmDomainL()
	{
	TBuf<30> url;
	url.Copy(KEnglishUrl); // default to English
	
	RArray<TLanguage> downgradePath;
	CleanupClosePushL(downgradePath);
	BaflUtils::GetDowngradePathL(CCoeEnv::Static()->FsSession(), User::Language(), downgradePath);
	
	TBool languageFound(EFalse);
	
	const TInt KLanguageCount(downgradePath.Count());
	for (TInt i(0); i < KLanguageCount && !languageFound; ++i)
		{
		languageFound = ETrue;
		
		switch (downgradePath[i])
			{
			case ELangEnglish:
				url.Copy(KEnglishUrl);
				break;
			case ELangFrench:
				url.Copy(KFrenchUrl);
				break;
			case ELangGerman: 
				url.Copy(KGermanUrl);
				break;
			case ELangSpanish: 
				url.Copy(KSpanishUrl);
				break;
			case ELangItalian: 
				url.Copy(KItalianUrl);
				break;
			case ELangSwedish: 
				url.Copy(KSwedishUrl);
				break;
			case ELangPortuguese: 
				url.Copy(KPortugueseUrl);
				break;
			case ELangRussian: 
				url.Copy(KRussianUrl);
				break;
			case ELangPolish:
				url.Copy(KPolishUrl);
				break;
/*			case ELangPrcChinese: 
				url.Copy(KChineseUrl); // No known Chinese mobile site
				break;*/
			case ELangJapanese: 
				url.Copy(KJapaneseUrl);
				break;
			case ELangTurkish:
				url.Copy(KTurkishUrl);
				break;
			default:
				// carry on iterating through the downgrade path
				languageFound = EFalse;
				break;
			};
		}
	
	CleanupStack::PopAndDestroy(&downgradePath);
	
	return url;
	}

void MobblerUtility::FixLyricsSpecialCharacters(TDes8& aText)
	{
	// Lyricsfly: "Because our database varies with many html format encodings 
	// including international characters, we recommend that you replace all 
	// quotes, ampersands and all other special and international characters 
	// with "%". Simply put; if the character is not [A-Z a-z 0-9] or space, 
	// just substitute "%" for it to get most out of your results."
	
	_LIT8(KSubstitute, "%");
	
	for (TInt i(0); i < aText.Length(); ++i)
		{
		TChar ch(aText[i]);

		if (ch.IsDigit() || ch.IsSpace())
			{
			// Do nothing
			}
		else
			{
			// Do nothing if [A-Za-z]
			ch.LowerCase();
			switch (ch)
				{
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
				case 'g':
				case 'h':
				case 'i':
				case 'j':
				case 'k':
				case 'l':
				case 'm':
				case 'n':
				case 'o':
				case 'p':
				case 'q':
				case 'r':
				case 's':
				case 't':
				case 'u':
				case 'v':
				case 'w':
				case 'x':
				case 'y':
				case 'z':
					// Do nothing
					break;
				default:
					// Replace with %
					LOG(_L8("Replace with %"));
					aText.Delete(i, 1);
					aText.Insert(i, KSubstitute);
					break;
				}
			}
		}
	}

void MobblerUtility::FixLyricsLineBreaks(TDes8& aText)
	{
	// First, remove all Windows newlines
	_LIT8(KCRLF,"\x0D\x0A");

	TInt pos(KErrNotFound);
	while ((pos = aText.Find(KCRLF)) != KErrNotFound)
		{
		aText.Delete(pos, KCRLF().Length());
		}
	
	// Next, remove all Linux newlines
	_LIT8(KLF,"\x0A");
	
	pos = KErrNotFound;
	while ((pos = aText.Find(KLF)) != KErrNotFound)
		{
		aText.Delete(pos, KLF().Length());
		}
	
	// Finally, replace [br] tags with newlines
	_LIT8(KBrTag1, "[br]");
	_LIT8(KBrTag2, "\r\n");
	
	pos = KErrNotFound;
	while ((pos = aText.Find(KBrTag1)) != KErrNotFound)
		{
		aText.Delete(pos, KBrTag1().Length());
		aText.Insert(pos, KBrTag2);
		}
	}

void MobblerUtility::StripUnwantedTagsFromHtmlL(HBufC8*& aHtml)
	{
	_LIT8(KAnchorStart, "<a");

	TPtr8 htmlPtr(aHtml->Des());

	TInt pos(KErrNotFound);
	while ((pos = htmlPtr.Find(KAnchorStart)) != KErrNotFound)
		{
		TPtrC8 ptrFromPos = htmlPtr.MidTPtr(pos);
		TInt endBracketPos = ptrFromPos.Locate('>');
		if (endBracketPos == KErrNotFound)
			{
			break; // HTML not well-formed, just stop
			}
		htmlPtr.Delete(pos, endBracketPos + 1);
		}

	_LIT8(KAnchorEnd, "</a>");
	while ((pos = htmlPtr.Find(KAnchorEnd)) != KErrNotFound)
		{
		htmlPtr.Delete(pos, KAnchorEnd().Length());
		}

	_LIT8(KBandMemberTag, "[bandmember]");
	while ((pos = htmlPtr.Find(KBandMemberTag)) != KErrNotFound)
		{
		TPtrC8 ptrFromPos = htmlPtr.MidTPtr(pos);
		TInt endBracketPos = ptrFromPos.Locate(']');
		if (endBracketPos == KErrNotFound)
			{
			break; // Tag not well-formed, just stop
			}
		htmlPtr.Delete(pos, endBracketPos + 1);
		}

	_LIT8(KBandMemberEndTag, "[/bandmember]");
	while ((pos = htmlPtr.Find(KBandMemberEndTag)) != KErrNotFound)
		{
		htmlPtr.Delete(pos, KBandMemberEndTag().Length());
		}

	// It makes the tidying code easier to use <br /><br /> as paragraph
	// breaks rather than <p>...</p>

	// Calculate if the descriptor's maximum length is big
	// enough to store the extra characters for the line breaks
	// Finally, replace paragraph with newlines
	_LIT8(KBrTag1, "<br /><br />");
	_LIT8(KNewLine, "\x20\x0A");

	// We don't want to keep growing the descriptor so do a quick
	// scan of the HTML and see if there is enough space in the
	// descriptor to store the extra <br />
	// In most cases, we will have gained some space from stripping
	// out anchors etc.
	while ((pos = htmlPtr.Find(KNewLine)) != KErrNotFound)
		{
		htmlPtr.Delete(pos, KNewLine().Length());

		if ((htmlPtr.Length() + KBrTag1().Length()) > htmlPtr.MaxLength())
			{
			CleanupStack::PushL(aHtml);
			aHtml = aHtml->ReAllocL(htmlPtr.Length() + KBrTag1().Length() * 3); // 3 times so that we don't have to keep reallocating
			CleanupStack::Pop();
			htmlPtr.Set(aHtml->Des());
			}
		htmlPtr.Insert(pos, KBrTag1);
		}
	}

CSenDomFragment* MobblerUtility::PrepareDomFragmentLC(CSenXmlReader& aXmlReader, const TDesC8& aXml)
	{
	// Create the DOM fragment and associate with the XML reader
	CSenDomFragment* domFragment(CSenDomFragment::NewL());
	CleanupStack::PushL(domFragment);
	aXmlReader.SetContentHandler(*domFragment);
	domFragment->SetReader(aXmlReader);
	
	// Parse the XML into the DOM fragment
	aXmlReader.ParseL(aXml);
	
	return domFragment;
	}

// End of file
