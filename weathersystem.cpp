#include "WeatherSystem.h"
#include <QRandomGenerator>

using namespace Config;

WeatherSystem::WeatherSystem()
    : m_currentWeather(WeatherType::SUNNY), m_targetWeather(WeatherType::SUNNY),
    m_weatherFrameCount(0),
    m_weatherDurationFrames(GameConfig::WEATHER_MAX_FRAMES),
    m_transitionFrameCount(0), m_isTransitioning(false),
    m_lightningTriggered(false),
    m_sunnyWeight(1), m_fogWeight(1), m_stormWeight(1),
    m_weatherMinFrames(GameConfig::WEATHER_MIN_FRAMES),
    m_weatherMaxFrames(GameConfig::WEATHER_MAX_FRAMES),
    m_lightningChanceDenominator(500) {}

WeatherSystem& WeatherSystem::instance() {
    static WeatherSystem ws;
    return ws;
}

void WeatherSystem::update(qreal deltaTime) {
    const qreal frameStep = qBound<qreal>(0.0, deltaTime * 60.0, 5.0);
    m_weatherFrameCount += frameStep;
    m_lightningTriggered = false;

    if (m_isTransitioning) {
        m_transitionFrameCount += frameStep;
        if (m_transitionFrameCount >= GameConfig::WEATHER_TRANSITION_FRAMES) {
            m_currentWeather = m_targetWeather;
            m_isTransitioning = false;
            m_transitionFrameCount = 0;
            m_weatherFrameCount = 0;
            m_weatherDurationFrames =
                QRandomGenerator::global()->bounded(m_weatherMinFrames, m_weatherMaxFrames + 1);
        }
    }
    else if (m_weatherFrameCount >= m_weatherDurationFrames) {
        switchWeather();
    }

    if (!m_isTransitioning && m_currentWeather == WeatherType::STORM &&
        m_lightningChanceDenominator > 0) {
        const qreal chance = qMin<qreal>(1.0, frameStep / m_lightningChanceDenominator);
        if (QRandomGenerator::global()->generateDouble() < chance) {
            m_lightningTriggered = true;
        }
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
    m_targetWeather = m_currentWeather;
    m_transitionFrameCount = 0;
    m_isTransitioning = false;
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
    WeatherType nextWeather = pickWeightedWeather();
    for (int attempt = 0; attempt < 4 && nextWeather == m_currentWeather; ++attempt) {
        nextWeather = pickWeightedWeather();
    }

    if (nextWeather == m_currentWeather) {
        m_weatherFrameCount = 0;
        m_weatherDurationFrames =
            QRandomGenerator::global()->bounded(m_weatherMinFrames, m_weatherMaxFrames + 1);
        return;
    }

    m_targetWeather = nextWeather;
    m_transitionFrameCount = 0;
    m_isTransitioning = true;
    emit weatherChanged(m_targetWeather);
}

qreal WeatherSystem::transitionProgress() const {
    if (!m_isTransitioning) return 1.0;
    return qBound<qreal>(
        0.0,
        static_cast<qreal>(m_transitionFrameCount) / GameConfig::WEATHER_TRANSITION_FRAMES,
        1.0
    );
}

qreal WeatherSystem::weatherWeight(WeatherType weather) const {
    if (!m_isTransitioning) {
        return m_currentWeather == weather ? 1.0 : 0.0;
    }

    const qreal progress = transitionProgress();
    qreal weight = 0.0;
    if (m_currentWeather == weather) weight += 1.0 - progress;
    if (m_targetWeather == weather) weight += progress;
    return qBound<qreal>(0.0, weight, 1.0);
}

qreal WeatherSystem::stormIntensity() const {
    return weatherWeight(WeatherType::STORM);
}

qreal WeatherSystem::fogIntensity() const {
    return weatherWeight(WeatherType::FOG);
}

int WeatherSystem::rainLevel() const {
    if (stormIntensity() <= 0.01) return 0;
    if (m_stormWeight >= 50) return 3;
    if (m_stormWeight >= 20) return 2;
    return 1;
}

qreal WeatherSystem::currentVisionMultiplier() const {
    return 1.0 - GameConfig::FOG_VISION_REDUCTION * fogIntensity();
}

qreal WeatherSystem::currentFishValueBonus() const {
    return 1.0 + (GameConfig::STORM_FISH_VALUE_BONUS - 1.0) * stormIntensity();
}

bool WeatherSystem::shouldTriggerLightning() const {
    return m_lightningTriggered;
}

QColor WeatherSystem::overlayColor() const {
    const qreal fog = fogIntensity();
    const qreal storm = stormIntensity();
    if (fog <= 0.001 && storm <= 0.001) {
        return QColor(0, 0, 0, 0);
    }

    const qreal total = qMax<qreal>(0.001, fog + storm);
    const int red = qRound((196.0 * fog + 35.0 * storm) / total);
    const int green = qRound((205.0 * fog + 47.0 * storm) / total);
    const int blue = qRound((210.0 * fog + 68.0 * storm) / total);
    const int alpha = qBound(0, qRound(72.0 * fog + 82.0 * storm), 96);
    return QColor(red, green, blue, alpha);
}

void WeatherSystem::reset() {
    m_currentWeather = WeatherType::SUNNY;
    m_targetWeather = WeatherType::SUNNY;
    m_weatherFrameCount = 0;
    m_transitionFrameCount = 0;
    m_isTransitioning = false;
    m_lightningTriggered = false;
}
