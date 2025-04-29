#pragma once

class Timer
{
public:
    Timer();

    float TotalTime() const;
    float DeltaTime() const;

    void Reset(); // Call before message loop.
    void Start(); // Call when unpaused.
    void Stop(); // Call when paused.
    void Tick(); // Call every frame.

private:
    double m_SecondsPerCount;
    double m_DeltaTime;

    // 计时起点
    __int64 m_BaseTime;
    // 暂停的总时间
    __int64 m_PausedTime;
    // 上次暂停的时刻
    __int64 m_StopTime;
    // 前一次时刻
    __int64 m_PrevTime;
    // 最近一次时刻
    __int64 m_CurTime;
    // 是否暂停
    bool m_IsStop;
};
