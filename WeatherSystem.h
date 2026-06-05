#ifndef WEATHERSYSTEM_H
#define WEATHERSYSTEM_H

#include <QObject>
#include <QElapsedTimer>
#include "GameConfig.h"
#include <QColor>

enum class WeatherType { SUNNY, FOG, STORM };

class WeatherSystem : public QObject
{
    Q_OBJECT
public:
    static WeatherSystem& instance();
    void update(qreal deltaTime);
    void configureStage(int sunnyWeight, int fogWeight, int stormWeight,
                        int minFrames, int maxFrames, int lightningChanceDenominator);
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
    WeatherType pickWeightedWeather() const;
    void switchWeather();
    WeatherType m_currentWeather;
    int m_weatherFrameCount;
    int m_weatherDurationFrames;
    mutable bool m_lightningTriggered;
    int m_sunnyWeight;
    int m_fogWeight;
    int m_stormWeight;
    int m_weatherMinFrames;
    int m_weatherMaxFrames;
    int m_lightningChanceDenominator;
};

#endif // WEATHERSYSTEM_H
