#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <CUnit/Basic.h>

#define main rvc_controller_main
#include "RVC Controller.c"
#undef main

static char output[4096];
static state start_state;

/* controller()는 while(1)이라 끝나지 않아서, 스레드로 실행시키고
   몇 초 기다린 뒤 강제로 종료시켜서 출력만 확인한다. */
static DWORD WINAPI run_controller(LPVOID arg)
{
    controller(start_state);
    return 0;
}

/* controller()를 시작만 해두고 스레드 핸들을 돌려준다.
   리턴받은 handle을 쥐고 있는 동안 센서값을 자유롭게 바꿔가며 Sleep을 반복하면
   중간에 상황이 바뀌는 긴 시나리오를 만들 수 있다. */
static HANDLE begin_run(state s)
{
    tick = 0;
    tick_5 = 0;
    start_state = s;

    freopen("system_test_output.txt", "w", stdout);

    return CreateThread(NULL, 0, run_controller, NULL, 0, NULL);
}

/* 스레드를 강제로 끝내고, 그동안 찍힌 출력을 output에 담는다. */
static void end_run(HANDLE thread)
{
    FILE* f;
    int len;

    TerminateThread(thread, 0);
    CloseHandle(thread);

    freopen("CON", "w", stdout);

    f = fopen("system_test_output.txt", "r");
    len = (int)fread(output, 1, sizeof(output) - 1, f);
    output[len] = '\0';
    fclose(f);
}

static void run_for_ticks(state s, int ticks)
{
    HANDLE thread = begin_run(s);
    Sleep(ticks * 1000 + 500);
    end_run(thread);
}

/* ===================== 앞으로 / 뒤로 / 왼쪽 / 오른쪽 ===================== */

/* 앞으로: 장애물 없으면 전진을 유지한다. */
static void test_forward(void)
{
    front_sensor_input = 0;
    left_sensor_input = 0;
    right_sensor_input = 0;
    dust_sensor_input = 0;

    run_for_ticks(MOVE_FORWARD, 2);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
    CU_ASSERT_PTR_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NULL(strstr(output, "RIGHT"));
    CU_ASSERT_PTR_NULL(strstr(output, "BACKWARD"));
}

/* 뒤로: 사방이 막히면 후진을 시작한다. */
static void test_forward_to_backward(void)
{
    front_sensor_input = 1;
    left_sensor_input = 1;
    right_sensor_input = 1;
    dust_sensor_input = 0;

    run_for_ticks(MOVE_FORWARD, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "OFF"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
}

/* 왼쪽: 전방이 막히고 왼쪽이 비어있으면 좌회전을 시작한다. */
static void test_forward_to_left(void)
{
    front_sensor_input = 1;
    left_sensor_input = 0;
    right_sensor_input = 1;
    dust_sensor_input = 0;

    run_for_ticks(MOVE_FORWARD, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "OFF"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "LEFT"));
}

/* 오른쪽: 전방이 막히고 오른쪽이 비어있으면 우회전을 시작한다. */
static void test_forward_to_right(void)
{
    front_sensor_input = 1;
    left_sensor_input = 1;
    right_sensor_input = 0;
    dust_sensor_input = 0;

    run_for_ticks(MOVE_FORWARD, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "OFF"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "RIGHT"));
}

/* ===================== 먼지 + 앞으로 ===================== */

/* 먼지 + 앞으로: 전방이 비어있는데 먼지가 감지되면 파워업으로 들어간다. */
static void test_dust_to_powerup(void)
{
    front_sensor_input = 0;
    left_sensor_input = 0;
    right_sensor_input = 0;
    dust_sensor_input = 1;

    run_for_ticks(MOVE_FORWARD, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
}

/* ===================== 뒤로이동 + 왼쪽 / 오른쪽 / 뒤로 ===================== */

/* 뒤로이동 + 왼쪽: 후진 중 왼쪽이 비어있으면 좌회전으로 바뀐다. */
static void test_backward_to_left(void)
{
    front_sensor_input = 0;
    left_sensor_input = 0;
    right_sensor_input = 1;
    dust_sensor_input = 0;

    run_for_ticks(MOVE_BACKWARD, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "LEFT"));
}

