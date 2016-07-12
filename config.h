#pragma once

#define Scheduling_Policy SCHED_RR
#define Realtime_Priority 63 // 63 is the maximum value on QNX
#define Background_Thread_Number 0
#define Yield_Count 0
#define Loops_Between_Yields 0xffffff // if testing preemptions, make sure this takes longer than one RR interval
#define Want_Starvation false
#define Temporarily_Block_Preemption false // without it we are not able to avoid starvation if preempted
#define Debug_Output false
#define Debug_Level 0