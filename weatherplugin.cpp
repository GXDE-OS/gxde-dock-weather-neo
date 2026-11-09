#include "weatherplugin.h"
#include <QApplication>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDesktopServices>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QAction>
#include <QFileDialog>
#include <QRadioButton>
#include <QButtonGroup>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QTranslator>
#include <QLocale>
#include <QFile>
#include <QCoreApplication>

namespace {

// The .qm files are compiled into the plugin's resource by qmake (see weather.pro),
// so the plugin also works when it is installed as a single .so file.
void installPluginTranslator()
{
    static bool installed = false;
    if (installed)
        return;
    installed = true;

    const QString locale = QLocale::system().name();
    QStringList candidates;
    candidates << locale;
    if (locale.contains(QLatin1Char('_')))
        candidates << locale.section(QLatin1Char('_'), 0, 0);

    // Sources are written in English, so every non-Chinese locale falls back to it.
    candidates << (locale.startsWith(QLatin1String("zh"), Qt::CaseInsensitive)
                       ? QStringLiteral("zh_CN") : QStringLiteral("en_US"));

    QTranslator *translator = new QTranslator(qApp);
    for (const QString &name : candidates) {
        const QString path = QStringLiteral(":/HTYWeather/i18n/%1.qm").arg(name);
        if (QFile::exists(path) && translator->load(path)) {
            qApp->installTranslator(translator);
            return;
        }
    }
    delete translator;
}

}

WeatherPlugin::WeatherPlugin(QObject *parent)
    : QObject(parent),
      m_tipsLabel(new QLabel),
      m_refershTimer(new QTimer(this)),
      m_settings("deepin", "gxde-dock-HTYWeather")
{
    installPluginTranslator();

    m_tipsLabel->setObjectName("HTYWeather");
    m_tipsLabel->setStyleSheet("color:white; padding:0px 3px;");

    m_centralWidget = new WeatherWidget;
    connect(m_centralWidget, &WeatherWidget::requestUpdateGeometry, [this] { m_proxyInter->itemUpdate(this, pluginName()); });

    forcastApplet = new ForcastWidget;
    forcastApplet->setObjectName("forcast");
    forcastApplet->setVisible(false);
    connect(forcastApplet, SIGNAL(weatherNow(QString,QString,QString,QPixmap)), this, SLOT(weatherNow(QString,QString,QString,QPixmap)));
    forcastApplet->updateWeather();

    m_refershTimer->setInterval(3600000);
    m_refershTimer->start();
    connect(m_refershTimer, &QTimer::timeout, forcastApplet, &ForcastWidget::updateWeather);

}

const QString WeatherPlugin::pluginName() const
{
    return "HTYWeather";
}

const QString WeatherPlugin::pluginDisplayName() const
{
    return tr("Weather");
}

void WeatherPlugin::init(PluginProxyInterface *proxyInter)
{
    m_proxyInter = proxyInter;
    if (m_centralWidget->enabled())
        m_proxyInter->itemAdded(this, pluginName());
}

void WeatherPlugin::pluginStateSwitched()
{
    m_centralWidget->setEnabled(!m_centralWidget->enabled());
    if (m_centralWidget->enabled())
        m_proxyInter->itemAdded(this, pluginName());
    else
        m_proxyInter->itemRemoved(this, pluginName());
}

bool WeatherPlugin::pluginIsDisable()
{
    return !m_centralWidget->enabled();
}

int WeatherPlugin::itemSortKey(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    const QString key = QString("pos_%1").arg(displayMode());
    return m_settings.value(key, 0).toInt();
}

void WeatherPlugin::setSortKey(const QString &itemKey, const int order)
{
    Q_UNUSED(itemKey);

    const QString key = QString("pos_%1").arg(displayMode());
    m_settings.setValue(key, order);
}

QWidget *WeatherPlugin::itemWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return m_centralWidget;
}

QWidget *WeatherPlugin::itemTipsWidget(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return m_tipsLabel;
}

QWidget *WeatherPlugin::itemPopupApplet(const QString &itemKey)
{
    Q_UNUSED(itemKey);
    return forcastApplet;
}

