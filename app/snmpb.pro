SOURCES	+= ../snmp++/src/address.cpp \
	../snmp++/src/asn1.cpp \
	../snmp++/src/auth_priv.cpp \
	../snmp++/src/counter.cpp \
	../snmp++/src/ctr64.cpp \
	../snmp++/src/eventlist.cpp \
	../snmp++/src/eventlistholder.cpp \
	../snmp++/src/gauge.cpp \
	../snmp++/src/idea.cpp \
	../snmp++/src/integer.cpp \
	../snmp++/src/log.cpp \
	../snmp++/src/md5c.cpp \
	../snmp++/src/mp_v3.cpp \
	../snmp++/src/msec.cpp \
	../snmp++/src/msgqueue.cpp \
	../snmp++/src/notifyqueue.cpp \
	../snmp++/src/octet.cpp \
	../snmp++/src/oid.cpp \
	../snmp++/src/pdu.cpp \
	../snmp++/src/reentrant.cpp \
	../snmp++/src/sha.cpp \
	../snmp++/src/snmpmsg.cpp \
	../snmp++/src/target.cpp \
	../snmp++/src/timetick.cpp \
	../snmp++/src/usm_v3.cpp \
	../snmp++/src/uxsnmp.cpp \
	../snmp++/src/v3.cpp \
	../snmp++/src/vb.cpp \
	../snmp++/src/IPv6Utility.cpp \
	main.cpp \
	snmpb.cpp \
	mibnode.cpp \
	mibview.cpp \
	mibmodule.cpp \
	agent.cpp \
	trap.cpp \
	graph.cpp \
	comboboxes.cpp \
	mibhighlighter.cpp \
	markerwidget.cpp \
	mibeditor.cpp \
	mibtextedit.cpp \
	mibselection.cpp \
	logsnmpb.cpp \
	discovery.cpp \
	agentprofile.cpp \
	usmprofile.cpp \
	preferences.cpp

HEADERS	+= snmpb.h \
        snmpbapp.h \
	mibnode.h \
	mibview.h \
	mibmodule.h \
	agent.h \
	trap.h \
	graph.h \
	comboboxes.h \
	mibhighlighter.h \
	markerwidget.h \
	mibeditor.h \
	mibtextedit.h \
	mibselection.h \
	logsnmpb.h \
	discovery.h \
	agentprofile.h \
	usmprofile.h \
	preferences.h

FORMS	= mainw.ui agentprofile.ui usmprofile.ui preferences.ui gotoline.ui find.ui replace.ui varbinds.ui plot.ui
TEMPLATE	= app
CONFIG	+= qt warn_on debug
RESOURCES     = snmpb.qrc
INCLUDEPATH	+= ../snmp++/include ../snmp++/ ../libtomcrypt/src/headers ../libsmi/lib ../qwt/src
LIBS	+= -L. -L../libtomcrypt -L../libsmi/lib/.libs -L../qwt/lib -lsmi -ltomcrypt -lqwt
LANGUAGE	= C++

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}
win32 {
  CONFIG += release
  RC_FILE = snmpb.rc
  LIBS	+= -lws2_32 -L../libsmi/win
}
macx:ICON = images/snmpb.icns

