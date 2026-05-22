#include "WaveSystem.h"
#include <QRandomGenerator>

WaveSystem::WaveSystem()
    : m_isWarning(false), m_isWaveActive(false),
    m_currentDir(WaveDirection::RIGHT), m_speedMultiplier(1.0f) {}

WaveSystem& WaveSystem::instance() {
    static WaveSystem ws;
    return ws;
}

void WaveSystem::update(qreal deltaTime) {
    if (m_isWarning && m_waveTimer.elapsed() >= GameConfig::WAVE_WARNING_MS) {
        m_isWarning = false;
        m_isWaveActive = true;
        m_waveTimer.restart();
        emit waveStateChanged();
    }

    if (m_isWaveActive) {
        qreal target = (m_currentDir == WaveDirection::RIGHT)
        ? GameConfig::WAVE_SPEED_UP_MULTIPLIER
        : GameConfig::WAVE_SPEED_DOWN_MULTIPLIER;
        updateSpeedMultiplier(target, deltaTime);

        if (m_waveTimer.elapsed() >= GameConfig::WAVE_DURATION_MS) {
            m_isWaveActive = false;
            updateSpeedMultiplier(1.0f, deltaTime);
            emit waveStateChanged();
        }
    }

    if (!m_isWarning && !m_isWaveActive && QRandomGenerator::global()->bounded(0, 1000) < 1) {
        startWave(QRandomGenerator::global()->bounded(0, 2) == 0 ? WaveDirection::LEFT : WaveDirection::RIGHT);
    }
}

void WaveSystem::startWave(WaveDirection dir) {
    m_isWarning = true;
    m_currentDir = dir;
    m_waveTimer.restart();
    emit waveStateChanged();
}

void WaveSystem::reset() {
    m_isWarning = false;
    m_isWaveActive = false;
    m_speedMultiplier = 1.0f;
}

void WaveSystem::updateSpeedMultiplier(qreal target, qreal deltaTime) {
    if (qAbs(m_speedMultiplier - target) < 0.01f) {
        m_speedMultiplier = target;
        return;
    }
    m_speedMultiplier += (target - m_speedMultiplier) * deltaTime * 2.0f;
}