#include(../gxde-dock/interfaces/interfaces.pri)

QT              += widgets network #svg
TEMPLATE         = lib
CONFIG          += plugin c++17 link_pkgconfig
PKGCONFIG       += dtk2widget

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

# Sources are written in English; the .qm files are generated with lrelease
# at build time and embedded into the plugin under :/HTYWeather/i18n/.
TRANSLATIONS += \
    translations/zh_CN.ts \
    translations/en_US.ts

QM_FILES_RESOURCE_PREFIX = HTYWeather/i18n
CONFIG += lrelease embed_translations
