#ifndef FORCASTWIDGET_H
#define FORCASTWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QSettings>

class ForcastWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ForcastWidget(QWidget *parent = 0);

private:
    QVBoxLayout *m_centralLayout;
    QWidget *m_centralWidget;
    static const int FORECAST_DAYS = 5;
    static const int CURRENT_WEATHER_INDEX = 0;

    QSettings m_settings;
    QLabel *labelWImg[FORECAST_DAYS + 1];
    QLabel *labelTemp[FORECAST_DAYS + 1];
    QLabel *labelDate[FORECAST_DAYS + 1];
    QLabel *labelHumidity;
    QLabel *labelSunset;
    QLabel *labelWind;
    QLabel *labelUpdateTime;
    QLabel *labelCity;

    QString getIconPath(const QString &iconName);
    void setupUI();
    QLayout* setupCurrentWeather();
    QLayout* setupForecast();


public slots:
    void updateWeather();

signals:
    void weatherNow(QString weather, QString temp, QString stip, QPixmap pixmap);

};

#endif // FORCASTWIDGET_H
