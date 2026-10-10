# RVC Controller — Visual Studio CUnit 시스템 테스트

GitHub main의 `415e621` 커밋에 있는 `RVC Controller.c`를 원본 그대로 사용합니다. `test_rvc_system.c`는 **1번의 긴 시나리오 하나만** 실행합니다. 번호 선택이나 실행 인자 없이 Visual Studio에서 실행하면 바로 테스트합니다.

## Visual Studio 실행

1. `RVC Controller/RVC Controller/RVC Controller.vcxproj`를 Visual Studio에서 엽니다.
2. 솔루션을 열었다면 해당 프로젝트를 시작 프로젝트로 설정합니다.
3. **Debug / x64**를 선택하고 **F5 또는 Ctrl+F5**를 누릅니다. 변경한 코드가 있으면 Visual Studio가 컴파일한 뒤 실행합니다.
4. 콘솔에서 입력과 CUnit PASS/FAIL 결과를 확인합니다. 마지막에 Enter를 누르면 종료합니다.

Visual Studio 2022의 **C++를 사용한 데스크톱 개발** 도구와 v143 도구 집합을 사용합니다. 실행 대상은 `$(TargetPath)`, 실행 인자는 비워 두고, 작업 디렉터리는 `$(ProjectDir)`로 설정되어 있습니다.

**컴파일 대상은 `test_rvc_system.c` 하나입니다.** 이 파일이 `RVC Controller.c`를 직접 포함하므로 원본은 별도 컴파일에서 제외합니다. 기존 `test_rvc_controller.c`도 제외되어 있습니다.

## CUnit 연결

프로젝트는 다음 경로에서 CUnit을 찾고, 컴파일 후 실행에 필요한 DLL을 자동 복사합니다.

| Debug / x64 설정 | 경로 또는 값 |
|---|---|
| 추가 포함 디렉터리 | `$(UserProfile)\vcpkg\installed\x64-windows\include` |
| 추가 라이브러리 디렉터리 | `$(UserProfile)\vcpkg\installed\x64-windows\debug\lib` |
| 추가 종속성 | `cunit.lib` |
| 자동 복사하는 DLL | `$(UserProfile)\vcpkg\installed\x64-windows\debug\bin\cunit.dll` |

다른 사람이 실행하려면 CUnit을 설치해야 합니다. 기본 경로는 사용자 폴더의 vcpkg 설치 위치이며, vcpkg에서 `cunit:x64-windows` 패키지를 설치하면 됩니다. 다른 위치에 설치했다면 프로젝트의 `CUnitRoot`를 실제 `installed/x64-windows` 위치로 지정합니다. Release는 같은 위치의 `lib`와 `bin`을 사용하고, Win32에는 x86용 CUnit이 필요합니다.

새 프로젝트를 직접 만든다면 두 C 파일을 같은 폴더에 두고 `test_rvc_system.c`만 컴파일합니다. CUnit 헤더/라이브러리 경로와 `cunit.lib`를 연결하고, DLL을 실행 파일 폴더에 배치합니다. 작업 디렉터리를 `$(ProjectDir)`로 설정하면 프로젝트 폴더에 결과가 저장됩니다.

## 단일 시나리오의 검사 내용

총 **10개 구간, 57개 가상 tick**을 하나의 실행으로 연결합니다. 구간 사이에 컨트롤러를 다시 시작하지 않으며, `controller(MOVE_FORWARD)`를 한 번만 호출합니다.

| 구간 | 검사 내용 |
|---|---|
| 1 | 정상 전진·청소 ON, 앞이 열려 있으면 양옆 장애물이 있어도 전진 |
| 2 | 앞·오른쪽 장애물로 좌회전, 회전 중 입력 변경에도 유지, tick 5 정상 복귀 |
| 3 | 앞이 막히고 양옆이 열리면 우회전 우선, 회전 유지와 tick 5 복귀 |
| 4 | 세 방향 장애물로 후진, 앞·먼지 입력이 바뀌어도 양옆이 막히면 후진 유지, 왼쪽이 열리면 좌회전·복귀 |
| 5 | 후진 중 오른쪽이 열리면 우회전·복귀 |
| 6 | 먼지로 파워업, 먼지가 사라져도 기간 유지, 파워업 tick 5 청소 ON |
| 7 | 먼지가 계속 있으면 파워업 종료 후 다음 tick에서 재진입 |
| 8 | 파워업 진입 후 다음 tick에서 앞·오른쪽 장애물을 공급해 OFF·좌회전, 새 회전 기간과 복귀 |
| 9 | 파워업 tick 2 이후 다음 tick에서 앞·왼쪽 장애물을 공급해 OFF·우회전, 카운터 새 시작과 복귀 |
| 10 | 파워업 중 세 방향 장애물로 OFF·후진, 오른쪽이 열리면 우회전 후 정상 청소 |

