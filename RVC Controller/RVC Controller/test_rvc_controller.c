#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>

#define main rvc_controller_main
#include "RVC Controller.c"
#undef main

/* 한 tick 뒤의 카운터와 상태를 함께 검사한다. */
static void test_state_tick(state current_state, int previous_tick,
                            state expected_state)
{
    int expected_tick;
    state actual_state;

    if (current_state != TURN_LEFT &&
        current_state != TURN_RIGHT &&
        current_state != POWER_UP_STATE) {
        CU_FAIL("Select TURN_LEFT, TURN_RIGHT, or POWER_UP_STATE");
        return;
    }

    /* POWER_UP_STATE의 카운터를 검사할 때는 전방이 열려 있어야 한다. */
    if (current_state == POWER_UP_STATE) {
        CU_ASSERT_EQUAL_FATAL(front_sensor_interface(), 0);
    }

    expected_tick = previous_tick + 1;
    if (expected_tick >= 4) {
        expected_tick = 0;
    }

    tick_5 = previous_tick;
    actual_state = controller_step(current_state);

    CU_ASSERT_EQUAL(tick_5, expected_tick);
    CU_ASSERT_EQUAL(actual_state, expected_state);
}

/* 카운터와 관계없이 전진 또는 후진 상태가 유지되는지 검사한다. */
static void test_state_condition(state current_state)
{
    state actual_state;

    if (current_state != MOVE_FORWARD && current_state != MOVE_BACKWARD) {
        CU_FAIL("Select MOVE_FORWARD or MOVE_BACKWARD");
        return;
    }

    actual_state = controller_step(current_state);
    CU_ASSERT_EQUAL(actual_state, current_state);
}

/* 전진 또는 후진에서 조건을 만족하면 시간으로 제어하는 상태에 진입한다. */
static void test_condition_to_tick(state current_state, state expected_state)
{
    state actual_state;

    if (current_state != MOVE_FORWARD && current_state != MOVE_BACKWARD) {
        CU_FAIL("Select MOVE_FORWARD or MOVE_BACKWARD");
        return;
    }

    actual_state = controller_step(current_state);
    CU_ASSERT_EQUAL(actual_state, expected_state);
    CU_ASSERT_EQUAL(tick_5, 0);
}

static void test_forward_remain(void)
{
    CU_ASSERT_EQUAL_FATAL(front_sensor_interface(), 0);
    CU_ASSERT_EQUAL_FATAL(determine_dust_existence(), 0);

    test_state_condition(MOVE_FORWARD);
}

static void test_backward_remain(void)
{
    CU_ASSERT_EQUAL_FATAL(left_sensor_interface(), 1);
    CU_ASSERT_EQUAL_FATAL(right_sensor_interface(), 1);

    test_state_condition(MOVE_BACKWARD);
}

static void test_forward_to_turn_left(void)
{
    CU_ASSERT_EQUAL_FATAL(front_sensor_interface(), 1);
    CU_ASSERT_EQUAL_FATAL(left_sensor_interface(), 0);
    CU_ASSERT_EQUAL_FATAL(right_sensor_interface(), 1);
    CU_ASSERT_EQUAL_FATAL(determine_dust_existence(), 0);

    test_condition_to_tick(MOVE_FORWARD, TURN_LEFT);
}

static void test_forward_to_backward(void)
{
    state actual_state;

    CU_ASSERT_EQUAL_FATAL(front_sensor_interface(), 1);
    CU_ASSERT_EQUAL_FATAL(left_sensor_interface(), 1);
    CU_ASSERT_EQUAL_FATAL(right_sensor_interface(), 1);
    CU_ASSERT_EQUAL_FATAL(determine_dust_existence(), 0);

    actual_state = controller_step(MOVE_FORWARD);
    CU_ASSERT_EQUAL(actual_state, MOVE_BACKWARD);
}

static void test_turn_left_remain(void)
{
    test_state_tick(TURN_LEFT, 0, TURN_LEFT);
    test_state_tick(TURN_LEFT, 1, TURN_LEFT);
    test_state_tick(TURN_LEFT, 2, TURN_LEFT);
}

static void test_turn_left_end(void)
{
    test_state_tick(TURN_LEFT, 3, MOVE_FORWARD);
}

static void test_forward_to_turn_right(void)
{
    CU_ASSERT_EQUAL_FATAL(front_sensor_interface(), 1);
    CU_ASSERT_EQUAL_FATAL(right_sensor_interface(), 0);
    CU_ASSERT_EQUAL_FATAL(determine_dust_existence(), 0);

    test_condition_to_tick(MOVE_FORWARD, TURN_RIGHT);
}

static void test_forward_to_powerup(void)
{
    CU_ASSERT_EQUAL_FATAL(front_sensor_interface(), 0);
    CU_ASSERT_EQUAL_FATAL(determine_dust_existence(), 1);

    test_condition_to_tick(MOVE_FORWARD, POWER_UP_STATE);
}

