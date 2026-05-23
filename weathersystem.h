#ifndef WEATHERSYSTEM_H
#define WEATHERSYSTEM_H

#include <QObject>
#include <QElapsedTimer>
#include "GameConfig.h"

enum class WeatherType { SUNNY, FOG, STORM };

class WeatherSystem : public QObject
{
    Q_OBJECT
public:
    static WeatherSystem& instance();
    void update(qreal deltaTime);
    void reset();
    WeatherType currentWeather() const { return m_currentWeather; }
    qreal currentVisionMultiplier() const;
    qreal currentFishValueBonus() const;
    bool shouldTriggerLightning() const;
    QColor overlayColor() const;

signals:
    void weatherChanged(WeatherType newWeather);

private:
    WeatherSystem();
    void switchWeather();
    WeatherType m_currentWeather;
    int m_weatherFrameCount;
    int m_weatherDurationFrames;
    mutable bool m_lightningTriggered;
};

#endif // WEATHERSYSTEM_H