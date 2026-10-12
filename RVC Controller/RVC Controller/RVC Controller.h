#ifndef RVC_CONTROLLER_H
#define RVC_CONTROLLER_H
	
#define TICK 1
#define TRUE 1
#define FALSE 0

#include <time.h>

typedef enum {
	DISABLE,
	ENABLE
}enable_signal;

typedef enum {
	NO_TRIGGER,
	TRIGGER
}trigger_signal;

typedef enum {
	FORWARD,
	LEFT,
	RIGHT,
	BACKWARD
}direction;

typedef enum {
	OFF,
	ON,
	POWER_UP_COMMAND
}clean;

typedef enum {
	MOVE_FORWARD,
	TURN_LEFT,
	TURN_RIGHT,
	MOVE_BACKWARD,
	POWER_UP_STATE
}state;

extern clock_t tick_start;
extern int tick, tick_5;
extern int front_sensor_input;
extern int left_sensor_input;
extern int right_sensor_input;
extern int dust_sensor_input;

void controller(state initial_state);
char* determine_obstacle_location(void);
char front_sensor_interface(void);
char left_sensor_interface(void);
char right_sensor_interface(void);
char determine_dust_existence(void);
char dust_sensor_interface(void);
void move_forward(enable_signal signal);
void turn_left(trigger_signal signal);
void turn_right(trigger_signal signal);
void move_backward(enable_signal signal);
void motor_interface(direction motor_command);
void on_off_power_up(clean cleaner_command);
void cleaner_interface(clean cleaner_command);

#endif