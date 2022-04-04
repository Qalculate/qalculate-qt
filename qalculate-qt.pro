VERSION = 4.1.1
isEmpty(PREFIX) {
	PREFIX = /usr/local
}
isEmpty(DESKTOP_DIR) {
	DESKTOP_DIR = $$PREFIX/share/applications
}
isEmpty(DESKTOP_ICON_DIR) {
	DESKTOP_ICON_DIR = $$PREFIX/share/icons
}
unix:!equals(COMPILE_RESOURCES,"yes"):!android:!macx {
	isEmpty(TRANSLATIONS_DIR) {
		TRANSLATIONS_DIR = $$PREFIX/share/qalculate-qt/translations
	}
	DEFINES += TRANSLATIONS_DIR=\\\"$$TRANSLATIONS_DIR\\\"
}
isEmpty(MAN_DIR) {
	MAN_DIR = $$PREFIX/share/man
}
isEmpty(APPDATA_DIR) {
	APPDATA_DIR = $$PREFIX/share/metainfo
}
TEMPLATE = app
TARGET = qalculate-qt
INCLUDEPATH += src
win32: {
	LIBS += -lqalculate -lxml2 -lmpfr -liconv -lintl -lgmp -licuuc -lcurl
	CONFIG += c++17
} else {
	CONFIG += link_pkgconfig
	macx: {
		PKGCONFIG += libqalculate gmp mpfr
		CONFIG += c++11
	} else {
		PKGCONFIG += libqalculate
	}
}
CONFIG += qt
QT += widgets network
MOC_DIR = build
OBJECTS_DIR = build

HEADERS += src/calendarconversiondialog.h src/csvdialog.h src/dataseteditdialog.h src/datasetsdialog.h src/expressionedit.h src/fpconversiondialog.h src/functioneditdialog.h src/functionsdialog.h src/historyview.h src/itemproxymodel.h src/keypadwidget.h src/matrixwidget.h src/percentagecalculationdialog.h src/periodictabledialog.h src/plotdialog.h src/preferencesdialog.h src/qalculateqtsettings.h src/qalculatewindow.h src/unitsdialog.h src/uniteditdialog.h src/unknowneditdialog.h src/variableeditdialog.h src/variablesdialog.h

SOURCES += src/calendarconversiondialog.cpp src/csvdialog.cpp src/dataseteditdialog.cpp src/datasetsdialog.cpp src/expressionedit.cpp src/fpconversiondialog.cpp src/functioneditdialog.cpp src/functionsdialog.cpp src/historyview.cpp src/itemproxymodel.cpp src/keypadwidget.cpp src/main.cpp src/matrixwidget.cpp src/percentagecalculationdialog.cpp src/periodictabledialog.cpp src/plotdialog.cpp src/preferencesdialog.cpp src/qalculateqtsettings.cpp src/qalculatewindow.cpp src/unitsdialog.cpp src/uniteditdialog.cpp src/unknowneditdialog.cpp src/variableeditdialog.cpp src/variablesdialog.cpp

LANGUAGES = ca de es fr nl pt_BR ru sl sv zh_CN

#parameters: var, prepend, append
defineReplace(prependAll) {
	for(a,$$1):result += $$2$${a}$$3
	return($$result)
}

TRANSLATIONS = 	translations/qalculate-qt_ca.ts \
		translations/qalculate-qt_de.ts \
		translations/qalculate-qt_es.ts \
		translations/qalculate-qt_fr.ts \
		translations/qalculate-qt_nl.ts \
		translations/qalculate-qt_pt_BR.ts \
		translations/qalculate-qt_ru.ts \
		translations/qalculate-qt_sl.ts \
		translations/qalculate-qt_sv.ts \
		translations/qalculate-qt_zh_CN.ts

TRANSLATIONS = $$prependAll(LANGUAGES, $$PWD/translations/qalculate-qt_, .ts)
TRANSLATIONS_FILES =
qtPrepareTool(LRELEASE, lrelease) for(tsfile, TRANSLATIONS) {
	qmfile = $$shadowed($$tsfile)
	qmfile ~= s,.ts$,.qm,
	qmdir = $$dirname(qmfile)
	exists($$qmdir) {
		mkpath($$qmdir)|error("Aborting.")
	}
	command = $$LRELEASE -removeidentical $$tsfile -qm $$qmfile
	system($$command)|error("Failed to run: $$command")
	TRANSLATIONS_FILES += $$qmfile
}

unix:!equals(COMPILE_RESOURCES,"yes"):!android:!macx {

	target.path = $$PREFIX/bin

	qm.files = 	translations/qalculate-qt_ca.qm \
			translations/qalculate-qt_de.qm \
			translations/qalculate-qt_es.qm \
			translations/qalculate-qt_fr.qm \
			translations/qalculate-qt_nl.qm \
			translations/qalculate-qt_pt_BR.qm \
			translations/qalculate-qt_ru.qm \
			translations/qalculate-qt_sl.qm \
			translations/qalculate-qt_sv.qm \
			translations/qalculate-qt_zh_CN.qm
		
	qm.path = $$TRANSLATIONS_DIR

	desktop.files = data/qalculate-qt.desktop
	desktop.path = $$DESKTOP_DIR

	appdata.files = data/qalculate-qt.appdata.xml
	appdata.path = $$APPDATA_DIR

	appicon16.files = data/16/qalculate-qt.png
	appicon16.path = $$DESKTOP_ICON_DIR/hicolor/16x16/apps
	appicon22.files = data/22/qalculate-qt.png
	appicon22.path = $$DESKTOP_ICON_DIR/hicolor/22x22/apps
	appicon32.files = data/32/qalculate-qt.png
	appicon32.path = $$DESKTOP_ICON_DIR/hicolor/32x32/apps
	appicon64.files = data/64/qalculate-qt.png
	appicon64.path = $$DESKTOP_ICON_DIR/hicolor/64x64/apps
	appicon128.files = data/128/qalculate-qt.png
	appicon128.path = $$DESKTOP_ICON_DIR/hicolor/128x128/apps
	appiconsvg.files = data/scalable/qalculate-qt.svg
	appiconsvg.path = $$DESKTOP_ICON_DIR/hicolor/scalable/apps

	INSTALLS += 	target desktop appdata qm \
			appicon16 appicon22 appicon32 appicon64 appicon128 appiconsvg

	RESOURCES = icons.qrc flags.qrc
} else {
	RESOURCES = icons.qrc flags.qrc translations.qrc
	target.path = $$PREFIX/bin
	desktop.files = data/qalculate-qt.desktop
	desktop.path = $$DESKTOP_DIR
	appicon64.files = data/64/qalculate-qt.png
	appicon64.path = $$DESKTOP_ICON_DIR/hicolor/64x64/apps
	INSTALLS += target desktop appicon64
}

unix:!android:!macx {
	man.files = data/qalculate-qt.1
	man.path = $$MAN_DIR/man1
	INSTALLS += man
}

win32: RC_FILE = winicon.rc
