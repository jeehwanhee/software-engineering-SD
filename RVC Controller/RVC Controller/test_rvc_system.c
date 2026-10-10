#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <setjmp.h>
#include <io.h>
#include <CUnit/Basic.h>

static clock_t scenario_clock(void);
static int controller_output(const char* format, ...);

/* Keep GitHub main's controller unchanged. Only its clock/output are adapted here. */
#define clock scenario_clock
#define printf controller_output
#define main rvc_controller_main
#include "RVC Controller.c"
#undef main
#undef printf
#undef clock

typedef enum { OUTPUT_MOTOR, OUTPUT_CLEANER } output_device;
typedef struct { output_device device; int command; } output_command;
typedef struct { int front, left, right, dust; } scenario_input;
typedef struct {
    const char* description;
    scenario_input input;
    direction expected_motor;
    clean expected_cleaner;
    unsigned int expected_count;
    output_command expected_commands[4];
} scenario_step;
typedef struct {
    const scenario_step* steps;
    size_t count;
    size_t completed;
} scenario_source;

#define MOTOR(command) { OUTPUT_MOTOR, (int)(command) }
#define CLEANER(command) { OUTPUT_CLEANER, (int)(command) }
#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

static output_command actual_commands[4];
static unsigned int actual_command_count;
static direction active_motor;
static clean active_cleaner;
static scenario_source* running_source;
static int at_tick_start;
static jmp_buf scenario_end;
static const char* motor_names[] = { "FORWARD", "LEFT", "RIGHT", "BACKWARD" };
static const char* cleaner_names[] = { "OFF", "ON", "POWER UP" };

static void record_command(output_device device, int command)
{
    if (actual_command_count < ARRAY_COUNT(actual_commands)) {
        actual_commands[actual_command_count].device = device;
        actual_commands[actual_command_count].command = command;
    }
    actual_command_count++;
    if (device == OUTPUT_MOTOR) {
        active_motor = (direction)command;
    } else {
        active_cleaner = (clean)command;
    }
}

static int controller_output(const char* format, ...)
{
    int result;
    va_list arguments;
    va_start(arguments, format);
    result = vprintf(format, arguments);
    va_end(arguments);
    if (strcmp(format, "Dust Sensor Input: %d\n") == 0) {
        printf("\n");
    }
    if (running_source != NULL) {
        if (strcmp(format, "FORWARD\n") == 0) record_command(OUTPUT_MOTOR, FORWARD);
        else if (strcmp(format, "LEFT\n") == 0) record_command(OUTPUT_MOTOR, LEFT);
        else if (strcmp(format, "RIGHT\n") == 0) record_command(OUTPUT_MOTOR, RIGHT);
        else if (strcmp(format, "BACKWARD\n") == 0) record_command(OUTPUT_MOTOR, BACKWARD);
        else if (strcmp(format, "OFF\n") == 0) record_command(OUTPUT_CLEANER, OFF);
        else if (strcmp(format, "ON\n") == 0) record_command(OUTPUT_CLEANER, ON);
        else if (strcmp(format, "POWER UP\n") == 0) record_command(OUTPUT_CLEANER, POWER_UP_COMMAND);
        else if (strcmp(format, "WRONG COMMAND\n") == 0) record_command(OUTPUT_MOTOR, -1);
    }
    return result;
}

