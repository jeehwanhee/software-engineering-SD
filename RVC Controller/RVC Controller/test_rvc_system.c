#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

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

static HANDLE begin_run(state s)
{
    tick = 0;
    tick_5 = 0;
    start_state = s;

    freopen("system_test_output.txt", "w", stdout);

    return CreateThread(NULL, 0, run_controller, NULL, 0, NULL);
}

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

/* 테스트 이름의 [F L R D, F L R D, ...] 그대로, 한 묶음(bracket)을 1tick씩 순서대로 넣는다. */
typedef struct {
    int f, l, r, d;
} input_t;

static void run_sequence(state start, const input_t* seq, int count)
{
    HANDLE thread;
    int i;

    front_sensor_input = seq[0].f;
    left_sensor_input = seq[0].l;
    right_sensor_input = seq[0].r;
    dust_sensor_input = seq[0].d;

    thread = begin_run(start);

    for (i = 0; i < count; i++) {
        if (i > 0) {
            front_sensor_input = seq[i].f;
            left_sensor_input = seq[i].l;
            right_sensor_input = seq[i].r;
            dust_sensor_input = seq[i].d;
        }
        Sleep(1000);
    }
    Sleep(500); /* 여유 */

    end_run(thread);
}

/* ===================== [F L R D] 단일 입력: 전진 중 8가지 조합 전수 ===================== */

static void test_01(void)
{
    input_t seq[] = { {0,0,0,0} };
    run_sequence(MOVE_FORWARD, seq, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
    CU_ASSERT_PTR_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NULL(strstr(output, "RIGHT"));
    CU_ASSERT_PTR_NULL(strstr(output, "BACKWARD"));
}

static void test_02(void)
{
    input_t seq[] = { {0,1,0,0} };
    run_sequence(MOVE_FORWARD, seq, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
    CU_ASSERT_PTR_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NULL(strstr(output, "RIGHT"));
    CU_ASSERT_PTR_NULL(strstr(output, "BACKWARD"));
}

static void test_03(void)
{
    input_t seq[] = { {0,0,1,0} };
    run_sequence(MOVE_FORWARD, seq, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
    CU_ASSERT_PTR_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NULL(strstr(output, "RIGHT"));
    CU_ASSERT_PTR_NULL(strstr(output, "BACKWARD"));
}

static void test_04(void)
{
    input_t seq[] = { {0,1,1,0} };
    run_sequence(MOVE_FORWARD, seq, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
    CU_ASSERT_PTR_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NULL(strstr(output, "RIGHT"));
    CU_ASSERT_PTR_NULL(strstr(output, "BACKWARD"));
}

static void test_05(void)
{
    input_t seq[] = { {1,0,0,0} };
    run_sequence(MOVE_FORWARD, seq, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "OFF"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "RIGHT"));
    CU_ASSERT_PTR_NULL(strstr(output, "LEFT"));
}

static void test_06(void)
{
    input_t seq[] = { {1,0,1,0} };
    run_sequence(MOVE_FORWARD, seq, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "OFF"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NULL(strstr(output, "RIGHT"));
}

static void test_07(void)
{
    input_t seq[] = { {1,1,0,0} };
    run_sequence(MOVE_FORWARD, seq, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "OFF"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "RIGHT"));
    CU_ASSERT_PTR_NULL(strstr(output, "LEFT"));
}

static void test_08(void)
{
    input_t seq[] = { {1,1,1,0} };
    run_sequence(MOVE_FORWARD, seq, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "OFF"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
}

/* ===================== 후진 진입 이후 ===================== */

static void test_09(void)
{
    input_t seq[] = { {1,1,1,0}, {0,0,1,0} };
    run_sequence(MOVE_FORWARD, seq, 2);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "LEFT"));
}

static void test_10(void)
{
    input_t seq[] = { {1,1,1,0}, {0,1,0,0} };
    run_sequence(MOVE_FORWARD, seq, 2);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "RIGHT"));
}

static void test_11(void)
{
    input_t seq[] = { {1,1,1,0}, {0,1,1,0}, {0,1,1,0} };
    run_sequence(MOVE_FORWARD, seq, 3);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
    CU_ASSERT_PTR_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NULL(strstr(output, "RIGHT"));
}

static void test_12(void)
{
    input_t seq[] = {
        {1,1,1,0}, {0,1,1,0}, {0,0,1,0},
        {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}
    };
    run_sequence(MOVE_FORWARD, seq, 8);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "ON"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
}

static void test_13(void)
{
    input_t seq[] = {
        {1,1,1,0}, {0,1,1,0}, {0,1,0,0},
        {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}
    };
    run_sequence(MOVE_FORWARD, seq, 8);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "RIGHT"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "ON"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
}

/* ===================== 회전 완료 후 전진 ===================== */

static void test_14(void)
{
    input_t seq[] = {
        {1,0,1,0},
        {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}
    };
    run_sequence(MOVE_FORWARD, seq, 5);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "ON"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
}

static void test_15(void)
{
    input_t seq[] = {
        {1,1,0,0},
        {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}
    };
    run_sequence(MOVE_FORWARD, seq, 5);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "RIGHT"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "ON"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
}

/* ===================== 파워업 ===================== */

static void test_16(void)
{
    input_t seq[] = { {0,0,0,1} };
    run_sequence(MOVE_FORWARD, seq, 1);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
}

static void test_17(void)
{
    input_t seq[] = {
        {0,0,0,0}, {0,0,0,1},
        {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}
    };
    run_sequence(MOVE_FORWARD, seq, 6);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "ON"));
}

