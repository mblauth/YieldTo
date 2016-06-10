#pragma once


#define Scheduling_Policy SCHED_RR
#define Realtime_Priority 63 // 63 is the maximum value on QNX
#define Background_Thread_Number 20
#define Load_Factor 10
#define YieldBack true