const QString WeatherPlugin::itemContextMenu(const QString &itemKey)
{
    Q_UNUSED(itemKey);

    QList<QVariant> items;
    items.reserve(0);

    QMap<QString, QVariant> about;
    about["itemId"] = "about";
    about["itemText"] = tr("About");
    about["isActive"] = true;
    items.push_back(about);

    QMap<QString, QVariant> set;
    set["itemId"] = "set";
    set["itemText"] = tr("Settings");
    set["isActive"] = true;
    items.push_back(set);

    QMap<QString, QVariant> refresh;
    refresh["itemId"] = "refresh";
    refresh["itemText"] = tr("Refresh");
    refresh["isActive"] = true;
    items.push_back(refresh);


    QMap<QString, QVariant> log;
    log["itemId"] = "log";
    log["itemText"] = tr("Log");
    log["isActive"] = true;
    items.push_back(log);

    QMap<QString, QVariant> menu;
    menu["items"] = items;
    menu["checkableMenu"] = false;
    menu["singleCheck"] = false;
    return QJsonDocument::fromVariant(menu).toJson();
}

void WeatherPlugin::invokedMenuItem(const QString &itemKey, const QString &menuId, const bool checked)
{
    Q_UNUSED(itemKey);
    Q_UNUSED(checked);

    if(menuId == "about"){
        MBAbout();
    }else if(menuId == "set"){
        set();
    }else if(menuId == "refresh"){
        forcastApplet->updateWeather();
        m_refershTimer->start();
    }else if(menuId == "log"){
        showLog();
    }

}

void WeatherPlugin::MBAbout()
{
    const QString aboutText = tr("GXDE Dock Weather Plugin") + "\n\n"
            + tr("Version: %1").arg("5.8") + "\n"
            + tr("Author: %1").arg("海天鹰") + "\n"
            + tr("E-mail: %1").arg("sonichy@163.com") + "\n"
            + tr("Source: %1").arg("https://github.com/sonichy/WEATHER_DDE_DOCK") + "\n"
            + tr("API: %1").arg("https://openweathermap.org/forecast5");
    QMessageBox aboutMB(QMessageBox::NoIcon, tr("About HTYWeather"), aboutText);
    aboutMB.setIconPixmap(QPixmap(":/icon/Default/01d.png"));
    aboutMB.exec();
}

void WeatherPlugin::weatherNow(QString weather, QString temp, QString stip, QPixmap pixmap)
{
    m_centralWidget->sw = weather;
    m_centralWidget->temp = temp;
    m_centralWidget->pixmap = pixmap;
    m_centralWidget->update();
    m_tipsLabel->setText(stip);
}


void WeatherPlugin::showLog()
{
    QString surl = "file://" + QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/HTYWeather.log";
    QDesktopServices::openUrl(QUrl(surl));
}

