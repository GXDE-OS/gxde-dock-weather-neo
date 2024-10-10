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

// 构造函数，初始化天气预报窗口部件
ForcastWidget::ForcastWidget(QWidget *parent)
    : QWidget(parent),
      m_settings("deepin", "dde-dock-HTYWeather")  // 初始化设置对象，用于存储用户设置
{
    setFixedWidth(300);  // 设置窗口宽度
    QGridLayout *layout = new QGridLayout;  // 创建网格布局
    for (int i=0; i<6; i++) {
        labelWImg[i] = new QLabel;  // 创建标签用于显示天气图标

        // 设置默认天气图标
        QString icon_path = ":icon/Default/na.png";
        QString iconTheme = m_settings.value("IconTheme","").toString();
        if(iconTheme != ""){
            // 如果设置了图标主题，检查图标路径是否存在
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

        // 设置图标大小并添加到布局中
        labelWImg[i]->setPixmap(QPixmap(icon_path).scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        labelWImg[i]->setAlignment(Qt::AlignCenter);
        layout->addWidget(labelWImg[i],i,0);

        // 创建用于显示温度的标签
        labelTemp[i] = new QLabel("25°C");
        labelTemp[i]->setAlignment(Qt::AlignCenter);
        layout->addWidget(labelTemp[i],i,1);

        // 设置第一个元素的样式
        if (i==0) {
            labelTemp[i]->setStyleSheet("color:white;font-size:20px;");
            labelDate[i] = new QLabel("City");  // 用于显示城市名称
            labelDate[i]->setStyleSheet("color:white;font-size:20px;");
        } else {
            // 其他元素的样式
            labelTemp[i]->setStyleSheet("color:white;font-size:12px;");
            labelDate[i] = new QLabel("01-01 Mon");  // 用于显示日期
            labelDate[i]->setStyleSheet("color:white;font-size:12px;");
        }

        labelDate[i]->setAlignment(Qt::AlignCenter);
        layout->addWidget(labelDate[i],i,2);
    }

    setLayout(layout);  // 应用布局到窗口部件
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
                            labelTemp[0]->setText(stemp);

                            labelDate[0]->setText(cityTranslations[JO_city.value("name").toString()].toString());

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
