#pragma once

#define Scheduling_Policy SCHED_RR
#define Realtime_Priority 63 // 63 is the maximum value on QNX
#define Background_Thread_Number 20
#define Yield_Count 40
#define Loops_Between_Yields 0xffffff
#define Want_Starvation true
#define Debug_Output true
#define Debug_Level 0
#define Log_Type simple_ascii_log