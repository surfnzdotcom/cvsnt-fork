!Ifndef VERSION

all: cvsnt trial

release: cvsnt trial

cvsnt: cvsnt-client cvsnt-server

cvsnt-client:
	@for /f "usebackq delims=" %%a in (`cvs version -q`) do $(MAKE) -f makefile /nologo $(MAKEFLAGS) VERSION=%%a LICENSE=gpl.rtf BINARY_TYPE=bin/ReleaseTrial $@

cvsnt-server:
	@for /f "usebackq delims=" %%a in (`cvs version -q`) do $(MAKE) -f makefile /nologo $(MAKEFLAGS) VERSION=%%a LICENSE=gpl.rtf BINARY_TYPE=bin/ReleaseTrial $@

combined: suite

suite:	suite-server suite-client

suite-server:
	@for /f "usebackq delims=" %%a in (`cvs version -q`) do $(MAKE) -f makefile /nologo $(MAKEFLAGS) VERSION=%%a LICENSE=combined-license.rtf BINARY_TYPE=bin/Release $@

suite-client:
	@for /f "usebackq delims=" %%a in (`cvs version -q`) do $(MAKE) -f makefile /nologo $(MAKEFLAGS) VERSION=%%a LICENSE=combined-license.rtf BINARY_TYPE=bin/Release $@

trial:	trial-server trial-client

trial-server:
	@for /f "usebackq delims=" %%a in (`cvs version -q`) do $(MAKE) -f makefile /nologo $(MAKEFLAGS) VERSION=%%a LICENSE=combined-license.rtf BINARY_TYPE=bin/ReleaseTrial $@

trial-client:
	@for /f "usebackq delims=" %%a in (`cvs version -q`) do $(MAKE) -f makefile /nologo $(MAKEFLAGS) VERSION=%%a LICENSE=combined-license.rtf BINARY_TYPE=bin/ReleaseTrial $@

clean:
	del *.wixobj *.msi *.cab _license.rtf

cleanall:
	del *.wixobj *.msi *.cab _license.rtf

!Else
 
CVSNT_CLIENT_OBJS=cvsnt-client.wixobj ui.wixobj cvsnt.wixobj common.wixobj msvcr71.wixobj mfc71u.wixobj cvsagent.wixobj register.wixobj
CVSNT_SERVER_OBJS=cvsnt-server.wixobj ui.wixobj cvsnt.wixobj server.wixobj common.wixobj msvcr71.wixobj msvcrt.wixobj mfc71u.wixobj register.wixobj
SUITE_CLIENT_OBJS=suite-client.wixobj ui.wixobj cvsnt.wixobj mfc71.wixobj mfc71u.wixobj  wincvs.wixobj mfc42u.wixobj msvcrt.wixobj winmerge.wixobj tortoisecvs.wixobj releasemanager.wixobj workspacemanager.wixobj ebook.wixobj scci.wixobj common.wixobj msvcr71.wixobj cvsagent.wixobj register.wixobj
SUITE_SERVER_OBJS=suite-server.wixobj ui.wixobj cvsnt.wixobj server.wixobj triggers.wixobj unison.wixobj wait4cvs.wixobj common.wixobj msvcr71.wixobj msvcrt.wixobj mfc71u.wixobj register.wixobj
TRIAL_CLIENT_OBJS=suite-client-trial.wixobj ui.wixobj cvsnt.wixobj mfc71.wixobj mfc71u.wixobj  wincvs.wixobj mfc42u.wixobj msvcrt.wixobj winmerge.wixobj tortoisecvs.wixobj workspacemanager.wixobj common.wixobj msvcr71.wixobj cvsagent.wixobj register.wixobj ebooktrial.wixobj # scci.wixobj
TRIAL_SERVER_OBJS=suite-server-trial.wixobj ui.wixobj cvsnt.wixobj server.wixobj unison.wixobj wait4cvs.wixobj common.wixobj msvcr71.wixobj msvcrt.wixobj mfc71u.wixobj register.wixobj

