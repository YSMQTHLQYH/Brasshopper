#include "camera.h"
#define SHAKE_FALLOFF	500
#define MIN_SHAKE		50
#define SHAKE_MULT		50
#define SHAKE_DIR_TIME	0.1

static void ShakeTick(_sCamera* camera, double dt);

void TrackCameraTo(_sCamera* camera, double target_x, double target_y, double dt) {
	ShakeTick(camera, dt);

	//Sum of forces = m*a = - k*x - c*v
	//=>  acc = (- k*x - c*v) / m
	double dist, f_spring, f_damper;
	double acc_x, acc_y;

	dist = camera->pos_x - target_x;
	f_spring = camera->spring * dist;
	f_damper = camera->damper * camera->vel_x;
	acc_x = (- f_spring - f_damper) / camera->mass;

	dist = camera->pos_y - target_y;
	f_spring = camera->spring * dist;
	f_damper = camera->damper * camera->vel_y;
	acc_y = (- f_spring - f_damper) / camera->mass;

	//integrals, Oooh scary ~(>_<. )
	camera->vel_x += acc_x * dt;
	camera->vel_y += acc_y * dt;

	camera->pos_x += camera->vel_x * dt;
	camera->pos_y += camera->vel_y * dt;
}

static void ShakeTick(_sCamera* camera, double dt) {
	static double time_change_dir = 0, angle = 0;
	time_change_dir -= dt;
	if (camera->shake < MIN_SHAKE) { camera->shake = 0; return; }
	if (time_change_dir <= 0) {
		angle = SDL_randf() * 3.14159165 * 2;
		time_change_dir = SDL_randf() * SHAKE_DIR_TIME;
	}
	double stenght = camera->shake * dt;
	camera->vel_x += SDL_cos(angle) * stenght;
	camera->vel_y += SDL_sin(angle) * stenght;
	camera->shake -= SHAKE_MULT * SHAKE_FALLOFF * dt;
}
void ShakeCamera(_sCamera* camera, float strengh) {
	camera->shake = strengh * SHAKE_MULT;
}

void ResetCamera(_sCamera* camera, double x, double y) {
	camera->pos_x = x;
	camera->pos_y = y;
	camera->vel_x = 0;
	camera->vel_y = 0;
	camera->shake = 0;
}