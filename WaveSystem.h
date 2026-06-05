#ifndef WAVESYSTEM_H
#define WAVESYSTEM_H

#include <QObject>
#include <QElapsedTimer>
#include <QPointF>
#include "GameConfig.h"

enum class WaveDirection { LEFT, RIGHT };

class WaveSystem : public QObject
{
    Q_OBJECT
public:
    static WaveSystem& instance();
    void update(qreal deltaTime);
    void startWave(WaveDirection dir);
    void configureStage(int triggerChancePerFrame, int rightWeight, int leftWeight);
    void reset();
    bool isWarningActive() const { return m_isWarning; }
    bool isWaveActive() const { return m_isWaveActive; }
    WaveDirection currentDirection() const { return m_currentDir; }
    qreal currentSpeedMultiplier() const { return m_speedMultiplier; }
    qreal currentSpeedMultiplierForMovement(const QPointF& moveDir) const;
    qreal warningProgress() const;
    qreal activeProgress() const;
    int warningRemainingMs() const;

signals:
    void waveStateChanged();

private:
    WaveSystem();
    void updateSpeedMultiplier(qreal target, qreal deltaTime);
    bool m_isWarning;
    bool m_isWaveActive;
    WaveDirection m_currentDir;
    qreal m_speedMultiplier;
    QElapsedTimer m_waveTimer;
    int m_triggerChancePerFrame;
    int m_rightWeight;
    int m_leftWeight;
};

#endif // WAVESYSTEM_H