CANDLEFLAGS=-sw1044 -dCVSNT_VERSION=$(VERSION) -dLICENSE=_license.rtf -dCVSNT_BASE=d:\cvsbin -dCVSNT64_BASE=d:\cvsbin64 -dWINCVS_BASE=d:\march-hare\wincvs -dWINMERGE_BASE=d:\march-hare\winmerge -dTORTOISECVS_BASE=d:\march-hare\TortoiseCVS -dMARCH_HARE=d:\march-hare\$(BINARY_TYPE) -dSMARTLOADER=d:\cvsdeps\smartloader -dMERGEFILES="%ProgramFiles%/Common Files/Merge Modules" -dSYSFILES=d:/cvsdeps/sysfiles

CVSNT_CLIENT_TARGET=cvsnt-client-$(VERSION).msi
CVSNT_SERVER_TARGET=cvsnt-server-$(VERSION).msi
TRIAL_CLIENT_TARGET=suite-client-trial-$(VERSION).msi
TRIAL_SERVER_TARGET=suite-server-trial-$(VERSION).msi
SUITE_CLIENT_TARGET=suite-client-$(VERSION).msi
SUITE_SERVER_TARGET=suite-server-$(VERSION).msi

all: cvsnt trial

cvsnt: cvsnt-client cvsnt-server

cvsnt-client: $(CVSNT_CLIENT_TARGET)

cvsnt-server: $(CVSNT_SERVER_TARGET)

suite: suite-client suite-server

suite-client: $(SUITE_CLIENT_TARGET)

suite-server: $(SUITE_SERVER_TARGET)

trial: trial-client trial-server

trial-client: $(TRIAL_CLIENT_TARGET)

trial-server: $(TRIAL_SERVER_TARGET)

clean:
	del *.wixobj *.msi *.cab _license.rtf

$(CVSNT_CLIENT_TARGET): $(CVSNT_CLIENT_OBJS)
	copy $(LICENSE) _license.rtf
	light -o $@ $(CVSNT_CLIENT_OBJS)
	signcode -n "March Hare CVSNT Client" -i "http://march-hare.com/cvsnt/" -a sha1 -cn "March Hare Software Ltd" -t http://timestamp.verisign.com/scripts/timestamp.dll $@


$(CVSNT_SERVER_TARGET): $(CVSNT_SERVER_OBJS)
	copy $(LICENSE) _license.rtf
	light -o $@ $(CVSNT_SERVER_OBJS)
	signcode -n "March Hare CVSNT Server" -i "http://march-hare.com/cvsnt/" -a sha1 -cn "March Hare Software Ltd" -t http://timestamp.verisign.com/scripts/timestamp.dll $@

$(SUITE_CLIENT_TARGET): $(SUITE_CLIENT_OBJS)
	copy $(LICENSE) _license.rtf
	light -o $@ $(SUITE_CLIENT_OBJS)
	signcode -n "CVS Suite" -i "http://march-hare.com/cvsnt/" -a sha1 -cn "March Hare Software Ltd" -t http://timestamp.verisign.com/scripts/timestamp.dll $@

$(SUITE_SERVER_TARGET): $(SUITE_SERVER_OBJS)
	copy $(LICENSE) _license.rtf
	light -o $@ $(SUITE_SERVER_OBJS)
	signcode -n "CVS Suite" -i "http://march-hare.com/cvsnt/" -a sha1 -cn "March Hare Software Ltd" -t http://timestamp.verisign.com/scripts/timestamp.dll $@

$(TRIAL_CLIENT_TARGET): $(TRIAL_CLIENT_OBJS)
	copy $(LICENSE) _license.rtf
	light -o $@ $(TRIAL_CLIENT_OBJS)
	signcode -n "CVS Suite Trial Edition" -i "http://march-hare.com/cvsnt/" -a sha1 -cn "March Hare Software Ltd" -t http://timestamp.verisign.com/scripts/timestamp.dll $@

$(TRIAL_SERVER_TARGET): $(TRIAL_SERVER_OBJS)
	copy $(LICENSE) _license.rtf
	light -o $@ $(TRIAL_SERVER_OBJS)
	signcode -n "CVS Suite Trial Edition" -i "http://march-hare.com/cvsnt/" -a sha1 -cn "March Hare Software Ltd" -t http://timestamp.verisign.com/scripts/timestamp.dll $@

.wxs.wixobj:
	candle $(CANDLEFLAGS) $<

.SUFFIXES: .wxs

!Endif
