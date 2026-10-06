#define TICK 0.2
#define TRUE 1
#define FALSE 0

#include <stdio.h>
#include <time.h>
#include <stdlib.h>

clock_t start1,start5;
int tick1, tick5;
char locked = FALSE;

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

void controller(void);
char* determine_obstacle_location(void);
char front_sensor_interface(void);
char read_front_sensor(void);
char left_sensor_interface(void);
char read_left_sensor(void);
char right_sensor_interface(void);
char read_right_sensor(void);
char determine_dust_existence(void);
char dust_sensor_interface(void);
char read_dust_sensor(void);
void move_forward(enable_signal signal);
void turn_left(trigger_signal signal);
void turn_right(trigger_signal signal);
void move_backward(enable_signal signal);
void motor_interface(direction motor_command);
void run_motor(int left, int right);
void on_off_power_up(clean cleaner_command);
void cleaner_interface(clean cleaner_command);
void run_cleaner(int level);

int main(void) {
	controller();

	return 0;
}

void controller(void) {
	char* obstacle_location;//함수 내부에서 호출한 하위 함수로부터 입력값을 받아오는것이므로 입력변수는 함수내부에 선언
	char f, l, r;
	char dust_existence;
	state cur_state=MOVE_FORWARD;
	tick1 = 1;

	while (1) {
		if(tick1>=1){
			start1 = clock();
			tick1 = 0;

			obstacle_location = determine_obstacle_location();
			f = obstacle_location[0];
			l = obstacle_location[1];
			r = obstacle_location[2];
			free(obstacle_location);
			dust_existence = determine_dust_existence();

			switch (cur_state) {
			case MOVE_FORWARD:
				if (dust_existence) {
					cur_state = POWER_UP_STATE;
					start5 = clock();
					tick5 = (int)((double)(clock() - start5) / CLOCKS_PER_SEC / TICK);
				}
				else if (!f) {
					on_off_power_up(ON);
					move_forward(ENABLE);
				}
				else if (f && !r) {
					move_forward(DISABLE);
					on_off_power_up(OFF);
					cur_state = TURN_RIGHT;
				}
				else if (f && !l && r) {
					move_forward(DISABLE);
					on_off_power_up(OFF);
					cur_state = TURN_LEFT;
				}
				else if (f && l && r) {
					move_forward(DISABLE);
					on_off_power_up(OFF);
					cur_state = MOVE_BACKWARD;
				}
				break;
			case TURN_RIGHT:
				if (!locked) {
					locked = TRUE;
					start5 = clock();
					tick5 = 0;
					turn_right(TRIGGER);
				}
				else if (locked && tick5 >= 5) {
					locked = FALSE;
					cur_state = MOVE_FORWARD;
				}
				tick5 = (int)((double)(clock() - start5) / CLOCKS_PER_SEC / TICK);
				break;
			case TURN_LEFT:
				if (!locked) {
					locked = TRUE;
					start5 = clock();
					tick5 = 0;
					turn_left(TRIGGER);
				}
				else if (locked && tick5 >= 5) {
					locked = FALSE;
					cur_state = MOVE_FORWARD;
				}
				tick5 = (int)((double)(clock() - start5) / CLOCKS_PER_SEC / TICK);
				break;
			case MOVE_BACKWARD:
				if (!r) {
					move_backward(DISABLE);
					cur_state = TURN_RIGHT;
				}
				else if (!l && r) {
					move_backward(DISABLE);
					cur_state = TURN_LEFT;
				}
				else if (l && r) {
					move_backward(ENABLE);
				}
				break;
			case POWER_UP_STATE:
				if (dust_existence) {
					on_off_power_up(POWER_UP_COMMAND);
				}
				else if (!f) {
					if (tick5 < 5) {
						on_off_power_up(POWER_UP_COMMAND);
					}
					else {
						on_off_power_up(ON);
						cur_state = MOVE_FORWARD;
					}
					move_forward(ENABLE);
				}
				else if (f && !r) {
					move_forward(DISABLE);
					on_off_power_up(OFF);
					cur_state = TURN_RIGHT;
				}
				else if (f && !l && r) {
					move_forward(DISABLE);
					on_off_power_up(OFF);
					cur_state = TURN_LEFT;
				}
				else if (f && l && r) {
					move_forward(DISABLE);
					on_off_power_up(OFF);
					cur_state = MOVE_BACKWARD;
				}
				tick5 = (int)((double)(clock() - start5) / CLOCKS_PER_SEC / TICK);
				break;
			default:
				cur_state = MOVE_FORWARD;
			}
		}
		tick1 = (int)((double)(clock() - start1) / CLOCKS_PER_SEC / TICK);
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
	static char front_sensor_input = 0;

	front_sensor_input = read_front_sensor();

	return front_sensor_input == 1 ? 1 : 0;
}

char read_front_sensor(void) {
	return 0;
}

char left_sensor_interface(void) {
	static char left_sensor_input = 0;

	if (tick1 >= 1) {
		left_sensor_input = read_left_sensor();
	}

	return left_sensor_input == 1 ? 1 : 0;
}

char read_left_sensor(void) {
	return 0;
}

char right_sensor_interface(void) {
	static char right_sensor_input = 0;

	if (tick1 >= 1) {
		right_sensor_input = read_right_sensor();
	}

	return right_sensor_input == 1 ? 1 : 0;
}

char read_right_sensor(void) {
	return 0;
}

char determine_dust_existence(void) {
	char dust_existence = dust_sensor_interface();

	return dust_existence;
}

char dust_sensor_interface(void) {
	static char dust_sensor_input = 0;

	if (tick1 >= 1) {
		dust_sensor_input = read_dust_sensor();
	}

	return dust_sensor_input == 1 ? 1 : 0;
}

char read_dust_sensor(void) {
	return 0;
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
	static direction last = -1;

	if (last!=motor_command) {
		last = motor_command;
		switch (motor_command) {
		case FORWARD:
			run_motor(1, 1);//왼쪽바퀴방향, 오른쪽바퀴방향
			break;
		case RIGHT:
			run_motor(1, -1);
			break;
		case LEFT:
			run_motor(-1, 1);
			break;
		case BACKWARD:
			run_motor(-1, -1);
			break;
		default:
			run_motor(1, 1);
		}
	}

	return;
}

void run_motor(int left, int right) {
	printf("run_motor %d %d\n", left, right);

	return;
}

void on_off_power_up(clean cleaner_command) {
	cleaner_interface(cleaner_command);
	
	return;
}

void cleaner_interface(clean cleaner_command) {
	static clean last=-1;

	if (last != cleaner_command) {
		last = cleaner_command;
		switch (cleaner_command) {
		case OFF:
			run_cleaner(0);
			break;
		case ON:
			run_cleaner(1);
			break;
		case POWER_UP_COMMAND:
			run_cleaner(2);
			break;
		default:
			run_cleaner(0);
		}
	}

	return;
}

void run_cleaner(int level) {
	printf("run_cleaner %d\n",level);
}