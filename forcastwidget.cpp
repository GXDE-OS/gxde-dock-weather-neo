#include "forcastwidget.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QFile>
#include <QStandardPaths>
#include <QTimeZone>
#include <QJsonDocument>
#include <QString>
#include <QDebug>

QJsonObject loadCityTranslations()
{
    // Open the file from Qt resource
    QFile file(":/city.json");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << file.errorString();
        return QJsonObject();
    }

    // Read the file content
    QByteArray jsonData = file.readAll();
    file.close();

    // Parse JSON
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "JSON parse error:" << parseError.errorString();
        return QJsonObject();
    }

    // Return the JSON object
    return jsonDoc.object();
}

ForcastWidget::ForcastWidget(QWidget *parent)
    : QWidget(parent),
      m_settings("deepin", "dde-dock-HTYWeather")
{
    setupUI();
}

void ForcastWidget::setupUI()
{
//    setFixedWidth(300);
    setStyleSheet(" border-radius: 15px;");

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);

    QLayout *currentWeatherLayout = setupCurrentWeather();
    mainLayout->addLayout(currentWeatherLayout);

    // Add separator line
    QFrame *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: rgba(255,255,255,0.3);");
    mainLayout->addWidget(line);

    QLayout *forecastLayout = setupForecast();

    mainLayout->addLayout(forecastLayout);
    setLayout(mainLayout);
}