static void check_tick(scenario_source* source)
{
    const scenario_step* step = &source->steps[source->completed];
    unsigned int j;
    int mismatch = actual_command_count != step->expected_count ||
                   active_motor != step->expected_motor || active_cleaner != step->expected_cleaner;
    printf("Current State: %s | Cleaner: %s\n",
           active_motor >= FORWARD && active_motor <= BACKWARD ? motor_names[active_motor] : "INVALID",
           active_cleaner >= OFF && active_cleaner <= POWER_UP_COMMAND ? cleaner_names[active_cleaner] : "INVALID");
    CU_ASSERT_EQUAL(actual_command_count, step->expected_count);
    CU_ASSERT_EQUAL(active_motor, step->expected_motor);
    CU_ASSERT_EQUAL(active_cleaner, step->expected_cleaner);
    CU_ASSERT(actual_command_count <= ARRAY_COUNT(actual_commands));
    for (j = 0; j < step->expected_count && j < actual_command_count && j < ARRAY_COUNT(actual_commands); j++) {
        CU_ASSERT_EQUAL(actual_commands[j].device, step->expected_commands[j].device);
        CU_ASSERT_EQUAL(actual_commands[j].command, step->expected_commands[j].command);
        if (actual_commands[j].device != step->expected_commands[j].device ||
            actual_commands[j].command != step->expected_commands[j].command) mismatch = 1;
    }
    if (mismatch) {
        printf("Mismatch at tick %u (%s)\nExpected active motor/cleaner: %d/%d; actual: %d/%d\n",
               (unsigned int)(source->completed + 1), step->description,
               (int)step->expected_motor, (int)step->expected_cleaner,
               (int)active_motor, (int)active_cleaner);
        printf("Expected commands:");
        for (j = 0; j < step->expected_count; j++) {
            printf(" %s:%d", step->expected_commands[j].device == OUTPUT_MOTOR ? "motor" : "cleaner",
                   step->expected_commands[j].command);
        }
        printf("\nActual commands:");
        for (j = 0; j < actual_command_count && j < ARRAY_COUNT(actual_commands); j++) {
            printf(" %s:%d", actual_commands[j].device == OUTPUT_MOTOR ? "motor" : "cleaner",
                   actual_commands[j].command);
        }
        printf("\n");
    }
}

/* Main calls clock at the start of a tick and after the completed FSM action.
 * Stop only at that completed-tick boundary; no controller_step or worker thread.
 */
static clock_t scenario_clock(void)
{
    scenario_source* source = running_source;
    if (source == NULL) return clock();
    if (at_tick_start) {
        const scenario_step* step = &source->steps[source->completed];
        front_sensor_input = step->input.front;
        left_sensor_input = step->input.left;
        right_sensor_input = step->input.right;
        dust_sensor_input = step->input.dust;
        actual_command_count = 0;
        printf("\n[Tick %u @ %u ms] %s\n", (unsigned int)(source->completed + 1),
               (unsigned int)source->completed * 1000, step->description);
        at_tick_start = 0;
        return (clock_t)(source->completed * CLOCKS_PER_SEC * TICK);
    }
    check_tick(source);
    source->completed++;
    if (source->completed == source->count) {
        longjmp(scenario_end, 1);
    }
    at_tick_start = 1;
    return (clock_t)(source->completed * CLOCKS_PER_SEC * TICK);
}

static void run_scenario(const scenario_step* steps, size_t count)
{
    /* Heap storage remains valid after longjmp; stop follows completed malloc/free. */
    scenario_source* source = (scenario_source*)calloc(1, sizeof(*source));
    if (source == NULL || count == 0) {
        free(source);
        CU_FAIL("Cannot initialize scenario input");
        return;
    }
    source->steps = steps;
    source->count = count;
    running_source = source;
    active_motor = FORWARD;
    active_cleaner = OFF;
    tick = 0;
    tick_5 = 0;
    at_tick_start = 1;
    if (setjmp(scenario_end) == 0) {
        controller(MOVE_FORWARD);
        CU_FAIL("Main controller returned before the scenario ended");
    }
    running_source = NULL;
    CU_ASSERT_EQUAL(source->completed, count);
    printf("\nScenario stopped after completed tick: %u/%u ticks checked\n",
           (unsigned int)source->completed, (unsigned int)count);
    free(source);
}

