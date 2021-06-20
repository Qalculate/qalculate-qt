VERSION = 0.1
isEmpty(PREFIX) {
	PREFIX = /usr/local
}
isEmpty(DESKTOP_DIR) {
	DESKTOP_DIR = $$PREFIX/share/applications
}
isEmpty(DESKTOP_ICON_DIR) {
	DESKTOP_ICON_DIR = $$PREFIX/share/icons
}
equals(INSTALL_THEME_ICONS,"no") {
	DEFINES += LOAD_QALCULATEICONS_FROM_FILE=1
}
unix:!equals(COMPILE_RESOURCES,"yes"):!android:!macx {
	isEmpty(ICON_DIR) {
		equals(INSTALL_THEME_ICONS,"no") {
			ICON_DIR = $$PREFIX/share/qalculate-qt/icons
		} else {
			ICON_DIR = $$PREFIX/share/icons
		}
	}
	isEmpty(TRANSLATIONS_DIR) {
		TRANSLATIONS_DIR = $$PREFIX/share/eqonomize/translations
	}
} else {
	ICON_DIR = ":/icons"
	TRANSLATIONS_DIR = ":/translations"
	DEFINES += RESOURCES_COMPILED=1
}
isEmpty(MAN_DIR) {
	MAN_DIR = $$PREFIX/share/man
}
TEMPLATE = app
TARGET = qalculate-qt
INCLUDEPATH += src  
INCLUDEPATH += /usr/include
INCLUDEPATH += /usr/local/include
LIBS += -L/usr/lib -L/usr/local/lib -lqalculate
CONFIG += qt
QT += widgets network
MOC_DIR = build
OBJECTS_DIR = build
DEFINES += TRANSLATIONS_DIR=\\\"$$TRANSLATIONS_DIR\\\"
DEFINES += ICON_DIR=\\\"$$ICON_DIR\\\"
DEFINES += VERSION=\\\"$$VERSION\\\"

HEADERS += src/expressionedit.h src/fpconversiondialog.h src/functioneditdialog.h src/functionsdialog.h src/historyview.h src/keypadwidget.h src/preferencesdialog.h src/qalculateqtsettings.h src/qalculatewindow.h src/variableeditdialog.h
SOURCES += src/expressionedit.cpp src/fpconversiondialog.cpp src/functioneditdialog.cpp src/functionsdialog.cpp src/historyview.cpp src/keypadwidget.cpp src/main.cpp src/preferencesdialog.cpp src/qalculateqtsettings.cpp src/qalculatewindow.cpp src/variableeditdialog.cpp

unix:!equals(COMPILE_RESOURCES,"yes"):!android:!macx {

	target.path = $$PREFIX/bin
		
	desktop.files = data/qalculate-qt.desktop
	desktop.path = $$DESKTOP_DIR

	appicon16.files = data/16/qalculate-qt.png
	appicon16.path = $$ICON_DIR/hicolor/16x16/apps
	appicon22.files = data/22/qalculate-qt.png
	appicon22.path = $$ICON_DIR/hicolor/22x22/apps
	appicon32.files = data/32/qalculate-qt.png
	appicon32.path = $$ICON_DIR/hicolor/32x32/apps
	appicon64.files = data/64/qalculate-qt.png
	appicon64.path = $$ICON_DIR/hicolor/64x64/apps
	appicon128.files = data/128/qalculate-qt.png
	appicon128.path = $$ICON_DIR/hicolor/128x128/apps
	appiconsvg.files = data/scalable/qalculate-qt.svg
	appiconsvg.path = $$ICON_DIR/hicolor/scalable/apps

	INSTALLS += 	target desktop \ 
			appicon16 appicon22 appicon32 appicon64 appicon128 appiconsvg
			
	!equals($$DESKTOP_ICON_DIR, $$ICON_DIR) {
		desktopappicon64.files = data/64/qalculate-qt.png
		desktopappicon64.path = $$DESKTOP_ICON_DIR/hicolor/64x64/apps
		INSTALLS += desktopappicon64
	}
	RESOURCES = icons.qrc flags.qrc
} else {
	RESOURCES = icons.qrc flags.qrc
	target.path = $$PREFIX/bin
	desktop.files = data/qalculate-qt.desktop
	desktop.path = $$DESKTOP_DIR
	appicon64.files = data/64/qalculate-qt.png
	appicon64.path = $$DESKTOP_ICON_DIR/hicolor/64x64/apps
	INSTALLS += target desktop appicon64
}

win32: RC_FILE = winicon.rc

