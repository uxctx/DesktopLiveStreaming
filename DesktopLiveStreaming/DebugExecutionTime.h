#pragma once

#include <time.h>
#include <stdio.h>
#include <debugapi.h>

char temp_debugs_tr[300];
clock_t start_time;
clock_t end_time;
void ExecutionTimeStart()
{
	start_time = clock();
}

void ExecutionTimeStop(char* msg)
{
	end_time = clock();
	sprintf(temp_debugs_tr, "%s:%ld", msg, end_time - start_time);
	OutputDebugStringA(temp_debugs_tr);
}