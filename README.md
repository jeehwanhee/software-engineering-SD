# RVC Controller CUnit 테스트

Windows의 Visual Studio에서 `RVC Controller.c`를 CUnit으로 테스트하는 방법입니다.
테스트를 실행하면 검사할 항목과 센서값을 입력할 수 있습니다.

## 1. 준비

- Visual Studio: 설치 관리자의 **C++를 사용한 데스크톱 개발** 워크로드 설치
- MSVC 빌드 도구: 설치한 Visual Studio 버전에 맞는 도구 집합
- Git
- 아래 두 C 파일

```text
RVC Controller.c
test_rvc_controller.c
```

두 C 파일은 같은 폴더에 둡니다. 이 안내는 **Debug / x64** 기준입니다.

## 2. CUnit 설치

vcpkg로 CUnit을 설치합니다. 처음 설치하는 경우 **명령 프롬프트(cmd)**에서 아래 명령을 순서대로 실행합니다. [vcpkg 설치 안내](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-msbuild)

```bat
cd /d "%USERPROFILE%"
git clone https://github.com/microsoft/vcpkg.git
cd /d "%USERPROFILE%\vcpkg"
bootstrap-vcpkg.bat
vcpkg.exe install cunit:x64-windows
```

이미 `%USERPROFILE%\vcpkg`에 vcpkg를 설치했다면 아래 두 명령만 실행합니다. `x64-windows`는 이번 테스트의 대상 플랫폼입니다. [CUnit 패키지](https://vcpkg.io/en/package/cunit.html), [vcpkg 플랫폼 지정 안내](https://learn.microsoft.com/en-us/vcpkg/concepts/triplets)

```bat
cd /d "%USERPROFILE%\vcpkg"
vcpkg.exe install cunit:x64-windows
```

설치가 끝나면 다음 파일들이 있는지 확인합니다.

```text
%USERPROFILE%\vcpkg\installed\x64-windows\
├─ include\CUnit\Basic.h
├─ debug\lib\cunit.lib
├─ debug\bin\cunit.dll
├─ lib\cunit.lib
└─ bin\cunit.dll
```

`include\CUnit`에는 `Basic.h` 외의 CUnit 헤더도 함께 있어야 합니다.
Debug에서는 `debug\lib`와 `debug\bin`의 파일을 사용합니다.

## 3. Visual Studio 프로젝트 열기

### 저장소의 프로젝트 파일을 사용하는 경우

1. `RVC Controller/RVC Controller.slnx`를 Visual Studio에서 엽니다.
2. 상단에서 **Debug / x64**를 선택합니다.
3. `.slnx`를 열 수 없는 버전이면 `RVC Controller/RVC Controller/RVC Controller.vcxproj`를 엽니다.
4. `test_rvc_controller.c`가 소스 파일에 없다면 **추가 → 기존 항목**으로 추가합니다.
5. `RVC Controller.c`의 속성을 열고 **모든 구성 / 모든 플랫폼**에서 **일반 → 빌드에서 제외 → 예**로 설정합니다.
6. 아래 4번의 CUnit 연결 설정을 적용합니다.

프로젝트가 요구하는 빌드 도구가 설치되어 있지 않다면 **프로젝트 속성 → 일반 → 플랫폼 도구 집합**을 설치된 버전으로 선택합니다. 예를 들어 Visual Studio 2022의 도구 집합은 `v143`입니다.

### C 파일 두 개로 새 프로젝트를 만드는 경우

1. Visual Studio에서 C++의 **빈 프로젝트**를 만듭니다. `.c` 파일은 C 소스로 컴파일됩니다.
2. 두 C 파일을 프로젝트 폴더에 복사합니다.
3. 솔루션 탐색기에서 **소스 파일 → 우클릭 → 추가 → 기존 항목**으로 두 파일을 추가합니다.
4. `RVC Controller.c`를 우클릭하고 **속성**을 엽니다.
5. **모든 구성 / 모든 플랫폼**에서 **일반 → 빌드에서 제외 → 예**로 설정합니다.

`test_rvc_controller.c`는 아래와 같이 원본 소스를 직접 포함합니다.
따라서 **`test_rvc_controller.c`만 컴파일**해야 합니다.

```c
#define main rvc_controller_main
#include "RVC Controller.c"
#undef main
```

## 4. CUnit 연결 설정

프로젝트에 CUnit을 연결하려면 다음 설정을 적용합니다.

솔루션 탐색기에서 **프로젝트 우클릭 → 속성**을 엽니다.
속성 창 상단의 **구성은 Debug**, **플랫폼은 x64**로 선택합니다.
아래 값을 기존 설정에 추가합니다.

| 설정 항목 | 추가할 값 |
|---|---|
| C/C++ → 일반 → 추가 포함 디렉터리 | `$(UserProfile)\vcpkg\installed\x64-windows\include` |
| 링커 → 일반 → 추가 라이브러리 디렉터리 | `$(UserProfile)\vcpkg\installed\x64-windows\debug\lib` |
| 링커 → 입력 → 추가 종속성 | `cunit.lib` |

헤더 경로와 라이브러리 경로를 설정하고, 링크할 `.lib` 이름을 추가 종속성에 등록합니다. [Microsoft 라이브러리 연결 안내](https://learn.microsoft.com/en-us/cpp/build/adding-references-in-visual-cpp-projects)

Visual Studio 속성의 `$(UserProfile)`은 현재 사용자의 폴더를 뜻합니다.
다른 위치에 vcpkg를 설치했다면 위 경로를 실제 설치 경로로 바꿉니다.

### cunit.dll 자동 복사

`cunit.lib`는 설치 폴더에 두고 경로를 등록합니다.
`cunit.dll`은 실행할 때 필요하므로 **빌드된 `.exe`와 같은 폴더**에 둡니다.

**빌드 이벤트 → 빌드 후 이벤트 → 명령줄**에 아래 명령을 입력합니다.
빌드가 끝나면 DLL이 실행파일 옆으로 복사됩니다. [Microsoft DLL 복사 안내](https://learn.microsoft.com/en-us/cpp/build/walkthrough-creating-and-using-a-dynamic-link-library-cpp)

```bat
copy /Y "$(UserProfile)\vcpkg\installed\x64-windows\debug\bin\cunit.dll" "$(TargetDir)cunit.dll"
```

## 5. 빌드 및 테스트 실행

1. **Debug / x64**를 선택합니다.
2. **Ctrl + Shift + B**로 빌드합니다.
3. **F5**로 디버깅 실행합니다.
4. 테스트 번호를 선택합니다.
5. 전방, 좌측, 우측, 먼지 값을 **F L R D 순서로** 입력합니다.

센서값은 `0`이면 감지되지 않음, `1`이면 감지됨입니다.
메뉴 옆에 표시된 조건에 맞는 값을 입력합니다. 조건에 표시되지 않은 센서는 `0` 또는 `1`을 사용할 수 있습니다.

예를 들어 전진에서 좌회전으로 변경되는 테스트는 아래와 같이 실행합니다.

```text
Select test: 3
Enter F L R D (example: 1 0 1 0): 1 0 1 0

... CUnit 검사 결과 ...

Result: PASS (failed assertions: 0)
Press Enter to clear the screen and select another test.
```

결과 화면에서 **엔터**를 누르면 화면을 지우고 다시 테스트를 선택합니다.
메뉴에서 **0**을 입력하면 종료합니다.

디버깅하려면 `test_state_tick()`의 아래 줄에 **F9로 중단점**을 설정합니다.

```c
actual_state = controller_step(current_state);
```

F5로 실행한 뒤 카운터를 검사하는 테스트를 선택합니다.
중단점에서 F10으로 한 tick을 처리하고 `actual_state`, `expected_state`, `tick_5`, `expected_tick`을 조사식에서 비교합니다.

## 6. 오류 확인

| 증상 | 확인할 내용 |
|---|---|
| `CUnit/Basic.h`를 찾을 수 없음 | 추가 포함 디렉터리가 `include`까지 지정되어 있는지 확인 |
| `cunit.lib`를 열 수 없음 | 추가 라이브러리 디렉터리에 실제 파일이 있는지 확인 |
| `CU_...` 함수에 대한 외부 기호 오류 | 추가 종속성에 `cunit.lib`가 등록되어 있는지 확인 |
| `cunit.dll`이 없다는 실행 오류 | 실행파일 옆에 DLL이 있는지 확인하고 빌드 후 복사 명령 확인 |
| 함수가 여러 번 정의되었다는 오류 | `RVC Controller.c`가 빌드에서 제외되어 있는지 확인 |
| 라이브러리의 플랫폼이 맞지 않는다는 오류 | 프로젝트의 플랫폼이 x64이고 CUnit도 `x64-windows`인지 확인 |
| 프로젝트에서 요구하는 빌드 도구가 없다는 오류 | 필요한 MSVC 빌드 도구를 설치하거나, 프로젝트 속성의 일반 → 플랫폼 도구 집합을 설치된 버전으로 변경 |
| CUnit 결과가 FAIL | 입력값이 선택한 테스트의 조건과 일치하는지 확인하고, 실패한 assertion의 예상값과 실제 동작 비교 |

Release로 실행할 때는 라이브러리 경로를 `debug\lib`에서 `lib`로, DLL 복사 원본을 `debug\bin`에서 `bin`으로 바꿉니다.
