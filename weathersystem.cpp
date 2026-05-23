#include "WeatherSystem.h"
#include <QRandomGenerator>

WeatherSystem::WeatherSystem()
    : m_currentWeather(WeatherType::SUNNY), m_weatherFrameCount(0),
    m_weatherDurationFrames(GameConfig::WEATHER_MAX_FRAMES), m_lightningTriggered(false) {}

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

    if (m_currentWeather == WeatherType::STORM && QRandomGenerator::global()->bounded(0, 500) < 1) {
        m_lightningTriggered = true;
    }
}

void WeatherSystem::switchWeather() {
    int rand = QRandomGenerator::global()->bounded(0, 3);
    m_currentWeather = static_cast<WeatherType>(rand);
    m_weatherFrameCount = 0;
    m_weatherDurationFrames = QRandomGenerator::global()->bounded(GameConfig::WEATHER_MIN_FRAMES, GameConfig::WEATHER_MAX_FRAMES + 1);
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
}