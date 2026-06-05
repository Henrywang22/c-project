#include "WaveSystem.h"
#include <QRandomGenerator>
#include <cmath>

using namespace Config;

WaveSystem::WaveSystem()
    : m_isWarning(false), m_isWaveActive(false),
    m_currentDir(WaveDirection::RIGHT), m_speedMultiplier(1.0f),
    m_triggerChancePerFrame(1000), m_rightWeight(50), m_leftWeight(50) {}

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
        updateSpeedMultiplier(GameConfig::WAVE_SPEED_UP_MULTIPLIER, deltaTime);

        if (m_waveTimer.elapsed() >= GameConfig::WAVE_DURATION_MS) {
            m_isWaveActive = false;
            emit waveStateChanged();
        }
    }
    else {
        updateSpeedMultiplier(1.0f, deltaTime);
    }

    if (!m_isWarning && !m_isWaveActive && m_triggerChancePerFrame > 0 &&
        QRandomGenerator::global()->bounded(0, m_triggerChancePerFrame) < 1) {
        const int totalWeight = qMax(1, m_rightWeight + m_leftWeight);
        const int roll = QRandomGenerator::global()->bounded(totalWeight);
        startWave(roll < m_rightWeight ? WaveDirection::RIGHT : WaveDirection::LEFT);
    }
}

void WaveSystem::startWave(WaveDirection dir) {
    m_isWarning = true;
    m_isWaveActive = false;
    m_currentDir = dir;
    m_waveTimer.restart();
    emit waveStateChanged();
}

void WaveSystem::configureStage(int triggerChancePerFrame, int rightWeight, int leftWeight) {
    m_triggerChancePerFrame = triggerChancePerFrame;
    m_rightWeight = qMax(0, rightWeight);
    m_leftWeight = qMax(0, leftWeight);
}

void WaveSystem::reset() {
    m_isWarning = false;
    m_isWaveActive = false;
    m_speedMultiplier = 1.0f;
}

qreal WaveSystem::warningProgress() const {
    if (!m_isWarning) return 0.0;
    return qBound<qreal>(0.0, static_cast<qreal>(m_waveTimer.elapsed()) / GameConfig::WAVE_WARNING_MS, 1.0);
}

qreal WaveSystem::activeProgress() const {
    if (!m_isWaveActive) return 0.0;
    return qBound<qreal>(0.0, static_cast<qreal>(m_waveTimer.elapsed()) / GameConfig::WAVE_DURATION_MS, 1.0);
}

qreal WaveSystem::currentSpeedMultiplierForMovement(const QPointF& moveDir) const
{
    if (!m_isWaveActive) {
        return 1.0;
    }

    const qreal length = std::sqrt(moveDir.x() * moveDir.x() + moveDir.y() * moveDir.y());
    if (length <= 0.001) {
        return 1.0;
    }

    const qreal waveSign = m_currentDir == WaveDirection::RIGHT ? 1.0 : -1.0;
    const qreal alignment = (moveDir.x() / length) * waveSign;
    const qreal ramp = qBound<qreal>(
        0.0,
        qAbs(m_speedMultiplier - 1.0) / qMax<qreal>(0.001, GameConfig::WAVE_SPEED_UP_MULTIPLIER - 1.0),
        1.0
    );

    if (alignment > 0.0) {
        return 1.0 + (GameConfig::WAVE_SPEED_UP_MULTIPLIER - 1.0) * alignment * ramp;
    }

    if (alignment < 0.0) {
        return 1.0 - (1.0 - GameConfig::WAVE_SPEED_DOWN_MULTIPLIER) * (-alignment) * ramp;
    }

    return 1.0;
}

int WaveSystem::warningRemainingMs() const {
    if (!m_isWarning) return 0;
    return qMax(0, GameConfig::WAVE_WARNING_MS - static_cast<int>(m_waveTimer.elapsed()));
}

void WaveSystem::updateSpeedMultiplier(qreal target, qreal deltaTime) {
    if (qAbs(m_speedMultiplier - target) < 0.01f) {
        m_speedMultiplier = target;
        return;
    }
    m_speedMultiplier += (target - m_speedMultiplier) * deltaTime * 2.0f;
}