void WeatherPlugin::set()
{
    QDialog *dialog = new QDialog;
    dialog->setWindowTitle(tr("Settings"));
    dialog->setFixedSize(350, 250);
    QVBoxLayout *vbox = new QVBoxLayout;

    // 添加单选按钮组
    QButtonGroup *locationGroup = new QButtonGroup(dialog);
    QRadioButton *manualRadio = new QRadioButton(tr("Set city manually"));
    QRadioButton *autoRadio = new QRadioButton(tr("Locate city by IP"));
    locationGroup->addButton(manualRadio);
    locationGroup->addButton(autoRadio);
    vbox->addWidget(manualRadio);
    vbox->addWidget(autoRadio);

    QHBoxLayout *hbox = new QHBoxLayout;
    QLabel *label = new QLabel(tr("City"));
    hbox->addWidget(label);
    QLineEdit *lineEdit_city = new QLineEdit;
    lineEdit_city->setPlaceholderText(tr("English only"));
    QRegularExpression regExp("[a-zA-Z ]+$");
    QValidator *validator = new QRegularExpressionValidator(regExp, lineEdit_city);
    lineEdit_city->setValidator(validator);
    lineEdit_city->setText(m_settings.value("city","").toString());
    hbox->addWidget(lineEdit_city);
    label = new QLabel(tr("Country"));
    hbox->addWidget(label);
    QComboBox *comboBox_country = new QComboBox;
    QString country_codes = "AF,AX,AL,DZ,AS,AD,AO,AI,AQ,AG,AR,AM,AW,AU,AT,AZ,BS,BH,BD,BB,BY,BE,BZ,BJ,BM,BT,BO,BQ,BA,BW,BV,BR,IO,BN,BG,BF,BI,KH,CM,CA,CV,KY,CF,TD,CL,CN,CX,CC,CO,KM,CD,CG,CK,CR,CI,HR,CU,CW,CY,CZ,DK,DJ,DM,DO,EC,EG,SV,GQ,ER,EE,ET,FK,FO,FJ,FI,FR,GF,PF,TF,GA,GM,GE,DE,GH,GI,GR,GL,GD,GP,GU,GT,GG,GW,GN,GY,HT,HM,VA,HN,HK,HU,IS,IN,ID,IR,IQ,IE,IM,IL,IT,JM,JP,JE,JO,KZ,KE,KI,KP,KR,KW,KG,LA,LV,LB,LS,LR,LY,LI,LT,LU,MO,MK,MG,MW,MY,MV,ML,MT,MH,MQ,MR,MU,YT,MX,FM,MD,MC,MN,ME,MS,MA,MZ,MM,NA,NR,NP,NL,NC,NZ,NI,NG,NE,NU,NF,MP,NO,OM,PK,PW,PS,PA,PG,PY,PE,PH,PN,PL,PT,PR,QA,RE,RO,RU,RW,BL,SH,KN,LC,MF,PM,VC,WS,SM,ST,SA,SN,RS,SC,SL,SG,SX,SK,SI,SB,SO,ZA,GS,SS,ES,LK,SD,SR,SJ,SZ,SE,CH,SY,TW,TJ,TZ,TH,TL,TG,TK,TO,TT,TN,TR,TM,TC,TV,UG,UA,AE,GB,UM,US,UY,UZ,VU,VE,VN,VG,VI,WF,EH,YE,ZM,ZW";
    QStringList SL;
    SL = country_codes.split(",");
    SL.sort();
    comboBox_country->addItems(SL);
    comboBox_country->setCurrentText(m_settings.value("country","").toString());
    hbox->addWidget(comboBox_country);
    vbox->addLayout(hbox);

    // 连接单选按钮信号到槽函数
    connect(manualRadio, &QRadioButton::toggled, [=](bool checked){
        lineEdit_city->setEnabled(checked);
        comboBox_country->setEnabled(checked);
    });

    // 设置初始状态
    bool isManual = m_settings.value("isManualLocation", true).toBool();
    manualRadio->setChecked(isManual);
    autoRadio->setChecked(!isManual);
    lineEdit_city->setEnabled(isManual);
    comboBox_country->setEnabled(isManual);

    hbox = new QHBoxLayout;
    label = new QLabel(tr("Search your city and country in <a style='color:white;' href='https://openweathermap.org'>openweathermap.org</a>"));
    connect(label, &QLabel::linkActivated, [](QString url){
        QDesktopServices::openUrl(QUrl(url));
    });
    hbox->addWidget(label);
    vbox->addLayout(hbox);
    hbox = new QHBoxLayout;
    label = new QLabel(tr("Icon theme (PNG only)"));
    hbox->addWidget(label);
    QComboBox *comboBox_iconTheme = new QComboBox;
    comboBox_iconTheme->addItem(QIcon(":icon/Default/01d.png"), "Default");
    comboBox_iconTheme->addItem(QIcon(":icon/Simple/01d.png"), "Simple");
    comboBox_iconTheme->addItem(QIcon(":icon/Flat/01d.png"), "Flat");
    QString iconTheme = m_settings.value("IconTheme","").toString();
    if(iconTheme == "" || !iconTheme.startsWith("/")){
        comboBox_iconTheme->addItem(QIcon(":icon/custom.png"), tr("Custom"));
    }else{
        comboBox_iconTheme->addItem(QIcon(iconTheme + "/01d.png"), iconTheme);
    }
    comboBox_iconTheme->setCurrentText(iconTheme);
    connect(comboBox_iconTheme, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), [=](int index){
        if(index == 3){
            QString siconTheme = QFileDialog::getExistingDirectory(dialog, tr("Select icon theme"), iconTheme);
            if(siconTheme != ""){
                QString icon_path = siconTheme + "/01d.png";
                QFile file(icon_path);
                if(file.exists()){
                    comboBox_iconTheme->setItemText(index, siconTheme);
                    comboBox_iconTheme->setItemIcon(index, QIcon(icon_path));
                }else{
                    QMessageBox MB(QMessageBox::Critical, tr("Error"), tr("The icon file %1 does not exist!").arg(icon_path));
                    MB.exec();
                }
            }
        }
    });
    hbox->addWidget(comboBox_iconTheme);
    vbox->addLayout(hbox);
    hbox = new QHBoxLayout;
    label = new QLabel(tr("Temperature unit"));
    hbox->addWidget(label);
    QComboBox *comboBox_TU = new QComboBox;
    comboBox_TU->addItem("°C");
    comboBox_TU->addItem("°F");
    comboBox_TU->setCurrentText(m_settings.value("TemperatureUnit","°C").toString());
    hbox->addWidget(comboBox_TU);
    vbox->addLayout(hbox);

    QPushButton *pushButton_confirm = new QPushButton(tr("OK"));
    QPushButton *pushButton_cancel = new QPushButton(tr("Cancel"));
    connect(pushButton_confirm, SIGNAL(clicked()), dialog, SLOT(accept()));
    connect(pushButton_cancel, SIGNAL(clicked()), dialog, SLOT(reject()));
    hbox = new QHBoxLayout;
    hbox->addStretch();
    hbox->addWidget(pushButton_confirm);
    hbox->addWidget(pushButton_cancel);
    hbox->addStretch();
    vbox->addLayout(hbox);
    dialog->setLayout(vbox);

    if(dialog->exec() == QDialog::Accepted){
        m_settings.setValue("isManualLocation", manualRadio->isChecked());
        if (manualRadio->isChecked()) {
            m_settings.setValue("city", lineEdit_city->text());
            m_settings.setValue("country", comboBox_country->currentText());
        } else {
            // 调用自动IP定位函数
//            autoLocateCity();

            this->autoLocateCity();
        }
        m_settings.setValue("IconTheme", comboBox_iconTheme->currentText());
        m_settings.setValue("TemperatureUnit", comboBox_TU->currentText());
        forcastApplet->updateWeather();
    }
    dialog->close();
}