각 tick에서 **유지 중인 이동 방향/청소 상태와 새 명령의 개수·순서**를 기대값과 비교합니다. 회전·파워업에 진입한 tick을 tick 1로 셉니다.

이 테스트의 PASS는 위 시나리오에 정의한 입력과 발생 시점에서 기대 동작을 만족했다는 의미입니다. 모든 F/L/R/D 조합이나 모든 입력 이력을 검증하지는 않습니다. 특히 파워업→좌회전 구간은 파워업 진입 직후이므로, 경과 카운터가 0인 경우를 검사합니다.

## main과 테스트의 시간 처리

main 원본은 무한 루프에서 센서를 **tick마다 읽습니다.** 장애물 입력은 다음 센서 검사 tick에서 반영되며, 하드웨어 인터럽트나 tick 사이의 즉시 반응을 검사하지 않습니다.

테스트에서만 원본의 `clock()`을 가상 시계로 치환합니다. tick 시작에 F/L/R/D를 공급하고 FSM 처리가 끝난 경계에서 결과를 검사합니다. 실제로 1초씩 기다리지 않고 원본의 tick 계산과 상태 전이를 실행합니다. `controller_step()`은 사용하지 않습니다.

마지막 tick 검사가 끝나면 테스트의 `setjmp/longjmp`로 무한 루프 실행을 종료합니다. 원본에 정상 종료 기능을 추가한 것은 아닙니다. 센서 메모리 해제가 끝난 tick 경계에서만 테스트를 멈춥니다.

명령은 원본의 출력 호출을 기록해 검사하므로 출력 문구나 `clock()` 호출 구조가 바뀌면 테스트 연결부도 검토해야 합니다. 실제 센서·모터 및 실시간 주기 정확도는 이 실행으로 검증하지 않습니다.

## 결과와 입력 수정

전체 결과는 프로젝트 폴더의 **`RVC Controller/RVC Controller/output.txt`**에 저장되고, 같은 로그가 콘솔에 표시됩니다. 실행할 때마다 이전 파일을 덮어씁니다. 솔루션 탐색기에 보이지 않으면 **모든 파일 표시**를 켜거나 파일 탐색기로 프로젝트 폴더를 엽니다. `system-results.txt`는 별도로 생성하지 않습니다.

입력 출력 뒤에는 빈 줄이 있고, `Current State: LEFT | Cleaner: OFF`처럼 유지 상태를 표시합니다. 청소 상태는 `ON`, `OFF`, `POWER UP`입니다. 활동 변경 구분선은 사용하지 않습니다.

`test_system_complete_journey()`의 `scenario_step steps[]`에서 한 행을 한 tick으로 정의합니다.

```c
{ "Left avoidance", {1,0,1,0}, LEFT,OFF,2,
  {CLEANER(OFF), MOTOR(LEFT)} },
{ "Keep left despite dust and opposite obstacle", {1,1,0,1}, LEFT,OFF,0,
  {{OUTPUT_MOTOR,0}} },
```

순서는 설명, 입력 `{F,L,R,D}`, 유지 모터/청소 상태, 새 명령 개수, 새 명령 순서입니다. 센서 값은 `0`(미감지) 또는 `1`(감지)입니다. 명령 개수가 0이면 이전 출력 상태를 유지하는지 검사합니다.

입력을 수정하면 의도한 동작에 맞는 기대값도 직접 작성해야 합니다. 기대값은 실제 출력에서 자동 생성하지 않습니다. 수정 후 Visual Studio에서 다시 F5 또는 Ctrl+F5로 실행합니다.

정상 결과 예시:

```text
Main-based journey: 10 phases, 57 virtual ticks
...
Scenario stopped after completed tick: 57/57 ticks checked
Result: PASS (tests: 1, failed assertions: 0)
```
