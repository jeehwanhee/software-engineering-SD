#define TICK 1
#define TRUE 1
#define FALSE 0

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>

clock_t tick_start;
int tick, tick_5;

/* 테스트에서는 실행 전에 센서 입력값을 바꿀 수 있다. */
int front_sensor_input = 1;
int left_sensor_input = 0;
int right_sensor_input = 1;
int dust_sensor_input = 0;

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

int main(void) {
	controller(MOVE_FORWARD);

	return 0;
}

void controller(state initial_state) {
	state last_state = initial_state;
	char* obstacle_location;
	char f, l, r;
	char dust_existence;
	tick = 1;

	while (1) {
		if (tick >= 1) {
			tick_start = clock();
			tick = 0;

			obstacle_location = determine_obstacle_location();
			f = obstacle_location[0];
			l = obstacle_location[1];
			r = obstacle_location[2];
			free(obstacle_location);
			dust_existence = determine_dust_existence();

			switch (last_state) {
			case MOVE_FORWARD:
				if (dust_existence) {
					on_off_power_up(POWER_UP_COMMAND);
					tick_5 = 0;
					printf("tick %d\n", tick_5 + 1);
					last_state = POWER_UP_STATE;
					if (!f) {
						move_forward(ENABLE);
					}
					else if (f && !r) {
						move_forward(DISABLE);
						on_off_power_up(OFF);
						turn_right(TRIGGER);
						tick_5 = 0;
						printf("tick %d\n", tick_5 + 1);
						last_state = TURN_RIGHT;
					}
					else if (f && !l && r) {
						move_forward(DISABLE);
						on_off_power_up(OFF);
						turn_left(TRIGGER);
						tick_5 = 0;
						printf("tick %d\n", tick_5 + 1);
						last_state = TURN_LEFT;
					}
					else if (f && l && r) {
						move_forward(DISABLE);
						on_off_power_up(OFF);
						move_backward(ENABLE);
						last_state = MOVE_BACKWARD;
					}
				}
				else if (!f) {
					on_off_power_up(ON);
					move_forward(ENABLE);
				}
				else if (f && !r) {
					move_forward(DISABLE);
					on_off_power_up(OFF);
					turn_right(TRIGGER);
					tick_5 = 0;
					printf("tick %d\n", tick_5 + 1);
					last_state = TURN_RIGHT;
				}
				else if (f && !l && r) {
					move_forward(DISABLE);
					on_off_power_up(OFF);
					turn_left(TRIGGER);
					tick_5 = 0;
					printf("tick %d\n", tick_5 + 1);
					last_state = TURN_LEFT;
				}
				else if (f && l && r) {
					move_forward(DISABLE);
					on_off_power_up(OFF);
					move_backward(ENABLE);
					last_state = MOVE_BACKWARD;
				}
				break;
			case TURN_RIGHT:
			case TURN_LEFT:
				tick_5++;
				printf("tick %d\n", tick_5 + 1);
				if (tick_5 >= 4) {
					tick_5 = 0;
					move_forward(ENABLE);
					on_off_power_up(ON);
					last_state = MOVE_FORWARD;
				}
				break;
			case MOVE_BACKWARD:
				if (!r) {
					move_backward(DISABLE);
					turn_right(TRIGGER);
					tick_5 = 0;
					printf("tick %d\n", tick_5 + 1);
					last_state = TURN_RIGHT;
				}
				else if (!l && r) {
					move_backward(DISABLE);
					turn_left(TRIGGER);
					tick_5 = 0;
					printf("tick %d\n", tick_5 + 1);
					last_state = TURN_LEFT;
				}
				else if (l && r) {
					move_backward(ENABLE);
				}
				break;
			case POWER_UP_STATE:
				if (!f) {
					tick_5++;
					printf("tick %d\n", tick_5 + 1);
					if (tick_5 >= 4) {
						tick_5 = 0;
						on_off_power_up(ON);
						last_state = MOVE_FORWARD;
					}
					move_forward(ENABLE);
				}
				else if (f && !r) {
					move_forward(DISABLE);
					on_off_power_up(OFF);
					turn_right(TRIGGER);
					tick_5 = 0;
					printf("tick %d\n", tick_5 + 1);
					last_state = TURN_RIGHT;
				}
				else if (f && !l && r) {
					move_forward(DISABLE);
					on_off_power_up(OFF);
					turn_left(TRIGGER);
					tick_5 = 0;
					printf("tick %d\n", tick_5 + 1);
					last_state = TURN_LEFT;
				}
				else if (f && l && r) {
					move_forward(DISABLE);
					on_off_power_up(OFF);
					move_backward(ENABLE);
					last_state = MOVE_BACKWARD;
				}
				break;
			default:
				last_state = MOVE_FORWARD;
			}
		}
		tick = (int)((double)(clock() - tick_start) / CLOCKS_PER_SEC / TICK);
	}
}

char* determine_obstacle_location() {
	char* obstacle_location = (char*)malloc(sizeof(char) * 3);
	obstacle_location[0] = front_sensor_interface();
	obstacle_location[1] = left_sensor_interface();
	obstacle_location[2] = right_sensor_interface();

	return obstacle_location;
}

char front_sensor_interface(void) {
	printf("Front Sensor Input: %d\n", front_sensor_input);

	return front_sensor_input == 1 ? 1 : 0;
}

char left_sensor_interface(void) {
	printf("Left Sensor Input: %d\n", left_sensor_input);

	return left_sensor_input == 1 ? 1 : 0;
}

char right_sensor_interface(void) {
	printf("Right sensor input: %d\n", right_sensor_input);

	return right_sensor_input == 1 ? 1 : 0;
}

char determine_dust_existence(void) {
	char dust_existence = dust_sensor_interface();

	return dust_existence;
}

char dust_sensor_interface(void) {
	printf("Dust Sensor Input: %d\n", dust_sensor_input);

	return dust_sensor_input == 1 ? 1 : 0;
}

void move_forward(enable_signal signal) {
	if (signal == ENABLE) {
		motor_interface(FORWARD);//출력이므로 아웃풋이 될걸 다음 호출 함수 인자로 넘기기
	}

	return;
}

void turn_left(trigger_signal signal) {
	if (signal == TRIGGER) {
		motor_interface(LEFT);
	}

	return;
}

void turn_right(trigger_signal signal) {
	if (signal == TRIGGER) {
		motor_interface(RIGHT);
	}

	return;
}

void move_backward(enable_signal signal) {
	if (signal == ENABLE) {
		motor_interface(BACKWARD);
	}

	return;
}

void motor_interface(direction motor_command) {

	switch (motor_command) {
	case FORWARD:
		printf("FORWARD\n");
		break;
	case RIGHT:
		printf("RIGHT\n");
		break;
	case LEFT:
		printf("LEFT\n");
		break;
	case BACKWARD:
		printf("BACKWARD\n");
		break;
	default:
		printf("WRONG COMMAND\n");
	}

	return;
}

void on_off_power_up(clean cleaner_command) {
	cleaner_interface(cleaner_command);
	
	return;
}

void cleaner_interface(clean cleaner_command) {
	switch (cleaner_command) {
	case OFF:
		printf("OFF\n");
		break;
	case ON:
		printf("ON\n");
		break;
	case POWER_UP_COMMAND:
		printf("POWER UP\n");
		break;
	default:
		printf("WRONG COMMAND\n");
	}

	return;
}
