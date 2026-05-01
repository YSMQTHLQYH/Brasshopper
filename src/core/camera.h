#pragma once
#include <SDL3/SDL.h>

typedef struct {
	double pos_x, pos_y;// position of center in world space
	double w, h;		// size of area drawn in world space
	double vel_x, vel_y;
	double mass, spring, damper;
	float shake;
}_sCamera;

void TrackCameraTo(_sCamera* camera, double target_x, double target_y, double dt);
void ShakeCamera(_sCamera* camera, float strengh);
void ResetCamera(_sCamera* camera, double x, double y);