static void test_backward_to_turn_left(void)
{
    CU_ASSERT_EQUAL_FATAL(left_sensor_interface(), 0);
    CU_ASSERT_EQUAL_FATAL(right_sensor_interface(), 1);

    test_condition_to_tick(MOVE_BACKWARD, TURN_LEFT);
}

static void test_backward_to_turn_right(void)
{
    CU_ASSERT_EQUAL_FATAL(right_sensor_interface(), 0);

    test_condition_to_tick(MOVE_BACKWARD, TURN_RIGHT);
}

static void test_turn_right_remain(void)
{
    test_state_tick(TURN_RIGHT, 0, TURN_RIGHT);
    test_state_tick(TURN_RIGHT, 1, TURN_RIGHT);
    test_state_tick(TURN_RIGHT, 2, TURN_RIGHT);
}

static void test_turn_right_end(void)
{
    test_state_tick(TURN_RIGHT, 3, MOVE_FORWARD);
}

/* POWER_UP_STATE 테스트는 F=0 입력을 제공한 뒤 실행한다. */
static void test_powerup_remain(void)
{
    /* 한 번 진입하면 먼지가 사라져도 정해진 tick 동안 POWER_UP_STATE를 유지한다. */
    test_state_tick(POWER_UP_STATE, 0, POWER_UP_STATE);
    test_state_tick(POWER_UP_STATE, 1, POWER_UP_STATE);
    test_state_tick(POWER_UP_STATE, 2, POWER_UP_STATE);
}

static void test_powerup_end(void)
{
    /* 먼지 유무와 관계없이 마지막 tick이 끝나면 전진으로 변경한다. */
    test_state_tick(POWER_UP_STATE, 3, MOVE_FORWARD);
}

static void test_powerup_to_turn_left(void)
{
    int previous_tick;
    state actual_state;

    /* POWER_UP_STATE 중 장애물을 만나면 카운터와 관계없이 좌회전으로 변경한다. */
    CU_ASSERT_EQUAL_FATAL(front_sensor_interface(), 1);
    CU_ASSERT_EQUAL_FATAL(left_sensor_interface(), 0);
    CU_ASSERT_EQUAL_FATAL(right_sensor_interface(), 1);

    for (previous_tick = 0; previous_tick <= 3; previous_tick++) {
        tick_5 = previous_tick;
        actual_state = controller_step(POWER_UP_STATE);
        CU_ASSERT_EQUAL(actual_state, TURN_LEFT);
        CU_ASSERT_EQUAL(tick_5, 0);
    }
}

static void test_powerup_to_turn_right(void)
{
    int previous_tick;
    state actual_state;

    CU_ASSERT_EQUAL_FATAL(front_sensor_interface(), 1);
    CU_ASSERT_EQUAL_FATAL(right_sensor_interface(), 0);

    for (previous_tick = 0; previous_tick <= 3; previous_tick++) {
        tick_5 = previous_tick;
        actual_state = controller_step(POWER_UP_STATE);
        CU_ASSERT_EQUAL(actual_state, TURN_RIGHT);
        CU_ASSERT_EQUAL(tick_5, 0);
    }
}

static void test_powerup_to_backward(void)
{
    int previous_tick;
    state actual_state;

    CU_ASSERT_EQUAL_FATAL(front_sensor_interface(), 1);
    CU_ASSERT_EQUAL_FATAL(left_sensor_interface(), 1);
    CU_ASSERT_EQUAL_FATAL(right_sensor_interface(), 1);

    /* POWER_UP_STATE의 진행 카운터와 관계없이 후진으로 변경한다. */
    for (previous_tick = 0; previous_tick <= 3; previous_tick++) {
        tick_5 = previous_tick;
        actual_state = controller_step(POWER_UP_STATE);
        CU_ASSERT_EQUAL(actual_state, MOVE_BACKWARD);
    }
}

static void test_powerup_restart(void)
{
    state actual_state;
    int i;

    /* 전방이 열려 있고 먼지가 계속 감지되는 상황이다. */
    CU_ASSERT_EQUAL_FATAL(front_sensor_interface(), 0);
    CU_ASSERT_EQUAL_FATAL(determine_dust_existence(), 1);

    tick_5 = 0;
    actual_state = controller_step(MOVE_FORWARD);
    CU_ASSERT_EQUAL_FATAL(actual_state, POWER_UP_STATE);
    CU_ASSERT_EQUAL(tick_5, 0);

    /* 진입 tick 이후 세 tick 동안 POWER_UP_STATE를 유지한다. */
    for (i = 1; i <= 3; i++) {
        actual_state = controller_step(actual_state);
        CU_ASSERT_EQUAL_FATAL(actual_state, POWER_UP_STATE);
        CU_ASSERT_EQUAL(tick_5, i);
    }

    /* 다섯 번째 tick에는 반드시 전진으로 돌아간다. */
    actual_state = controller_step(actual_state);
    CU_ASSERT_EQUAL_FATAL(actual_state, MOVE_FORWARD);
    CU_ASSERT_EQUAL(tick_5, 0);

    /* 다음 tick에 먼지를 감지하면 POWER_UP_STATE를 다시 시작한다. */
    actual_state = controller_step(actual_state);
    CU_ASSERT_EQUAL(actual_state, POWER_UP_STATE);
    CU_ASSERT_EQUAL(tick_5, 0);
}

