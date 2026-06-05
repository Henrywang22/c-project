#include "WeatherSystem.h"
#include <QRandomGenerator>

using namespace Config;

WeatherSystem::WeatherSystem()
    : m_currentWeather(WeatherType::SUNNY), m_weatherFrameCount(0),
    m_weatherDurationFrames(GameConfig::WEATHER_MAX_FRAMES), m_lightningTriggered(false),
    m_sunnyWeight(1), m_fogWeight(1), m_stormWeight(1),
    m_weatherMinFrames(GameConfig::WEATHER_MIN_FRAMES),
    m_weatherMaxFrames(GameConfig::WEATHER_MAX_FRAMES),
    m_lightningChanceDenominator(500) {}

WeatherSystem& WeatherSystem::instance() {
    static WeatherSystem ws;
    return ws;
}

void WeatherSystem::update(qreal deltaTime) {
    Q_UNUSED(deltaTime);
    m_weatherFrameCount++;
    m_lightningTriggered = false;

    if (m_weatherFrameCount >= m_weatherDurationFrames) {
        switchWeather();
    }

    if (m_currentWeather == WeatherType::STORM && m_lightningChanceDenominator > 0 &&
        QRandomGenerator::global()->bounded(0, m_lightningChanceDenominator) < 1) {
        m_lightningTriggered = true;
    }
}

void WeatherSystem::configureStage(int sunnyWeight, int fogWeight, int stormWeight,
                                   int minFrames, int maxFrames, int lightningChanceDenominator) {
    m_sunnyWeight = qMax(0, sunnyWeight);
    m_fogWeight = qMax(0, fogWeight);
    m_stormWeight = qMax(0, stormWeight);
    m_weatherMinFrames = qMax(1, minFrames);
    m_weatherMaxFrames = qMax(m_weatherMinFrames, maxFrames);
    m_lightningChanceDenominator = lightningChanceDenominator;
    m_weatherFrameCount = 0;
    m_weatherDurationFrames =
        QRandomGenerator::global()->bounded(m_weatherMinFrames, m_weatherMaxFrames + 1);
    m_currentWeather = pickWeightedWeather();
    m_lightningTriggered = false;
    emit weatherChanged(m_currentWeather);
}

WeatherType WeatherSystem::pickWeightedWeather() const {
    const int total = qMax(1, m_sunnyWeight + m_fogWeight + m_stormWeight);
    const int roll = QRandomGenerator::global()->bounded(total);
    if (roll < m_sunnyWeight) return WeatherType::SUNNY;
    if (roll < m_sunnyWeight + m_fogWeight) return WeatherType::FOG;
    return WeatherType::STORM;
}

void WeatherSystem::switchWeather() {
    m_currentWeather = pickWeightedWeather();
    m_weatherFrameCount = 0;
    m_weatherDurationFrames =
        QRandomGenerator::global()->bounded(m_weatherMinFrames, m_weatherMaxFrames + 1);
    emit weatherChanged(m_currentWeather);
}

qreal WeatherSystem::currentVisionMultiplier() const {
    return (m_currentWeather == WeatherType::FOG) ? (1.0f - GameConfig::FOG_VISION_REDUCTION) : 1.0f;
}

qreal WeatherSystem::currentFishValueBonus() const {
    return (m_currentWeather == WeatherType::STORM) ? GameConfig::STORM_FISH_VALUE_BONUS : 1.0f;
}

bool WeatherSystem::shouldTriggerLightning() const {
    return m_lightningTriggered;
}

QColor WeatherSystem::overlayColor() const {
    switch (m_currentWeather) {
    case WeatherType::FOG: return QColor(200, 200, 200, 120);
    case WeatherType::STORM: return QColor(30, 30, 50, 150);
    default: return QColor(0, 0, 0, 0);
    }
}

void WeatherSystem::reset() {
    m_currentWeather = WeatherType::SUNNY;
    m_weatherFrameCount = 0;
    m_lightningTriggered = false;
}
