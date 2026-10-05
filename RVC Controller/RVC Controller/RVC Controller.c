#define TICK 0.2

#include<stdio.h>
#include<time.h>

clock_t start;
int tick1, tick5;

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
	POWER_UP
}clean;

typedef enum {
	MOVE_FORWARD,
	TURN_LEFT,
	TURN_RIGHT,
	MOVE_BACKWARD,
	POWER_UP
}state;

void controller(void);
void determine_obstacle_location(char obstacle_location[3]);
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

int main(void) {
	controller();

	return 0;
}

void controller(void) {
	char obstacle_location[3] = {0,0,0};//함수 내부에서 호출한 하위 함수로부터 입력값을 받아오는것이므로 입력변수는 함수내부에 선언
	char f, l, r;
	char dust_existence = 0;
	state cur_state=MOVE_FORWARD;
	start = clock();

	while (1) {
		tick1 = (int)((double)(clock() - start) / CLOCKS_PER_SEC / TICK) % 2;
		determine_obstacle_location(obstacle_location);
		f = obstacle_location[0];
		l = obstacle_location[1];
		r = obstacle_location[2];
		dust_existence = determine_dust_existence();
		
		switch (cur_state) {
		case MOVE_FORWARD:
			if (dust_existence) {
				cur_state = POWER_UP;
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
		case TURN_RIGHT:
			turn_right(TRIGGER);
			cur_state = MOVE_FORWARD;
		case TURN_LEFT:
			turn_left(TRIGGER);
			cur_state = MOVE_FORWARD;
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
		case POWER_UP:
			on_off_power_up(POWER_UP);
			cur_state = MOVE_FORWARD;
		}
	}
}

void determine_obstacle_location(char obstacle_location[3]) {
	obstacle_location[0] = front_sensor_interface();
	obstacle_location[1] = left_sensor_interface();
	obstacle_location[2] = right_sensor_interface();

	return;
}

char front_sensor_interface(void) {
	char front_sensor_input = read_front_sensor();

	return front_sensor_input == 1 ? 1 : 0;
}

char read_front_sensor(void) {
	return 0;
}

char left_sensor_interface(void) {
	if (tick1 < 1)return 0;
	
	char left_sensor_input = read_left_sensor();

	return left_sensor_input == 1 ? 1 : 0;
}

char read_left_sensor(void) {
	return 0;
}

char right_sensor_interface(void) {
	if (tick1 < 1)return 0;

	char right_sensor_input = read_right_sensor();

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
	if (tick1 < 1)return 0;

	char dust_sensor_input = read_dust_sensor();

	return dust_sensor_input == 1 ? 1 : 0;
}

char read_dust_sensor(void) {
	return 0;
}

void move_forward(enable_signal signal) {
	if (signal == ENABLE) {
		moter_interface(FORWARD);//출력이므로 아웃풋이 될걸 다음 호출 함수 인자로 넘기기
	}

	return;
}

void turn_left(trigger_signal signal) {
	tick5 = (int)((double)(clock() - start) / CLOCKS_PER_SEC / TICK);
	while (tick5 < 5);

	if (signal == TRIGGER) {
		moter_interface(LEFT);
	}

	return;
}

void turn_right(trigger_signal signal) {
	if (signal == TRIGGER) {
		moter_interface(RIGHT);
	}

	return;
}

void move_backward(enable_signal signal) {
	if (signal == ENABLE) {
		moter_interface(BACKWARD);
	}

	return;
}

void motor_interface(direction motor_command) {
	switch (motor_command) {
	case FORWARD:
		run_motor(1, 1);//왼쪽바퀴방향, 오른쪽바퀴방향
	case RIGHT:
		run_motor(1, -1);
	case LEFT:
		run_motor(-1, 1);
	case BACKWARD:
		run_motor(-1, -1);
	default:
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
	switch (cleaner_command) {
	case OFF:
		run_cleaner(0);
	case ON:
		run_cleaner(1);
	case POWER_UP:
		run_cleaner(2);
	default:
	}

	return;
}

void run_cleaner(int level) {
	printf("run_cleaner %d\n",level);
}