QLayout* ForcastWidget::setupCurrentWeather()
{
    QGridLayout *currentWeatherLayout = new QGridLayout;

    labelWImg[0] = new QLabel;
    labelTemp[0] = new QLabel;
    labelDate[0] = new QLabel;

    QString icon_path = getIconPath("na");
    QPixmap pixmap(icon_path);
    labelWImg[0]->setPixmap(pixmap.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    labelWImg[0]->setFixedSize(90, 90);
    labelWImg[0]->setAlignment(Qt::AlignCenter);
    labelWImg[0]->setStyleSheet("border-radius: 10px;");

    labelTemp[0]->setText("多云 25°C");
    labelTemp[0]->setStyleSheet("color: white; font-size: 32px; font-weight: bold;");
    labelTemp[0]->setAlignment(Qt::AlignCenter);

    labelDate[0]->setText("2024/10/12");
    labelDate[0]->setStyleSheet("color: white; font-size: 48px;");

    labelCity = new  QLabel;
    labelCity->setStyleSheet("color: white; font-size: 48px;");


//    QVBoxLayout *imgAndTempLayout = new QVBoxLayout;
//    imgAndTempLayout->addWidget(labelWImg[0]);
//    imgAndTempLayout->addWidget(labelTemp[0]);

//    QHBoxLayout *currentInfoLayout = new QHBoxLayout;
//    currentInfoLayout->addWidget(labelDate[CURRENT_WEATHER_INDEX]);
//    currentInfoLayout->addWidget(labelTemp[CURRENT_WEATHER_INDEX]);
//    currentInfoLayout->setAlignment(Qt::AlignVCenter);

//    labelHumidity = new QLabel(this);
//    labelSunset = new QLabel(this);
//    labelWind = new QLabel(this);
//    labelUpdateTime = new QLabel(this);

//    QString infoLabelStyle = "color: white; font-size: 12px;";
//    labelHumidity->setStyleSheet(infoLabelStyle);
//    labelSunset->setStyleSheet(infoLabelStyle);
//    labelWind->setStyleSheet(infoLabelStyle);
//    labelUpdateTime->setStyleSheet(infoLabelStyle);



    // 添加原有的部件
    currentWeatherLayout->addWidget(labelCity, 0, 0);
    currentWeatherLayout->addWidget(labelDate[0], 1, 0);
    currentWeatherLayout->addWidget(labelWImg[0], 0, 1);
    currentWeatherLayout->addWidget(labelTemp[0], 1, 1);

    // 创建并添加一个水平空间
    QSpacerItem *horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    currentWeatherLayout->addItem(horizontalSpacer, 0, 2, 2, 1);  // 跨越两行

    // 设置列拉伸因子，使空间吸收多余的水平空间
    currentWeatherLayout->setColumnStretch(0, 0);  // 第一列不拉伸
    currentWeatherLayout->setColumnStretch(1, 0);  // 第二列不拉伸
    currentWeatherLayout->setColumnStretch(2, 1);  // 第三列（spacer）拉伸

    // 可选：调整部件之间的间距
    currentWeatherLayout->setHorizontalSpacing(30);
    currentWeatherLayout->setVerticalSpacing(5);

    // 设置布局的对齐方式
    currentWeatherLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

//    currentWeatherLayout->addWidget(labelHumidity, 0, 2);
//    currentWeatherLayout->addWidget(labelSunset, 0, 2);
//    currentWeatherLayout->addWidget(labelWind, 1, 3);
//    currentWeatherLayout->addWidget(labelUpdateTime, 1, 3);


    return currentWeatherLayout;
}


QLayout* ForcastWidget::setupForecast()
{
    QLayout *forecastLayout = new QHBoxLayout;

    for (int i = 1; i <= FORECAST_DAYS; i++) {

        QVBoxLayout *itemLayout = new QVBoxLayout;
        labelWImg[i] = new QLabel;
        labelTemp[i] = new QLabel;
        labelDate[i] = new QLabel;

        QString icon_path = getIconPath("na");
        QPixmap pixmap(icon_path);
        labelWImg[i]->setPixmap(pixmap.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        labelWImg[i]->setFixedSize(50, 50);
        labelWImg[i]->setAlignment(Qt::AlignLeft);
        labelWImg[i]->setStyleSheet("border-radius: 5px;");

        labelTemp[i]->setText("25°C");
        labelTemp[i]->setStyleSheet("color: white; font-size: 16px;");
        labelTemp[i]->setAlignment(Qt::AlignVCenter);

        labelDate[i]->setText("01-01 Mon");
        labelDate[i]->setStyleSheet("color: rgba(255,255,255,0.8); font-size: 14px;");
        labelDate[i]->setAlignment(Qt::AlignVCenter);

        itemLayout->addWidget(labelDate[i]);
        itemLayout->addWidget(labelWImg[i]);
        itemLayout->addWidget(labelTemp[i]);
        itemLayout->addStretch();
        itemLayout->setAlignment(Qt::AlignVCenter);

        // 创建一个 QWidget 作为容器
        QWidget *containerWidget = new QWidget;
        containerWidget->setLayout(itemLayout);

        forecastLayout->addWidget(containerWidget);
    }
    return forecastLayout;
}



QString ForcastWidget::getIconPath(const QString &iconName)
{
    QString icon_path = ":icon/Default/" + iconName + ".png";
    QString iconTheme = m_settings.value("IconTheme","").toString();
    if (!iconTheme.isEmpty()) {
        if (!iconTheme.startsWith("/")) {
            icon_path = ":icon/" + iconTheme + "/" + iconName + ".png";
        } else {
            QString icon_path1 = iconTheme + "/" + iconName + ".png";
            QFile file(icon_path1);
            if (file.exists()) {
                icon_path = icon_path1;
            }
        }
    }
    return icon_path;
}

// 更新天气信息函数
void ForcastWidget::updateWeather()
{
    QDateTime currentDateTime = QDateTime::currentDateTime();  // 获取当前时间
    QString stemp = "", stip = "", surl="";
    QString log = currentDateTime.toString("yyyy/MM/dd HH:mm:ss") + "\n";  // 记录日志信息
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply;

    // 从设置中读取城市和国家信息
    QString city = m_settings.value("city","").toString();
    QString country = m_settings.value("country","").toString();

    if(city != "" && country != ""){
        // 设置默认图标路径
        QString icon_path = ":icon/Default/na.png";
        QString iconTheme = m_settings.value("IconTheme","").toString();
        if(iconTheme != ""){
            if(!iconTheme.startsWith("/")){
                icon_path = ":icon/" + iconTheme + "/na.png";
            }else{
                QString icon_path1 = iconTheme + "/na.png";
                QFile file(icon_path1);
                if(file.exists()){
                    icon_path = icon_path1;
                }
            }
        }

        // 发送信号显示当前天气信息
        emit weatherNow("Weather", "Temp", currentDateTime.toString("yyyy/MM/dd HH:mm:ss") + "\nGetting weather of " + city + "," + country, QPixmap(icon_path));

        // 构建OpenWeatherMap API的URL
        QString appid = "8f3c852b69f0417fac76cd52c894ba63";
        surl = "https://api.openweathermap.org/data/2.5/forecast?q=" + city + "," + country + "&appid=" + appid + "&lang=zh_cn";

        // 发送网络请求
        reply = manager.get(QNetworkRequest(QUrl(surl)));
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
        loop.exec();  // 等待请求完成

        // 读取回复数据
        QByteArray BA = reply->readAll();
        log += surl + "\n";
        log += BA + "\n";

        QJsonParseError JPE;
        QJsonDocument JD = QJsonDocument::fromJson(BA, &JPE);  // 解析JSON数据
        QJsonObject cityTranslations = loadCityTranslations();

        if (JPE.error == QJsonParseError::NoError) {  // 确保解析无错误
            QString cod = JD.object().value("cod").toString();
            if(cod == "200"){
                // 提取城市和天气数据
                QJsonObject JO_city = JD.object().value("city").toObject();
                QJsonObject coord = JO_city.value("coord").toObject();
                double lat = coord.value("lat").toDouble();
                double lon = coord.value("lon").toDouble();
                m_settings.setValue("lat", lat);
                m_settings.setValue("lon", lon);

                // 提取日出和日落时间
                QDateTime time_sunrise = QDateTime::fromMSecsSinceEpoch(JO_city.value("sunrise").toInt()*1000L, Qt::LocalTime);
                QDateTime time_sunset = QDateTime::fromMSecsSinceEpoch(JO_city.value("sunset").toInt()*1000L, Qt::LocalTime);

                QJsonArray list = JD.object().value("list").toArray();  // 获取天气列表数据
                int r = 0;

                // 遍历天气预报数据
                for (int i=0; i<list.size(); i++) {
                    QDateTime date = QDateTime::fromMSecsSinceEpoch(list[i].toObject().value("dt").toInt()*1000L, Qt::UTC);
                    QString sdate = date.toString("MM-dd ddd");  // 格式化日期
                    QString today = date.toString("MM-dd");  // 格式化日期
                    QString dt_txt = list[i].toObject().value("dt_txt").toString();
                    double temp = list[i].toObject().value("main").toObject().value("temp").toDouble() - 273.15;  // 转换温度为摄氏度
                    stemp = QString::number(qRound(temp)) + "°C";

                    // 检查是否需要将温度单位转换为华氏度
                    if(m_settings.value("TemperatureUnit","°C").toString() == "°F"){
                        stemp = QString::number(qRound(temp*1.8 + 32)) + "°F";
                    }

                    // 提取其他天气信息
                    QString humidity = "湿度: " + QString::number(list[i].toObject().value("main").toObject().value("humidity").toInt()) + "%";
                    QString weather = list[i].toObject().value("weather").toArray().at(0).toObject().value("description").toString();
                    QString icon_name = list[i].toObject().value("weather").toArray().at(0).toObject().value("icon").toString() + ".png";
                    QString icon_path = ":icon/Default/" + icon_name;

                    // 根据图标主题设置图标路径
                    QString iconTheme = m_settings.value("IconTheme","").toString();
                    if(iconTheme != ""){
                        if(!iconTheme.startsWith("/")){
                            icon_path = ":icon/" + iconTheme + "/" + icon_name;
                        }else{
                            QString icon_path1 = iconTheme + "/" + icon_name;
                            QFile file(icon_path1);
                            if(file.exists()){
                                icon_path = icon_path1;
                            }
                        }
                    }

                    // 提取风速和风向
                    QString wind = "风: " + QString::number(list[i].toObject().value("wind").toObject().value("speed").toDouble()) + "m/s, " + QString::number(qRound(list[i].toObject().value("wind").toObject().value("deg").toDouble())) + "°";

                    log +=     + ", " + date.toString("yyyy-MM-dd HH:mm:ss ddd") + ", " + stemp + ", " + humidity + ","+ weather + ", " + icon_path + ", " + wind + "\n";

                    // 仅更新每天中午的天气数据
                    if(date.time() == QTime(12,0,0)){
                        if (r == 0) {
                            // 更新主天气信息
                            QPixmap pixmap(icon_path);
                            labelWImg[0]->setPixmap(pixmap.scaled(80,80,Qt::KeepAspectRatio,Qt::SmoothTransformation));
                            labelTemp[0]->setText(weather + " " + stemp);
                            labelDate[0]->setText(today);

                            labelCity->setText(cityTranslations[JO_city.value("name").toString()].toString());

//                            // 更新新添加的信息
//                            labelHumidity->setText("湿度: " + humidity);
//                            labelSunset->setText("日落: " + time_sunset.toString("hh:mm"));
//                            labelWind->setText(wind);
//                            labelUpdateTime->setText("更新: " + currentDateTime.toString("HH:mm:ss"));

                            // 更新未来天气预报
                            labelWImg[1]->setPixmap(QPixmap(icon_path).scaled(50,50,Qt::KeepAspectRatio,Qt::SmoothTransformation));
                            labelTemp[1]->setText(weather + " " + stemp);
                            labelDate[1]->setText(sdate);

                            // 构建显示信息
                            stip = city + ", " + country + "\n" + weather + "\n" + stemp + "\n" + humidity + "\n" + wind + "\n日升: " + time_sunrise.toString("hh:mm") + "\n日落: " + time_sunset.toString("hh:mm") + "\n刷新：" + currentDateTime.toString("HH:mm:ss");

                            emit weatherNow(weather, stemp, stip, pixmap);
                            r++;
                        } else {
                            // 更新剩余天气信息
                            labelWImg[r]->setPixmap(QPixmap(icon_path).scaled(50,50,Qt::KeepAspectRatio,Qt::SmoothTransformation));
                            labelTemp[r]->setText(weather + " " + stemp);
                            labelDate[r]->setText(sdate);
                        }
                        r++;
                    }
                }
            } else {
                // 如果返回的状态码不是200，显示错误信息
                emit weatherNow("Weather", "Temp", city + ", " + country + "\n" + cod + "\n" + JD.object().value("message").toString(), QPixmap(":icon/na.png"));
            }
        }else{
            // 如果JSON解析出错，显示原始响应内容
            emit weatherNow("Weather", "Temp", QString(BA), QPixmap(":icon/na.png"));
        }

        // 将日志写入文件
        QString path = QStandardPaths::standardLocations(QStandardPaths::CacheLocation).first() + "/HTYWeather.log";
        QFile file(path);
        if (file.open(QFile::WriteOnly)) {
            file.write(log.toUtf8());
            file.close();
        }
    }
}