/* 뒤로이동 + 오른쪽: 후진 중 오른쪽이 비어있으면 우회전으로 바뀐다. */
static void test_backward_to_right(void)
{
    front_sensor_input = 0;
    left_sensor_input = 1;
    right_sensor_input = 0;
    dust_sensor_input = 0;

    run_for_ticks(MOVE_BACKWARD, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "RIGHT"));
}

/* 뒤로이동 + 뒤로: 양쪽 다 막혀있으면 후진을 유지한다. */
static void test_backward_remain(void)
{
    front_sensor_input = 0;
    left_sensor_input = 1;
    right_sensor_input = 1;
    dust_sensor_input = 0;

    run_for_ticks(MOVE_BACKWARD, 2);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
    CU_ASSERT_PTR_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NULL(strstr(output, "RIGHT"));
}

/* ===================== 앞으로 + (먼지 + 앞으로) ===================== */

/* 앞으로 + (먼지 + 앞으로): 파워업이 4tick을 다 채우면 전진으로 복귀한다. */
static void test_powerup_to_forward(void)
{
    front_sensor_input = 0;
    left_sensor_input = 0;
    right_sensor_input = 0;
    dust_sensor_input = 1;

    run_for_ticks(MOVE_FORWARD, 5);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "ON"));
}

/* ===================== 왼쪽 + 앞으로 / 오른쪽 + 앞으로 ===================== */

/* 왼쪽 + 앞으로: 좌회전이 4tick을 다 채우면 전진으로 복귀한다. */
static void test_left_to_forward(void)
{
    front_sensor_input = 1;
    left_sensor_input = 0;
    right_sensor_input = 1;
    dust_sensor_input = 0;

    run_for_ticks(MOVE_FORWARD, 5);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "ON"));
}

/* 오른쪽 + 앞으로: 우회전이 4tick을 다 채우면 전진으로 복귀한다. */
static void test_right_to_forward(void)
{
    front_sensor_input = 1;
    left_sensor_input = 1;
    right_sensor_input = 0;
    dust_sensor_input = 0;

    run_for_ticks(MOVE_FORWARD, 5);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "RIGHT"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "ON"));
}

/* ===================== 앞으로 + (먼지 + 앞으로) + 먼지가 끝나기 전 ... ===================== */

/* 먼지가 끝나기 전 왼쪽: 파워업 4tick이 끝나기 전에 장애물을 만나면
   끝까지 안 기다리고 바로 좌회전으로 빠진다 (ON이 찍히면 안 된다). */
static void test_powerup_interrupted_left(void)
{
    HANDLE thread;

    front_sensor_input = 0;
    left_sensor_input = 0;
    right_sensor_input = 0;
    dust_sensor_input = 1;

    thread = begin_run(MOVE_FORWARD);
    Sleep(1500); /* 파워업 상태에 머무는 중, 아직 4tick 안 됨 */

    front_sensor_input = 1;
    left_sensor_input = 0;
    right_sensor_input = 1;
    Sleep(1000);

    end_run(thread);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "OFF"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NULL(strstr(output, "ON"));
}

/* 먼지가 끝나기 전 오른쪽: 위와 동일하되 우회전으로 빠지는 경우. */
static void test_powerup_interrupted_right(void)
{
    HANDLE thread;

    front_sensor_input = 0;
    left_sensor_input = 0;
    right_sensor_input = 0;
    dust_sensor_input = 1;

    thread = begin_run(MOVE_FORWARD);
    Sleep(1500);

    front_sensor_input = 1;
    left_sensor_input = 1;
    right_sensor_input = 0;
    Sleep(1000);

    end_run(thread);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "OFF"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "RIGHT"));
    CU_ASSERT_PTR_NULL(strstr(output, "ON"));
}

/* 먼지가 끝나기 전 뒤로: 위와 동일하되 후진으로 빠지는 경우. */
static void test_powerup_interrupted_backward(void)
{
    HANDLE thread;

    front_sensor_input = 0;
    left_sensor_input = 0;
    right_sensor_input = 0;
    dust_sensor_input = 1;

    thread = begin_run(MOVE_FORWARD);
    Sleep(1500);

    front_sensor_input = 1;
    left_sensor_input = 1;
    right_sensor_input = 1;
    Sleep(1000);

    end_run(thread);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "OFF"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
    CU_ASSERT_PTR_NULL(strstr(output, "ON"));
}

/* ===================== 긴 시나리오 ===================== */

