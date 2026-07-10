#pragma once

#include <QElapsedTimer>
#include <QtGlobal>

// A process-wide monotonic clock for gameplay timers.  Unlike QElapsedTimer
// used directly, this clock stops while the game is paused or a modal gameplay
// dialog is open.
class SimulationClock
{
public:
    static qint64 nowMs()
    {
        ensureStarted();
        const qint64 wallNow = wallClock().elapsed();
        const qint64 currentPause = pausedFlag() ? wallNow - pauseStartedMs() : 0;
        return qMax<qint64>(0, wallNow - pausedTotalMs() - currentPause);
    }

    static void reset()
    {
        wallClock().start();
        startedFlag() = true;
        pausedTotalMs() = 0;
        pauseStartedMs() = 0;
        pausedFlag() = false;
    }

    static void setPaused(bool paused)
    {
        ensureStarted();
        if (paused == pausedFlag()) return;

        const qint64 wallNow = wallClock().elapsed();
        if (paused) {
            pauseStartedMs() = wallNow;
            pausedFlag() = true;
        }
        else {
            pausedTotalMs() += qMax<qint64>(0, wallNow - pauseStartedMs());
            pauseStartedMs() = 0;
            pausedFlag() = false;
        }
    }

    static bool isPaused() { return pausedFlag(); }

private:
    static QElapsedTimer& wallClock()
    {
        static QElapsedTimer timer;
        return timer;
    }

    static bool& startedFlag()
    {
        static bool started = false;
        return started;
    }

    static bool& pausedFlag()
    {
        static bool paused = false;
        return paused;
    }

    static qint64& pausedTotalMs()
    {
        static qint64 value = 0;
        return value;
    }

    static qint64& pauseStartedMs()
    {
        static qint64 value = 0;
        return value;
    }

    static void ensureStarted()
    {
        if (!startedFlag()) {
            wallClock().start();
            startedFlag() = true;
        }
    }
};

// QElapsedTimer-compatible subset backed by SimulationClock.
class GameTimer
{
public:
    void start() { m_startedAtMs = SimulationClock::nowMs(); }

    qint64 restart()
    {
        const qint64 previous = elapsed();
        start();
        return previous;
    }

    qint64 elapsed() const
    {
        return isValid() ? qMax<qint64>(0, SimulationClock::nowMs() - m_startedAtMs) : 0;
    }

    bool isValid() const { return m_startedAtMs >= 0; }
    void invalidate() { m_startedAtMs = -1; }

private:
    qint64 m_startedAtMs = -1;
};
