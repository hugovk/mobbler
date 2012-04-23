# 
# Mobbler, a Last.fm mobile scrobbler for Symbian smartphones.
# Copyright (C) 2008, 2009, 2010  Michael Coffey
# Copyright (C) 2009, 2010, 2012  Hugo van Kemenade
# 
# http://code.google.com/p/mobbler
# 
# This file is part of Mobbler.
# 
# Mobbler is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# Mobbler is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with Mobbler.  If not, see <http://www.gnu.org/licenses/>.
# 

ifeq (WINS,$(findstring WINS, $(PLATFORM)))
ZDIR=$(EPOCROOT)epoc32\release\$(PLATFORM)\$(CFG)\Z
else
ZDIR=$(EPOCROOT)epoc32\data\z
endif

TARGETDIR=$(ZDIR)\resource\apps

# Platform independent:
ICONTARGETFILENAME=$(TARGETDIR)\mobbler.mif
SSICONTARGETFILENAME=$(TARGETDIR)\ssmobbler.mif

# For S60 3rd Edition:
BUTTONSTARGETFILENAME_3RD=$(TARGETDIR)\mobblerbuttons_3rd.mif

# For S60 5th Edition and later:
BUTTONSTARGETFILENAME_5TH=$(TARGETDIR)\mobblerbuttons_5th.mif

ICONDIR=..\gfx

HEADERDIR=$(EPOCROOT)epoc32\include

# Platform independent:
HEADERFILENAME=$(HEADERDIR)\mobbler.mbg
SSHEADERFILENAME=$(HEADERDIR)\ssmobbler.mbg

# Platform dependent:
BUTTONSHEADERFILENAME=$(HEADERDIR)\mobblerbuttons.mbg

do_nothing :
	@rem do_nothing

MAKMAKE : do_nothing

BLD : do_nothing

CLEAN : do_nothing

LIB : do_nothing

CLEANLIB : do_nothing

RESOURCE :

# Platform independent:
	mifconv $(ICONTARGETFILENAME) /X /h$(HEADERFILENAME) \
		/c32,8 $(ICONDIR)\mobbler.svg \
		/c32,8 $(ICONDIR)\speaker_low.svg \
		/c32,8 $(ICONDIR)\speaker_high.svg \
		/c32,8 $(ICONDIR)\drive_harddisk.svg \
		/c32,8 $(ICONDIR)\on_tour.svg

	mifconv $(SSICONTARGETFILENAME) /X /h$(SSHEADERFILENAME) \
		/c32,8 $(ICONDIR)\mobbler_icon_weird.svg \
		/c32,8 $(ICONDIR)\speaker_low.svg \
		/c32,8 $(ICONDIR)\speaker_high.svg \
		/c32,8 $(ICONDIR)\drive_harddisk.svg \
		/c32,8 $(ICONDIR)\on_tour.svg

# For S60 3rd Edition:
	mifconv $(BUTTONSTARGETFILENAME_3RD) /X /h$(BUTTONSHEADERFILENAME) \
		/c32,8 $(ICONDIR)\icons3rd\more.svg \
		/c32,8 $(ICONDIR)\icons3rd\love.svg \
		/c32,8 $(ICONDIR)\icons3rd\ban.svg \
		/c32,8 $(ICONDIR)\icons3rd\stop.svg \
		/c32,8 $(ICONDIR)\icons3rd\play.svg \
		/c32,8 $(ICONDIR)\icons3rd\next.svg \
		/c32,8 $(ICONDIR)\icons3rd\pause.svg

# For S60 5th Edition and later, with -V<SVGT binary code version>:
#       "3 – RGB / fixed point encoding. Performance improved compared to 
#       the binary type of version 1. Default in S60 3.1, 3.2 and 5.0."
	mifconv $(BUTTONSTARGETFILENAME_5TH) /X /h$(BUTTONSHEADERFILENAME) -V3 \
		/c32,8 $(ICONDIR)\icons5th\more.svg \
		/c32,8 $(ICONDIR)\icons5th\love.svg \
		/c32,8 $(ICONDIR)\icons5th\ban.svg \
		/c32,8 $(ICONDIR)\icons5th\stop.svg \
		/c32,8 $(ICONDIR)\icons5th\play.svg \
		/c32,8 $(ICONDIR)\icons5th\next.svg \
		/c32,8 $(ICONDIR)\icons5th\pause.svg

FREEZE : do_nothing

SAVESPACE : do_nothing

RELEASABLES :
	@echo $(ICONTARGETFILENAME)

FINAL : do_nothing

# End of file