// 新增自动IP定位函数
void WeatherPlugin::autoLocateCity()
{
    QDateTime currentDateTime = QDateTime::currentDateTime();
    QString log = "IP:" + currentDateTime.toString("yyyy/MM/dd HH:mm:ss") + "\n";

    // 这里需要实现IP定位逻辑
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, [&log, manager, this](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NoError) {
            log += "NoError";
            QString result = reply->readAll();
            // 解析结果，获取城市和国家信息
            // 这里假设返回的是JSON格式，包含city和country字段
            QJsonDocument doc = QJsonDocument::fromJson(result.toUtf8());
            QJsonObject obj = doc.object();
            QString city = obj["city"].toString();
            QString country = obj["countryCode"].toString();

            m_settings.setValue("city", city);
            m_settings.setValue("country", country);
            log += "IP::city: " + city + " country: " + country;
            forcastApplet->updateWeather();
        }
        else{
            QString result = reply->readAll();
            log += "IP::error: " + result;

        }
        reply->deleteLater();
        manager->deleteLater();
    });
    QEventLoop loop;
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl("http://ip-api.com/json/")));
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();  // 等待请求完成

    // 将日志写入文件
    QString path = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first() + "/HTYWeather.log";
    QFile file(path);
    if (file.open(QFile::WriteOnly)) {
        file.write(log.toUtf8());
        file.close();
    }
}
