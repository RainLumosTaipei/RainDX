#include "d3d/Timer.h"
#include <windows.h>


Timer::Timer()
: m_SecondsPerCount(0.0), m_DeltaTime(-1.0), m_BaseTime(0), 
  m_PausedTime(0), m_StopTime(0), m_PrevTime(0), m_CurTime(0), m_IsStop(false)
{
	__int64 countsPerSec;
	// 获取每秒计时的频率
	QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&countsPerSec));
	// 每次计时对应的物理秒数
	m_SecondsPerCount = 1.0 / static_cast<double>(countsPerSec);
}

// 获取总计时
float Timer::TotalTime()const
{

	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------------*------> time
	//  m_BaseTime       m_StopTime        startTime     m_StopTime    m_CurTime

	if( m_IsStop )
	{
		return static_cast<float>(
			static_cast<double>(m_StopTime - m_PausedTime - m_BaseTime) * m_SecondsPerCount);
	}

	
	//  (m_CurTime - m_PausedTime) - m_BaseTime 
	//
	//                     |<--paused time-->|
	// ----*---------------*-----------------*------------*------> time
	//  m_BaseTime       m_StopTime        startTime     m_CurTime
	
	return static_cast<float>(
		static_cast<double>(m_CurTime - m_PausedTime - m_BaseTime) * m_SecondsPerCount);
	
}

// 帧间隔
float Timer::DeltaTime() const
{
	return static_cast<float>(m_DeltaTime);
}

// 重置计时
void Timer::Reset()
{
	__int64 curTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&curTime));

	m_BaseTime = curTime;
	m_PrevTime = curTime;
	m_StopTime = 0;
	m_IsStop  = false;
}

// 开始计时，记录停顿时长
void Timer::Start()
{
	__int64 startTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&startTime));


	// Accumulate the time elapsed between stop and start pairs.
	//
	//                     |<-------d------->|
	// ----*---------------*-----------------*------------> time
	//  m_BaseTime       m_StopTime        startTime     

	if( m_IsStop )
	{
		m_PausedTime += (startTime - m_StopTime);	

		m_PrevTime = startTime;
		m_StopTime = 0;
		m_IsStop  = false;
	}
}

// 停止计时，记录停止时间
void Timer::Stop()
{
	if( !m_IsStop )
	{
		__int64 curTime;
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&curTime));

		m_StopTime = curTime;
		m_IsStop  = true;
	}
}

void Timer::Tick()
{
	// 已暂停跳过计时
	if( m_IsStop )
	{
		m_DeltaTime = 0.0;
		return;
	}

	__int64 curTime;
	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&curTime));
	m_CurTime = curTime;
	m_PrevTime = m_CurTime;

	// Time difference between this frame and the previous.
	m_DeltaTime = static_cast<double>(m_CurTime - m_PrevTime) * m_SecondsPerCount;
	
	if(m_DeltaTime < 0.0) m_DeltaTime = 0.0;
}