/* 입력 뒤에 남은 줄바꿈을 비운다. 결과 화면에서는 엔터 입력을 기다린다. */
static int finish_line(void)
{
    int ch;

    do {
        ch = getchar();
    } while (ch != '\n' && ch != EOF);

    return ch;
}

int main(void)
{
    const char* test_names[] = {
        "Forward remain (F=0, D=0)",
        "Backward remain (L=1, R=1)",
        "Forward -> Turn left (F=1, L=0, R=1, D=0)",
        "Forward -> Backward (F=1, L=1, R=1, D=0)",
        "Turn left remain (any input)",
        "Turn left end (any input)",
        "Forward -> Turn right (F=1, R=0, D=0)",
        "Forward -> Power up (F=0, D=1)",
        "Backward -> Turn left (L=0, R=1)",
        "Backward -> Turn right (R=0)",
        "Turn right remain (any input)",
        "Turn right end (any input)",
        "Power up remain (F=0)",
        "Power up end (F=0)",
        "Power up -> Turn left (F=1, L=0, R=1)",
        "Power up -> Turn right (F=1, R=0)",
        "Power up -> Backward (F=1, L=1, R=1)",
        "Power up -> Forward -> Power up (F=0, D=1)"
    };
    CU_TestFunc test_functions[] = {
        test_forward_remain,
        test_backward_remain,
        test_forward_to_turn_left,
        test_forward_to_backward,
        test_turn_left_remain,
        test_turn_left_end,
        test_forward_to_turn_right,
        test_forward_to_powerup,
        test_backward_to_turn_left,
        test_backward_to_turn_right,
        test_turn_right_remain,
        test_turn_right_end,
        test_powerup_remain,
        test_powerup_end,
        test_powerup_to_turn_left,
        test_powerup_to_turn_right,
        test_powerup_to_backward,
        test_powerup_restart
    };
    int test_count = (int)(sizeof(test_functions) / sizeof(test_functions[0]));
    int choice, count, i;
    CU_ErrorCode error;
    CU_pSuite suite;
    unsigned int failures;

    while (1) {
        printf("=== RVC Controller Test ===\n");
        printf("F: front, L: left, R: right, D: dust\n");
        printf("0: clear, 1: detected\n\n");
        for (i = 0; i < test_count; i++) {
            printf("%2d. %s\n", i + 1, test_names[i]);
        }
        printf(" 0. Exit\n\nSelect test: ");
        fflush(stdout);

        count = scanf("%d", &choice);
        if (count == EOF) {
            break;
        }
        finish_line();
        if (count != 1 || choice < 0 || choice > test_count) {
            printf("Enter a test number from 0 to %d.\n\n", test_count);
            continue;
        }
        if (choice == 0) {
            break;
        }

        printf("Enter F L R D (example: 1 0 1 0): ");
        fflush(stdout);
        count = scanf("%d %d %d %d", &front_sensor_input, &left_sensor_input,
                      &right_sensor_input, &dust_sensor_input);
        if (count == EOF) {
            break;
        }
        finish_line();
        if (count != 4 ||
            (front_sensor_input != 0 && front_sensor_input != 1) ||
            (left_sensor_input != 0 && left_sensor_input != 1) ||
            (right_sensor_input != 0 && right_sensor_input != 1) ||
            (dust_sensor_input != 0 && dust_sensor_input != 1)) {
            printf("Each sensor value must be 0 or 1.\n\n");
            continue;
        }

        tick = 0;
        tick_5 = 0;
        tick_start = 0;

        error = CU_initialize_registry();
        if (error != CUE_SUCCESS) {
            return (int)error;
        }
        suite = CU_add_suite("RVC Controller", NULL, NULL);
        if (suite == NULL ||
            CU_add_test(suite, test_names[choice - 1],
                        test_functions[choice - 1]) == NULL) {
            error = CU_get_error();
            CU_cleanup_registry();
            return (int)error;
        }

        printf("\n--- Test result ---\n");
        CU_basic_set_mode(CU_BRM_VERBOSE);
        error = CU_basic_run_tests();
        failures = CU_get_number_of_failures();
        CU_cleanup_registry();
        if (error != CUE_SUCCESS) {
            return (int)error;
        }
        printf("\nResult: %s (failed assertions: %u)\n",
               failures == 0 ? "PASS" : "FAIL", failures);
        printf("Press Enter to clear the screen and select another test.");
        fflush(stdout);
        if (finish_line() == EOF) {
            break;
        }
        system("cls");
    }

    return EXIT_SUCCESS;
}
