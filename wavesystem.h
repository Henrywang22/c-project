#ifndef WAVESYSTEM_H
#define WAVESYSTEM_H

#include <QObject>
#include <QElapsedTimer>
#include "GameConfig.h"

enum class WaveDirection { LEFT, RIGHT };

class WaveSystem : public QObject
{
    Q_OBJECT
public:
    static WaveSystem& instance();
    void update(qreal deltaTime);
    void startWave(WaveDirection dir);
    void reset();
    bool isWarningActive() const { return m_isWarning; }
    WaveDirection currentDirection() const { return m_currentDir; }
    qreal currentSpeedMultiplier() const { return m_speedMultiplier; }

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
};

#endif // WAVESYSTEM_H