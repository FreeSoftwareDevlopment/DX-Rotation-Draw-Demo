#include <Windows.h>
#include <math.h>
#include <D2d1.h>
#include "cursorHelper.h"
#pragma comment(lib, "d2d1")

#define PI 3.14159265
#define minusTan(param) ( atan (param) * 180 / PI )

int cCalc(POINT myh, D2D_POINT_2F px) {
	return (myh.x < px.x ? 180 : 0) + minusTan((px.y - myh.y) / (px.x - myh.x));
}

POINT cCalc(HWND myh) {
	POINT p;
	if (GetCursorPos(&p))
	{
		ScreenToClient(myh, &p);
		return p;
	}
	p.x = 0;
	p.y = 0;
	return p;
}