static void test_system_complete_journey(void)
{
    const scenario_step steps[] = {
        { "[Phase 01] Normal forward cleaning", {0,0,0,0}, FORWARD,ON,2,{CLEANER(ON),MOTOR(FORWARD)} },
        { "Side obstacles do not block forward", {0,1,1,0}, FORWARD,ON,2,{CLEANER(ON),MOTOR(FORWARD)} },
        { "[Phase 02] Obstacle starts left turn, turn tick 1", {1,0,1,0}, LEFT,OFF,2,{CLEANER(OFF),MOTOR(LEFT)} },
        { "Turn tick 2: opposite-direction obstacle", {1,1,0,0}, LEFT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Turn tick 3: all obstacles and dust", {1,1,1,1}, LEFT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Turn tick 4: all inputs clear", {0,0,0,0}, LEFT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Turn tick 5: resume forward and cleaner ON", {0,0,0,0}, FORWARD,ON,2,{MOTOR(FORWARD),CLEANER(ON)} },
        { "[Phase 03] Both routes open: right turn has priority", {1,0,0,0}, RIGHT,OFF,2,{CLEANER(OFF),MOTOR(RIGHT)} },
        { "Turn tick 2: opposite-direction obstacle", {1,0,1,0}, RIGHT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Turn tick 3: all obstacles and dust", {1,1,1,1}, RIGHT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Turn tick 4: dust only", {0,0,0,1}, RIGHT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Turn tick 5: resume normal cleaning", {0,0,0,0}, FORWARD,ON,2,{MOTOR(FORWARD),CLEANER(ON)} },
        { "[Phase 04] All routes blocked: backward", {1,1,1,0}, BACKWARD,OFF,2,{CLEANER(OFF),MOTOR(BACKWARD)} },
        { "Backward ignores front/dust when both sides blocked", {0,1,1,1}, BACKWARD,OFF,1,{MOTOR(BACKWARD)} },
        { "Left route opens: start left turn", {0,0,1,0}, LEFT,OFF,1,{MOTOR(LEFT)} },
        { "Hold left tick 2", {0,0,0,0}, LEFT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Hold left tick 3", {0,0,0,0}, LEFT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Hold left tick 4", {0,0,0,0}, LEFT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Left tick 5: return to forward", {0,0,0,0}, FORWARD,ON,2,{MOTOR(FORWARD),CLEANER(ON)} },
        { "[Phase 05] Reverse again", {1,1,1,0}, BACKWARD,OFF,2,{CLEANER(OFF),MOTOR(BACKWARD)} },
        { "Right route opens: start right turn", {0,1,0,0}, RIGHT,OFF,1,{MOTOR(RIGHT)} },
        { "Hold right tick 2", {0,0,0,0}, RIGHT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Hold right tick 3", {0,0,0,0}, RIGHT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Hold right tick 4", {0,0,0,0}, RIGHT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Right tick 5: return to forward", {0,0,0,0}, FORWARD,ON,2,{MOTOR(FORWARD),CLEANER(ON)} },
        { "[Phase 06] Dust starts power-up tick 1", {0,0,0,1}, FORWARD,POWER_UP_COMMAND,2,{CLEANER(POWER_UP_COMMAND),MOTOR(FORWARD)} },
        { "Power tick 2: dust clears", {0,0,0,0}, FORWARD,POWER_UP_COMMAND,1,{MOTOR(FORWARD)} },
        { "Power tick 3: side inputs change", {0,1,0,0}, FORWARD,POWER_UP_COMMAND,1,{MOTOR(FORWARD)} },
        { "Power tick 4: keep power", {0,0,1,0}, FORWARD,POWER_UP_COMMAND,1,{MOTOR(FORWARD)} },
        { "Power tick 5: restore normal cleaner", {0,0,0,0}, FORWARD,ON,2,{CLEANER(ON),MOTOR(FORWARD)} },
        { "[Phase 07] Persistent dust starts power-up", {0,0,0,1}, FORWARD,POWER_UP_COMMAND,2,{CLEANER(POWER_UP_COMMAND),MOTOR(FORWARD)} },
        { "Persistent dust: power tick 2", {0,0,0,1}, FORWARD,POWER_UP_COMMAND,1,{MOTOR(FORWARD)} },
        { "Persistent dust: power tick 3", {0,0,0,1}, FORWARD,POWER_UP_COMMAND,1,{MOTOR(FORWARD)} },
        { "Persistent dust: power tick 4", {0,0,0,1}, FORWARD,POWER_UP_COMMAND,1,{MOTOR(FORWARD)} },
        { "Persistent dust: normal output at tick 5", {0,0,0,1}, FORWARD,ON,2,{CLEANER(ON),MOTOR(FORWARD)} },
        { "Next tick starts a new power cycle", {0,0,0,1}, FORWARD,POWER_UP_COMMAND,2,{CLEANER(POWER_UP_COMMAND),MOTOR(FORWARD)} },
        { "[Phase 08] Obstacle interrupts power with left turn", {1,0,1,0}, LEFT,OFF,2,{CLEANER(OFF),MOTOR(LEFT)} },
        { "New left counter: tick 2", {0,0,0,0}, LEFT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "New left counter: tick 3", {0,0,0,0}, LEFT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "New left counter: tick 4", {0,0,0,0}, LEFT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "New left counter: tick 5", {0,0,0,0}, FORWARD,ON,2,{MOTOR(FORWARD),CLEANER(ON)} },
        { "[Phase 09] Start power-up before right obstacle", {0,0,0,1}, FORWARD,POWER_UP_COMMAND,2,{CLEANER(POWER_UP_COMMAND),MOTOR(FORWARD)} },
        { "Advance power to tick 2", {0,0,0,1}, FORWARD,POWER_UP_COMMAND,1,{MOTOR(FORWARD)} },
        { "Obstacle interrupts power with right turn", {1,1,0,0}, RIGHT,OFF,2,{CLEANER(OFF),MOTOR(RIGHT)} },
        { "New right counter: tick 2", {0,0,0,0}, RIGHT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "New right counter: tick 3", {0,0,0,0}, RIGHT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "New right counter: tick 4", {0,0,0,0}, RIGHT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "New right counter: tick 5", {0,0,0,0}, FORWARD,ON,2,{MOTOR(FORWARD),CLEANER(ON)} },
        { "[Phase 10] Start power-up before blocked routes", {0,0,0,1}, FORWARD,POWER_UP_COMMAND,2,{CLEANER(POWER_UP_COMMAND),MOTOR(FORWARD)} },
        { "All routes blocked: interrupt power with backward", {1,1,1,0}, BACKWARD,OFF,2,{CLEANER(OFF),MOTOR(BACKWARD)} },
        { "Keep reversing", {0,1,1,0}, BACKWARD,OFF,1,{MOTOR(BACKWARD)} },
        { "Right route opens", {0,1,0,0}, RIGHT,OFF,1,{MOTOR(RIGHT)} },
        { "Final right tick 2", {0,0,0,0}, RIGHT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Final right tick 3", {0,0,0,0}, RIGHT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Final right tick 4", {0,0,0,0}, RIGHT,OFF,0,{{OUTPUT_MOTOR,0}} },
        { "Final right tick 5", {0,0,0,0}, FORWARD,ON,2,{MOTOR(FORWARD),CLEANER(ON)} },
        { "Verify normal cleaning before scenario end", {0,0,0,0}, FORWARD,ON,2,{CLEANER(ON),MOTOR(FORWARD)} }
    };
    printf("\nMain-based journey: 10 phases, %u virtual ticks\n", (unsigned int)ARRAY_COUNT(steps));
    run_scenario(steps, ARRAY_COUNT(steps));
}

typedef struct {
    const char* id;
    const char* name;
    CU_TestFunc function;
} test_entry;

static const test_entry system_test = {
    "journey", "Complete journey through all 10 phases", test_system_complete_journey
};

static int run_cunit_tests(const test_entry* tests, size_t test_count, const char* suite_name)
{
    CU_ErrorCode error;
    CU_pSuite suite;
    unsigned int failures;
    size_t i;

    error = CU_initialize_registry();
    if (error != CUE_SUCCESS) {
        return EXIT_FAILURE;
    }
    suite = CU_add_suite(suite_name, NULL, NULL);
    if (suite == NULL) {
        CU_cleanup_registry();
        return EXIT_FAILURE;
    }
    for (i = 0; i < test_count; i++) {
        if (CU_add_test(suite, tests[i].name, tests[i].function) == NULL) {
            CU_cleanup_registry();
            return EXIT_FAILURE;
        }
    }
    printf("\n--- Test result ---\n");
    CU_basic_set_mode(CU_BRM_VERBOSE);
    error = CU_basic_run_tests();
    failures = CU_get_number_of_failures();
    printf("\nResult: %s (tests: %u, failed assertions: %u)\n",
           error == CUE_SUCCESS && failures == 0 ? "PASS" : "FAIL",
           CU_get_number_of_tests_run(), failures);
    CU_cleanup_registry();
    return error == CUE_SUCCESS && failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* Capture the complete CUnit/controller output, then show the same run on stdout. */
static int run_with_output_file(const test_entry* selected)
{
    FILE* output_file;
    int stdout_fd = _fileno(stdout);
    int saved_stdout;
    int result;
    char buffer[4096];
    size_t count;

    if (fflush(stdout) != 0) {
        perror("Cannot flush console output");
        return EXIT_FAILURE;
    }
    output_file = fopen("output.txt", "w+");
    if (output_file == NULL) {
        perror("Cannot create output.txt in the current working directory");
        return EXIT_FAILURE;
    }
    saved_stdout = _dup(stdout_fd);
    if (saved_stdout == -1) {
        perror("Cannot preserve console output");
        fclose(output_file);
        return EXIT_FAILURE;
    }
    if (_dup2(_fileno(output_file), stdout_fd) != 0) {
        perror("Cannot redirect test output");
        _close(saved_stdout);
        fclose(output_file);
        return EXIT_FAILURE;
    }

    printf("Selected scenario: %s - %s\n", selected->id, selected->name);
    result = run_cunit_tests(selected, 1,
                            "RVC main software system scenario (virtual ticks)");
    if (fflush(stdout) != 0 || ferror(stdout)) {
        fprintf(stderr, "Cannot write the complete test output to output.txt\n");
        result = EXIT_FAILURE;
    }
    if (_dup2(saved_stdout, stdout_fd) != 0) {
        perror("Cannot restore console output");
        _close(saved_stdout);
        fclose(output_file);
        return EXIT_FAILURE;
    }
    _close(saved_stdout);
    clearerr(stdout);

    if (fseek(output_file, 0, SEEK_SET) != 0) {
        perror("Cannot read output.txt");
        fclose(output_file);
        return EXIT_FAILURE;
    }
    while ((count = fread(buffer, 1, sizeof(buffer), output_file)) != 0) {
        if (fwrite(buffer, 1, count, stdout) != count) {
            result = EXIT_FAILURE;
            break;
        }
    }
    if (ferror(output_file)) {
        fprintf(stderr, "Cannot read the complete output.txt\n");
        result = EXIT_FAILURE;
    }
    if (fclose(output_file) != 0 || fflush(stdout) != 0) {
        result = EXIT_FAILURE;
    }
    return result;
}

/* Visual Studio F5/Ctrl+F5 entry: run the single journey automatically. */
int main(void)
{
    char line[128];
    int result;
    printf("Running system scenario 1: complete journey through all 10 phases.\n");
    result = run_with_output_file(&system_test);
    printf("\nTest finished. Press Enter to close...");
    fflush(stdout);
    (void)fgets(line, sizeof(line), stdin);
    return result;
}
