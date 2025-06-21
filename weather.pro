#include(../gxde-dock/interfaces/interfaces.pri)

QT              += widgets network #svg
TEMPLATE         = lib
CONFIG          += plugin c++11 link_pkgconfig
PKGCONFIG       += dtkwidget

TARGET           = HTYWeather
DISTFILES       += weather.json

HEADERS += \
    weatherplugin.h \
    weatherwidget.h \
    forcastwidget.h

SOURCES += \
    weatherplugin.cpp \
    weatherwidget.cpp \
    forcastwidget.cpp

target.path = $${PREFIX}/lib/gxde-dock/plugins/
INSTALLS += target

RESOURCES += res.qrc