static void test_18(void)
{
    input_t seq[] = { {0,0,0,1}, {0,0,0,0}, {1,0,1,0} };
    run_sequence(MOVE_FORWARD, seq, 3);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NULL(strstr(output, "ON"));
}

static void test_19(void)
{
    input_t seq[] = { {0,0,0,1}, {0,0,0,0}, {1,1,0,0} };
    run_sequence(MOVE_FORWARD, seq, 3);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "RIGHT"));
    CU_ASSERT_PTR_NULL(strstr(output, "ON"));
}

static void test_20(void)
{
    input_t seq[] = { {0,0,0,1}, {0,0,0,0}, {1,1,1,0} };
    run_sequence(MOVE_FORWARD, seq, 3);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
    CU_ASSERT_PTR_NULL(strstr(output, "ON"));
}

/* 먼지와 장애물이 같은 tick에 동시에 감지되는 경우.
   R=0이면 L값과 무관하게 즉시 우회전 분기가 먼저 걸리므로,
   좌회전을 보려면 R=1(1011)이어야 한다. (원래 1001은 실제로 우회전이 됨) */
static void test_21(void)
{
    input_t seq[] = { {0,0,0,0}, {1,0,1,1} };
    run_sequence(MOVE_FORWARD, seq, 2);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "OFF"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "LEFT"));
}

static void test_22(void)
{
    input_t seq[] = { {0,0,0,0}, {1,1,1,1} };
    run_sequence(MOVE_FORWARD, seq, 2);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "OFF"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
}

/* ===================== 긴 시나리오 ===================== */

static void test_23(void)
{
    input_t seq[] = {
        {0,0,0,0}, {1,0,1,0},
        {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0},
        {1,1,1,0}
    };
    run_sequence(MOVE_FORWARD, seq, 8);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "LEFT"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "ON"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
}

static void test_24(void)
{
    input_t seq[] = {
        {0,0,0,1}, {1,1,1,0}, {1,1,1,0}, {1,1,0,0},
        {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0},
        {0,0,0,1}, {1,1,1,1}
    };
    run_sequence(MOVE_FORWARD, seq, 11);

    CU_ASSERT_PTR_NOT_NULL(strstr(output, "POWER UP"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "BACKWARD"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "RIGHT"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "ON"));
    CU_ASSERT_PTR_NOT_NULL(strstr(output, "FORWARD"));
}

int main(void)
{
    CU_pSuite suite;

    CU_initialize_registry();
    suite = CU_add_suite("RVC Controller System Test", NULL, NULL);

    CU_add_test(suite, "[0000] 전진", test_01);
    CU_add_test(suite, "[0100] 왼쪽 장애물 -> 전진", test_02);
    CU_add_test(suite, "[0010] 오른쪽 장애물 -> 전진", test_03);
    CU_add_test(suite, "[0110] 양쪽 장애물 -> 전진", test_04);
    CU_add_test(suite, "[1000] 전방 장애물 -> 우회전", test_05);
    CU_add_test(suite, "[1010] 오른쪽 장애물 -> 좌회전", test_06);
    CU_add_test(suite, "[1100] 왼쪽 장애물 -> 우회전", test_07);
    CU_add_test(suite, "[1110] 사방 막힘 -> 후진", test_08);

    CU_add_test(suite, "[1110, 0010] 후진 후 좌측 열림 -> 좌회전", test_09);
    CU_add_test(suite, "[1110, 0100] 후진 후 우측 열림 -> 우회전", test_10);
    CU_add_test(suite, "[1110, 0110, 0110] 후진 후 양쪽 막힘 -> 후진 유지", test_11);
    CU_add_test(suite, "[1110, 0110, 0010, 0000, 0000, 0000, 0000, 0000] 후진 유지 후 좌회전", test_12);
    CU_add_test(suite, "[1110, 0110, 0100, 0000, 0000, 0000, 0000, 0000] 후진 유지 후 우회전", test_13);

    CU_add_test(suite, "[1010, 0000, 0000, 0000, 0000] 좌회전 후 전진", test_14);
    CU_add_test(suite, "[1100, 0000, 0000, 0000, 0000] 우회전 후 전진", test_15);

    CU_add_test(suite, "[0001] 먼지 감지 -> 파워업", test_16);
    CU_add_test(suite, "[0000, 0001, 0000, 0000, 0000, 0000] 파워업 이후 ON", test_17);
    CU_add_test(suite, "[0001, 0000, 1010] 파워업 중 좌회전", test_18);
    CU_add_test(suite, "[0001, 0000, 1100] 파워업 중 우회전", test_19);
    CU_add_test(suite, "[0001, 0000, 1110] 파워업 중 후진", test_20);
    CU_add_test(suite, "[0000, 1011] 먼지, 벽 동시 감지 -> 파워업 후 바로 OFF, 좌회전", test_21);
    CU_add_test(suite, "[0000, 1111] 먼지, 벽 동시 감지 -> 파워업 후 바로 OFF, 후진", test_22);

    CU_add_test(suite, "[0000, 1010, 0000, 0000, 0000, 0000, 0000, 1110] 전진 -> 좌회전 -> 전진 -> 후진", test_23);
    CU_add_test(suite, "[0001, 1110, 1110, 1100, 0000, 0000, 0000, 0000, 0000, 0001, 1111] 전진 -> 후진 -> 우회전 -> 전진 -> 파워업 -> 후진", test_24);

    printf("테스트 실행 중... (완료까지 1~2분 걸릴 수 있습니다)\n");

    CU_set_output_filename("rvc_system_test_report");
    CU_automated_run_tests();
    CU_cleanup_registry();

    printf("완료. rvc_system_test_report-Results.xml 파일을 확인하세요.\n");

    return 0;
}