/* 왼쪽 + 앞으로 + 뒤로: 좌회전 완주 -> 전진 -> 장애물이 사방을 막아 후진까지
   하나의 실행으로 이어서 확인한다. */
static void test_left_forward_backward(void)
{
    HANDLE thread;

    front_sensor_input = 0;
    left_sensor_input = 0;
    right_sensor_input = 0;
    dust_sensor_input = 0;

    thread = begin_run(MOVE_FORWARD);
    Sleep(2000); /* 장애물 없이 2tick 전진 */

    front_sensor_input = 1;
    right_sensor_input = 1; /* 좌측이 비어있어서 좌회전 시작 */
    Sleep(5000); /* 좌회전 4tick이 끝나고 다시 전진으로 돌아올 때까지 */

    front_sensor_input = 0; /* 장애물이 사라짐 */
    Sleep(2000); /* 다시 전진을 유지하는지 확인 */

    end_run(thread);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "ON"));
}

/* 앞으로 + (먼지 + 앞으로) + (5tick 뒤 먼지 + 뒤로) + 왼쪽 + 앞으로 + 먼지:
   전진 -> 파워업 완주 -> 후진 -> 좌회전 -> 전진 -> 파워업 재진입까지
   전부 한 번의 실행으로 이어서, 상태가 여러 번 바뀌어도 꼬이지 않는지 확인. */
static void test_full_cycle(void)
{
    HANDLE thread;

    front_sensor_input = 0;
    left_sensor_input = 0;
    right_sensor_input = 0;
    dust_sensor_input = 0;

    thread = begin_run(MOVE_FORWARD);
    Sleep(1000); /* 장애물 없이 잠깐 전진 */

    dust_sensor_input = 1;
    Sleep(5500); /* 파워업 진입 -> 5tick 다 채우고 전진 복귀까지 */

    front_sensor_input = 1;
    left_sensor_input = 1;
    right_sensor_input = 1;
    dust_sensor_input = 1;
    Sleep(1000); /* 사방이 막히고 먼지도 감지돼서 바로 후진 */

    front_sensor_input = 0; /* 나중에 전진으로 돌아왔을 때를 대비해 미리 치워둠 */
    left_sensor_input = 0;
    right_sensor_input = 1;
    dust_sensor_input = 0;
    Sleep(1000); /* 왼쪽이 열려서 좌회전 시작 */

    Sleep(5000); /* 좌회전 4tick 끝나고 전진 복귀까지 */

    dust_sensor_input = 1;
    Sleep(1500); /* 먼지 재감지 -> 파워업 재진입 */

    end_run(thread);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "ON"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
}

int main(void)
{
    CU_pSuite suite;

    CU_initialize_registry();
    suite = CU_add_suite("RVC Controller System Test", NULL, NULL);

    CU_add_test(suite, "앞으로", test_forward);
    CU_add_test(suite, "뒤로", test_forward_to_backward);
    CU_add_test(suite, "왼쪽", test_forward_to_left);
    CU_add_test(suite, "오른쪽", test_forward_to_right);

    CU_add_test(suite, "먼지 + 앞으로", test_dust_to_powerup);

    CU_add_test(suite, "뒤로이동 + 왼쪽", test_backward_to_left);
    CU_add_test(suite, "뒤로이동 + 오른쪽", test_backward_to_right);
    CU_add_test(suite, "뒤로이동 + 뒤로", test_backward_remain);

    CU_add_test(suite, "앞으로 + (먼지 + 앞으로)", test_powerup_to_forward);

    CU_add_test(suite, "왼쪽 + 앞으로", test_left_to_forward);
    CU_add_test(suite, "오른쪽 + 앞으로", test_right_to_forward);

    CU_add_test(suite, "먼지가 끝나기 전 왼쪽", test_powerup_interrupted_left);
    CU_add_test(suite, "먼지가 끝나기 전 오른쪽", test_powerup_interrupted_right);
    CU_add_test(suite, "먼지가 끝나기 전 뒤로", test_powerup_interrupted_backward);

    CU_add_test(suite, "왼쪽 + 앞으로 + 뒤로", test_left_forward_backward);
    CU_add_test(suite, "앞으로+(먼지+앞으로)+(5tick뒤 먼지+뒤로)+왼쪽+앞으로+먼지", test_full_cycle);

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();

    return 0;
}
