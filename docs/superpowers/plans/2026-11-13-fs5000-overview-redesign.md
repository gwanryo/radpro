# FS-5000 통합 화면 개편 구현 계획

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** FS-5000 측정 화면을 "통합 화면 하나에 두 쪽"(1쪽 값, 2쪽 그래프)으로 바꾸고, 상단 바를 14 px 글꼴 24 px 높이로 줄인다.

**Architecture:** 그리기와 데이터 수집을 나눈다. `ui/overview.c`는 구조체로 받은 문자열·값만 그린다(PC 하네스에서 그대로 렌더링 가능). `measurements/overview.c`는 측정 모듈의 접근 함수로 구조체를 채우고 키를 처리한다. 그래프 세로축 계산은 표시 장치와 무관한 순수 함수(`ui/historyaxis.c`)로 떼어 PC에서 단위 테스트한다. 모든 변경은 `FS5000` 조건 안에 둔다.

**Tech Stack:** C (gnu11), STM32L431 + PlatformIO, mcu-renderer(ST7789), Python 글꼴 도구(`tools/build-fonts.py`, freetype-py, bdflib), PC 검증용 WinLibs gcc(이미 PATH에 있음).

**Spec:** `docs/superpowers/specs/2026-11-13-fs5000-overview-redesign-design.md` — 시안 `docs/mockups/overview-v4.html`이 좌표·색의 기준.

## Global Constraints

- 모든 펌웨어 변경은 `#if defined(FS5000)` 안. GMC-800(`DISPLAY_240X320` + `DISPLAY_125PPI` + `VOICE`)과 다른 기기의 빌드 결과는 바뀌지 않는다.
- 한국어 펌웨어만 빌드한다. `tools/build-env.py`에서 `bosean-fs5000` 환경은 `ko`만 생성.
- 한국어 빌드 플래시 사용량 ≤ 81,920 B (`FIRMWARE_SIZE=0x14000`). 시작 시점 65,552 B.
- 빌드 경고 0.
- 소스 줄 끝 CRLF 유지. 파이썬으로 고칠 때 `newline=''`. 고친 뒤 `git diff --stat`으로 파일 전체가 바뀌지 않았는지 확인.
- 주석을 새로 달지 않는다(사용자 전역 규칙). 기존 스타일: 중괄호 줄바꿈, 4칸 들여쓰기.
- 새 문자열은 `platform.io/src/system/strings/*.h` 30개 파일 모두에. ko·en만 번역, 나머지는 en 문장.
- 펌웨어 그리기는 지우기가 없다: 텍스트·사각형이 덮는 영역만 바뀐다. 각 화면은 y 24~320, x 0~240을 빈틈없이 덮어야 한다.
- 빌드 명령: `cd platform.io && C:/Users/USER/.radpro-pio/Scripts/pio run -e bosean-fs5000-ko`
- 작업 브랜치 `feature/fs5000-overview-v2`, 태스크마다 커밋.

## Review Focus

1. 큰 값과 접두어 변화: 순간 `12.47 mSv/h`, `1234 kcpm` 같은 값이 1쪽 숫자 칸(오른쪽 단위 열과 겹치지 않음)과 2쪽 칸(라벨 `순간 mSv/h`)에 들어가야 한다. → Task 6·7 하네스 사례.
2. RTC 미설정: 상단 바 시각 없음(평균 방식만), 시작 `--`. → Task 4·6 사례.
3. 그래프 데이터 없음/값 하나: 프레임만 그리고 라벨·경고선 없이 정상. → Task 2 단위 테스트, Task 7 사례.
4. 경과 1000시간 이상(`1234:56:12`)과 12시간제 시각에서 시작·경과 줄이 겹치지 않음. → Task 6 사례.
5. 튜브 고장·선량 경보: 1쪽 막대 자리 `고장`, 누적 선량 줄 경보 색, 2쪽 보조 줄 `고장`. → Task 6·7 사례.

---

### Task 1: PC 렌더링 하네스와 테스트 실행기

기존 하네스(`docs/handoff/attachments/harness/harness.c`, macOS clang 기준)를 이 Windows PC의 gcc로 옮기고 PNG를 바로 만든다. 이후 모든 UI 태스크의 검증 수단이다.

**Files:**
- Create: `tools/host/harness.c` (기존 하네스에서 시작), `tools/host/render.sh`, `tools/host/ppm2png.py`, `tools/host/run-tests.sh`
- Output: `tools/host/out/` (gitignore에 추가: `tools/host/out/`)

**Interfaces:**
- Produces: `bash tools/host/render.sh` → `tools/host/out/*.png` 생성, 이미지마다 `name: magenta=N` 한 줄 출력, 자홍색이 하나라도 있으면 종료 코드 1. `bash tools/host/run-tests.sh` → `tools/host/test_*.c`를 각각 컴파일·실행, 하나라도 실패하면 종료 코드 1.
- `harness.c`의 사례 추가 방식: `static void render(const char *name, void (*draw)(void))` — 화면을 자홍색(0xF81F)으로 칠하고 `draw()` 후 PPM 저장. 이후 태스크는 이 함수에 사례를 추가한다.

- [ ] **Step 1: 브랜치 확인** — 메인 세션이 만든 `feature/fs5000-overview-v2`에 있는지 `git branch --show-current`로 확인.
- [ ] **Step 2: 하네스 이식** — `render.sh`가 핸드오프 문서의 clang 명령과 같은 정의·소스로 `gcc -std=gnu11 -w -DSIMULATOR -DFS5000 ...`을 실행(`-DFS5000` 추가). 출력 폴더는 `tools/host/out`. `/tmp` 사용 금지. `ppm2png.py`는 표준 라이브러리(`zlib`, `struct`)만으로 PPM→PNG 변환하고 자홍색 픽셀 수를 센다.
- [ ] **Step 3: 실행 확인** — Run: `bash tools/host/render.sh` / Expected: 기존 사례(`overview-day`, `overview-night`, `overview-cpm`, `overview-fault`, `strings`) PNG 생성, 통합 화면 사례 `magenta=0`. (`strings`처럼 일부만 그리는 사례는 자홍색 검사 제외 목록에 둔다.)
- [ ] **Step 4: 테스트 실행기** — `run-tests.sh`는 `tools/host/test_*.c` 각각을 `gcc -std=gnu11 -Wall -Werror -I platform.io/src <test> <대상 소스> -lm`으로 빌드·실행. 대상 소스는 테스트 파일 첫 줄 주석 `// SOURCES: ui/historyaxis.c`에서 읽는다(테스트 파일 안의 주석은 허용). 테스트가 아직 없으면 "no tests"를 출력하고 0으로 끝난다.
- [ ] **Step 5: 커밋** — `git add tools/host .gitignore && git commit -m "build: add Windows host render harness and test runner"`

### Task 2: 그래프 세로축 계산 (순수 함수, TDD)

**Files:**
- Create: `platform.io/src/ui/historyaxis.h`, `platform.io/src/ui/historyaxis.c`
- Test: `tools/host/test_historyaxis.c`

**Interfaces:**
- Produces (`ui/historyaxis.h`, `FS5000` 조건 없이 컴파일 가능한 순수 C, `<math.h>`만 사용):

```c
#define HISTORY_AXIS_LINE_MAX 12

typedef struct
{
    float bottom;
    float top;
    uint32_t lineNum;
    float lineValues[HISTORY_AXIS_LINE_MAX];
} HistoryAxis;

float getHistoryValueFromLog(uint8_t logValue, float scale);
bool buildHistoryAxis(float minValue, float maxValue, float warningValue, int32_t height, HistoryAxis *axis);
int32_t getHistoryAxisY(const HistoryAxis *axis, float value, int32_t height);
int32_t getHistoryAxisPrefixExponent(float top, int32_t minExponent);
bool strcatHistoryAxisLabel(char *s, float value, int32_t prefixExponent);
```

규칙(스펙 3.5 + 빌더 결정):
- `getHistoryValueFromLog`: `logValue == 0`이면 0(데이터 없음). 아니면 `scale * 0.02 * 10^(logValue / 40)` (`HISTORY_VALUE_MIN`, `HISTORY_DECADE`와 같은 값. `ui/history.h`의 매크로를 include해 쓴다).
- `buildHistoryAxis`: `maxValue <= 0`이면 false. `t = max(maxValue * 1.2, warningValue)` (단 `warningValue`는 `0 < warningValue <= 2 * maxValue`일 때만 반영). `top` = t보다 **큰** 가장 작은 1·2·5 값. `bottom` = `minValue` 이하 가장 큰 1·2·5 값. `top / bottom <= 10^1.5`이면 [bottom, top] 안의 모든 1·2·5 값을 선으로, 아니면 bottom, 그 사이 10의 거듭제곱, top을 선으로. 그다음 아래에서 위로 훑으며 직전에 남긴 선이나 top과의 거리가 18 px 미만인 중간 선을 뺀다(bottom·top은 항상 남김). `lineValues`는 오름차순, `[0] == bottom`, `[lineNum-1] == top`.
- `getHistoryAxisY`: `round((1 - log(value/bottom)/log(top/bottom)) * (height - 1))`, 0..height-1로 자름.
- `getHistoryAxisPrefixExponent`: `floor(log10(top) / 3) * 3`과 `minExponent` 중 큰 값.
- `strcatHistoryAxisLabel`: `value / 10^prefixExponent`를 소수 0~3자리 중 필요한 최소 자리로(끝의 0 제거) 덧붙인다. 3자리를 넘어야 하면 아무것도 붙이지 않고 false.

- [ ] **Step 1: 실패하는 테스트 작성** — `tools/host/test_historyaxis.c` (`// SOURCES: ui/historyaxis.c`), 실수 비교는 상대오차 1e-4:

```c
// 시안 상황: 0.09~0.317, 경고 0.5, 높이 176
assert(buildHistoryAxis(0.09e-6f, 0.317e-6f, 0.5e-6f, 176, &a));
expect_lines(&a, (float[]){0.05e-6f, 0.1e-6f, 0.2e-6f, 0.5e-6f, 1e-6f}, 5);
// 경고가 멀면 반영 안 함
assert(buildHistoryAxis(0.09e-6f, 0.317e-6f, 10e-6f, 176, &a));
expect_lines(&a, (float[]){0.05e-6f, 0.1e-6f, 0.2e-6f, 0.5e-6f}, 4);
// 넓은 범위는 10배 선
assert(buildHistoryAxis(0.1e-6f, 50e-6f, 0, 176, &a));
expect_lines(&a, (float[]){0.1e-6f, 1e-6f, 10e-6f, 100e-6f}, 4);
// 값 하나
assert(buildHistoryAxis(0.128e-6f, 0.128e-6f, 0, 176, &a));
expect_lines(&a, (float[]){0.1e-6f, 0.2e-6f}, 2);
// 낮은 높이에서 솎아내기
assert(buildHistoryAxis(0.05f, 0.9f, 0, 60, &a));
expect_lines(&a, (float[]){0.05f, 2.0f}, 2);
// 데이터 없음
assert(!buildHistoryAxis(0, 0, 0, 176, &a));
// 좌표
assert(getHistoryAxisY(&(HistoryAxis){0.05f, 1.0f, 0, {0}}, 1.0f, 176) == 0);
assert(getHistoryAxisY(&(HistoryAxis){0.05f, 1.0f, 0, {0}}, 0.05f, 176) == 175);
// 로그 디코딩
assert(getHistoryValueFromLog(0, 3.0f) == 0);
assert(close(getHistoryValueFromLog(40, 3.0f), 0.6f));
// 접두어와 라벨
assert(getHistoryAxisPrefixExponent(1e-6f, -9) == -6);
assert(getHistoryAxisPrefixExponent(2000.0f, 0) == 3);
assert(getHistoryAxisPrefixExponent(0.5f, 0) == 0);
label_is(0.05e-6f, -6, "0.05"); label_is(1e-6f, -6, "1"); label_is(100e-6f, -6, "100");
label_is(0.5f, 0, "0.5");
char s[16] = ""; assert(!strcatHistoryAxisLabel(s, 0.0005f, 0) && s[0] == 0);
```

- [ ] **Step 2: 실패 확인** — Run: `bash tools/host/run-tests.sh` / Expected: 컴파일 실패(`historyaxis.h` 없음).
- [ ] **Step 3: `ui/historyaxis.c` 구현** — 위 규칙대로. 1·2·5 값 열거는 `10^floor(log10(x))`에 {1,2,5}를 곱해 만든다.
- [ ] **Step 4: 통과 확인** — Run: `bash tools/host/run-tests.sh` / Expected: `test_historyaxis: PASS`, 종료 코드 0.
- [ ] **Step 5: 커밋** — `git add platform.io/src/ui/historyaxis.* tools/host/test_historyaxis.c && git commit -m "feat(fs5000): add 1-2-5 history axis computation"`

### Task 3: 새 문자열, 글꼴, 빌드 환경

**Files:**
- Modify: `platform.io/src/system/strings/*.h` (30개), `tools/build-fonts.py`, `tools/build-env.py`, `platform.io/platformio.ini`(재생성), `platform.io/src/ui/draw.h`, `platform.io/src/ui/draw.c`(글꼴 extern/정의), `tools/host/render.sh`
- Create: `platform.io/src/ui/fonts/font_tiny_ko_color_14.h`, `font_value_color_54.h`, `font_symbols_color_16.h`; 재생성: `font_small_ko_color_21.h`(및 빌드 도구가 함께 만드는 ko 파일)

**Interfaces:**
- Produces: 문자열 매크로 `STRING_CUMULATIVE_DOSE`, `STRING_START`, `STRING_ELAPSED`, `STRING_AVERAGE_TIME`, `STRING_NOW`, `STRING_WARNING`, `STRING_ALARM`(없을 때만), `STRING_HISTORY_HALF_10_MINUTES`, `STRING_HISTORY_HALF_1_HOUR`, `STRING_HISTORY_HALF_1_DAY`, `STRING_HISTORY_HALF_1_WEEK`, `STRING_HISTORY_HALF_1_MONTH`, `STRING_HISTORY_HALF_1_YEAR`, `STRING_NOTIFICATION_CUMULATIVE_RESET_CONFIRM`, `STRING_NOTIFICATION_CUMULATIVE_RESET_SUCCESS` (문구는 스펙 3.6 표 그대로). `strings` 테이블(문자열 ID enum이 있으면 거기에도) 반영.
- Produces: 전역 글꼴 `font_tiny`, `font_value`, `font_symbols_small` (`FS5000`에서만 정의), 매크로 `FONT_TINY_LINE_HEIGHT`, `FONT_VALUE_LINE_HEIGHT`, `FONT_SYMBOLS_SMALL_LINE_HEIGHT` 등 생성 헤더의 메트릭에서 정한 값(`ui/draw.h`의 FS5000 블록).

- [ ] **Step 1: 문자열 추가** — 30개 파일에 같은 위치(비슷한 문자열 근처)로 추가. 이미 같은 뜻이 있으면 재사용하고 표를 보고서에 적는다.
- [ ] **Step 2: 도구 설치** — `C:/Users/USER/.radpro-pio/Scripts/python -m pip install freetype-py bdflib` 후 그 파이썬으로 `tools/build-fonts.py` 실행 가능 확인.
- [ ] **Step 3: `build-fonts.py`에 FS-5000 글꼴 추가** —
  - `font_value_color_54.h`: 한국어 숫자 큰 글꼴과 같은 원본·`CODEPOINT_SET_LARGE`, `pixels=54`, 이름 `font_value`.
  - `font_symbols_color_16.h`: `RadPro-Symbols.ttf`, `CODEPOINT_SET_SYMBOLS_COLOR`, `pixels=16`, 이름 `font_symbols_small`.
  - `font_tiny_ko_color_14.h`: ko 작은 글꼴과 같은 원본, `pixels=14`, 이름 `font_tiny`, 글자 집합 = `0x20-0x7e,0xb1,0xb5` + 아래 문자열들의 ko 값 글자. 문자열 목록 = 명시 목록(`STRING_ADAPTIVE_FAST`, `STRING_ADAPTIVE_PRECISION`, `STRING_1_MINUTE`, `STRING_30_SECONDS`, `STRING_10_SECONDS`, `STRING_START`, `STRING_ELAPSED`, `STRING_AVERAGE_TIME`, `STRING_NOW`, `STRING_WARNING`, `STRING_ALARM`, `STRING_ALERT_FAULT`, `STRING_INSTANTANEOUS`, `STRING_AVERAGE`, `STRING_HISTORY_*`, `STRING_HISTORY_HALF_*`, `STRING_COUNT`, `STRING_COUNTS`) + 소스에서 정규식으로 찾은 **화면 제목 문자열 전부**: `platform.io/src/**/*.c`에서 `Menu` 초기화의 첫 멤버로 쓰인 `STRING_\w+`, `drawTitleBar(getString(STRING_\w+))`, `drawNotification(getString(STRING_\w+)`의 첫 인자. 생성 후 그 목록의 모든 글자가 집합에 있는지 검사해 빠지면 예외.
  - `build-fonts.py` 실행 후 `git status`에서 ko·새 파일 외 변경은 `git checkout --`으로 되돌린다.
- [ ] **Step 4: `build-env.py`** — `bosean-fs5000`은 언어 `ko`만 생성하고, 이 환경에 `-DFONT_TINY='"fonts/font_tiny_ko_color_14.h"'`, `-DFONT_VALUE='"fonts/font_value_color_54.h"'`, `-DFONT_SYMBOLS_SMALL='"fonts/font_symbols_color_16.h"'` 추가. `python tools/build-env.py`로 `platformio.ini` 재생성(다른 환경 줄은 바뀌지 않아야 함: diff 확인).
- [ ] **Step 5: 글꼴 연결** — `ui/draw.c`(기존 `FONT_SMALL` 등을 include하는 곳)에 `FS5000`일 때 세 글꼴 include. `render.sh`에도 세 매크로 추가.
- [ ] **Step 6: 빌드 확인** — Run: pio 빌드 명령 / Expected: SUCCESS, 경고 0. 크기 기록(아직 새 글꼴을 안 쓰면 링커가 버려 크기 변화가 작을 수 있다). Run: `bash tools/host/render.sh` / Expected: 기존 사례 그대로 `magenta=0`.
- [ ] **Step 7: 커밋** — `git commit -am "feat(fs5000): add strings and tiny/value/symbol fonts for overview"` (새 파일은 `git add`로 포함)

### Task 4: 상단 바 24 px

**Files:**
- Modify: `platform.io/src/ui/draw.h` (FS5000: `TITLEBAR_HEIGHT 24`), `platform.io/src/ui/system.c`, `platform.io/src/ui/system.h`, `platform.io/src/ui/draw.c`(`setupBatteryIcon` 등 기호 글꼴을 바꿔야 하면), `tools/host/harness.c`

**Interfaces:**
- Consumes: `font_tiny`, `font_symbols_small` (Task 3)
- Produces: `void drawMeasurementTitleBar(const char *averagingString);` (`ui/system.h`, `FS5000`) — 왼쪽 `시:분` + 10 px + `averagingString`(neutral), 오른쪽 아이콘. `drawTitleBar(title)`은 FS5000에서 같은 글꼴·아이콘으로 왼쪽 제목, 오른쪽 아이콘+시각.

- [ ] **Step 1: 하네스 사례 추가(실패 상태)** — `titlebar-normal`(평균 방식 `적응형 (빠름)`), `titlebar-fixed`(`10초`), `titlebar-icons`(소리·잠금·경보·배터리 모두, `적응형 (정밀)`), `titlebar-nortc`(RTC 미설정: 시각 없음), 그리고 전체 화면 사례 `menu-settings`, `statistics`, `game`, `rng`, `notification`(누적 초기화 확인 문구). 상단 바 사례는 위 24 px만 검사하도록 자홍색 검사 영역을 지정할 수 있게 `render`를 확장(`renderRegion(name, draw, height)`).
- [ ] **Step 2: 실행해 현재 상태 확인** — Run: `bash tools/host/render.sh` / Expected: `drawMeasurementTitleBar` 미정의로 빌드 실패.
- [ ] **Step 3: 구현** — `TITLEBAR_HEIGHT`를 FS5000에서 24로(그림자 1 포함). 아이콘은 `font_symbols_small`, 글자는 `font_tiny`. 넘치면 평균 방식 문자열을 생략(텍스트 폭 `getTextWidth`로 판단).
- [ ] **Step 4: 렌더 확인** — Run: `bash tools/host/render.sh` / Expected: 모든 사례 `magenta=0`. PNG를 열어 시안 `overview-v4.html`의 "상단 바" 세 가지와 비교: 글자 겹침 없음, 아이콘 세로 가운데. 메뉴·통계·게임·난수·알림 화면이 깨지지 않음(게임판이 화면 안에 들어가고 타이머 글자가 잘리지 않음).
- [ ] **Step 5: 빌드** — pio 빌드 SUCCESS, 경고 0, 크기 기록.
- [ ] **Step 6: 커밋** — `git commit -am "feat(fs5000): 24px title bar with averaging mode"`

### Task 5: 측정 데이터 접근 함수, 시작 시각, 누적 초기화 메뉴

**Files:**
- Modify: `platform.io/src/measurements/instantaneous.c/.h`, `average.c/.h`, `cumulative.c/.h`, `history.c/.h`, `platform.io/src/system/settings.c`, `platform.io/src/system/system.h`, `platform.io/src/measurements/measurements.c` (측정 메뉴)

**Interfaces:**
- Produces (모두 `FS5000` 조건, 헤더에 선언):
  - instantaneous: `float getInstantaneousRateConfidence(void);` `float getInstantaneousMaxRate(void);` `MeasurementStyle getInstantaneousRateStyle(void);` `void clearInstantaneousRateValues(void);`(기존 static `clearInstantaneousRate` 공개) `void drawInstantaneousRateBar(void (*drawBar)(float value, int32_t minExponent, float warningValue, float alarmValue));`(기존 static 공개)
  - average: `float getAverageRateConfidence(void);` `uint32_t getAverageRateTime(void);` `bool isAverageRateDone(void);` `void clearAverageRateValues(void);`
  - cumulative: `MeasurementStyle getCumulativeDoseStyle(void);` `uint32_t getCumulativeDoseStartTime(void);` `void setCumulativeDoseStartTime(uint32_t value);` `void clearCumulativeDoseValues(void);`(선량·시간·카운트를 지우고 시작 시각 = `getDeviceTime()`) `extern const Menu cumulativeDoseMenu;`
  - history: `void selectHistoryTab(int32_t delta);`(순환) `const uint8_t *getHistoryLogValues(void);` `uint32_t getHistoryTimeTicksNum(void);` `const char *getHistoryPeriodString(void);` `const char *getHistoryHalfString(void);`
- `State`에 `uint32_t doseStartTime;` (FS5000). `initSettings`가 읽어 `setCumulativeDoseStartTime`, 저장된 상태가 없으면 `getDeviceTime()`. `saveSettings`가 씀(구조체를 먼저 `memset(&state, 0, sizeof(state))`).
- `system.h`: `#if defined(FS5000)`이면 `SETTINGS_VERSION {'R','a','d','P','r','o','K','1'}`.
- 측정 메뉴(`measurementsMenuOptions`)에 FS5000일 때 `{STRING_CUMULATIVE_DOSE, &cumulativeDoseMenu}`. `cumulativeDoseMenu`는 `STRING_RESET` 한 항목 → 확인 화면(`STRING_NOTIFICATION_CUMULATIVE_RESET_CONFIRM`) → OK면 `clearCumulativeDoseValues()` 후 완료 알림(`..._SUCCESS`). 구조는 `measurements/datalog.c`의 데이터 로그 초기화(`onDatalogClearAlertEvent`, `onDatalogClearConfirmationEvent`)를 따른다.

- [ ] **Step 1: 접근 함수 추가** — 위 시그니처. 기존 값·static을 그대로 돌려주는 얇은 함수.
- [ ] **Step 2: 시작 시각 저장** — `State`, `initSettings`, `saveSettings`, `SETTINGS_VERSION`.
- [ ] **Step 3: 누적 초기화 메뉴** — 위 구조.
- [ ] **Step 4: 빌드** — pio 빌드 SUCCESS, 경고 0. GMC-800 무변경 확인: `platform.io/platformio.ini`의 `[gq-gmc800]` build_flags와 같은 `-D` 정의(+ `-DSIMULATOR` 없이, 언어 매크로는 en)로 `gcc -fsyntax-only -std=gnu11 -I platform.io/src -I platform.io/lib/mcu-renderer <바뀐 .c 파일들>` 경고 없이 통과. 또는 `pio run -e gq-gmc800-en` 성공.
- [ ] **Step 5: 커밋** — `git commit -am "feat(fs5000): measurement accessors, dose start time, cumulative reset menu"`

### Task 6: 1쪽 그리기

**Files:**
- Create: `platform.io/src/ui/overview.h`, `platform.io/src/ui/overview.c` (`FS5000`)
- Modify: `platform.io/src/ui/measurements.c/.h` (작은 글꼴 눈금 막대), `tools/host/harness.c`, `tools/host/render.sh`(새 소스 추가)

**Interfaces:**
- Consumes: `font_tiny`, `font_value` (Task 3), `MeasurementStyle`
- Produces (`ui/overview.h`):

```c
typedef struct
{
    const char *key;
    char value[16];
    char unit[16];
    MeasurementStyle style;
} OverviewRow;

typedef struct
{
    char value[16];
    char unit[16];
    char confidence[16];
    MeasurementStyle style;
    const char *faultString;
    OverviewRow rows[4];
    char startTime[16];
    char elapsedTime[16];
} OverviewPage1;

void drawOverviewPage1(const OverviewPage1 *page, void (*drawBar)(void));
void drawOverviewPageDots(uint32_t pageIndex, uint32_t pageNum, int16_t top);
void drawMeasurementCompactBar(float value, int32_t minExponent, float warningValue, float alarmValue);
```

`faultString != NULL`이면 막대 대신 그 문자열(경보 색), 아니면 `drawBar()`를 부른다(실기에서는 `drawInstantaneousRateBar(drawMeasurementCompactBar)`를 감싼 함수, 하네스에서는 고정 값으로 `drawMeasurementCompactBar` 호출).

영역(모두 x 0~240 전체 폭을 덮는다, 시안 v4 "1쪽" 기준, ±2 px 허용):
- y 24~84 숫자 블록: `font_value` 숫자 x=12 왼쪽 정렬, 단위 21 px 오른쪽 정렬(x=228, 시안 top 30), 신뢰구간 14 px 오른쪽 정렬(시안 top 59, 값 색).
- y 84~136 막대: 14 px 10배 눈금 글자(시안 top 86) → 눈금(top 106, 8 px) → 막대(top 114, 18 px).
- y 136~262 값 줄 4개: 21 px, 줄 top 140/170/200/230, 라벨 neutral 왼쪽, 값 active(행 `style` 색) + 단위 neutral 오른쪽.
- y 262~296 시작·경과: 14 px, top 270. 왼쪽 `시작 <startTime>`, 오른쪽 `경과 <elapsedTime>`, 라벨 neutral.
- y 296~320 점: `drawOverviewPageDots(0, 2, 302)` — 6×6, 간격 6, 가운데. 켜진 점 active, 나머지 frame-line(테마 색 인덱스는 기존 `ColorIndex` 중 가까운 것).

- [ ] **Step 1: 하네스 사례(실패 상태)** — `page1-day`, `page1-dusk`, `page1-night`(시안 표본 값: 0.142 µSv/h ±12%, 평균 0.128, 최대 0.317, 누적 1.284 µSv, 카운트 11,849, 시작 11.13 05:13, 경과 10:01:12), `page1-alarm`(값·누적 선량 줄 경보 스타일), `page1-fault`(`고장`), `page1-big`(`12.47` `mSv/h`, 평균 `9.876 mSv/h`), `page1-long`(경과 `1234:56:12`, 시작 `--`).
- [ ] **Step 2: 실패 확인** — `render.sh` 빌드 실패(`overview.h` 없음).
- [ ] **Step 3: 구현** — `ui/overview.c`, `drawMeasurementCompactBar`(기존 `drawMeasurementBarAt`의 눈금·막대 그리기 재사용, 눈금 글자만 `font_tiny`·위 좌표).
- [ ] **Step 4: 렌더 확인** — Expected: 모든 page1 사례 `magenta=0`. PNG와 시안 1쪽 비교: 숫자와 단위 열이 겹치지 않음(`page1-big` 포함), 시작·경과가 한 줄에 겹치지 않음(`page1-long`).
- [ ] **Step 5: 커밋** — `git commit -am "feat(fs5000): overview page 1 drawing"`

### Task 7: 2쪽 그리기 (그래프)

**Files:**
- Modify: `platform.io/src/ui/overview.c/.h`, `tools/host/harness.c`, `tools/host/render.sh`(`ui/historyaxis.c` 추가)

**Interfaces:**
- Consumes: `HistoryAxis` 함수들 (Task 2), `drawOverviewPageDots` (Task 6)
- Produces:

```c
typedef struct
{
    const uint8_t *logValues;
    float scale;
    const char *unitName;
    int32_t minPrefixExponent;
    float warningValue;
    float alarmValue;
    uint32_t timeTicksNum;
    const char *periodString;
    const char *halfString;
} OverviewGraph;

typedef struct
{
    char instantaneousKey[24];
    char instantaneousValue[16];
    MeasurementStyle instantaneousStyle;
    char averageKey[24];
    char averageValue[16];
    MeasurementStyle averageStyle;
    char secondaryKey[16];
    char secondaryValue[16];
    char averageTime[16];
    char averageConfidence[16];
    const char *faultString;
} OverviewPage2;

void drawOverviewPage2(const OverviewPage2 *page, const OverviewGraph *graph);
```

`warningValue`/`alarmValue`는 표시 단위, 0이면 꺼짐. `unitName`은 접두어 없는 단위 이름(예 `Sv/h`, `cpm`). 라벨 접두어는 `getHistoryAxisPrefixExponent(top, minPrefixExponent)`로 정하고 기존 측정값 문자열 만드는 곳(`measurements.c`의 `buildValueString`)이 쓰는 접두어 문자 표를 재사용해 맨 위 라벨에 `" " + 접두어 + unitName`을 붙인다.

영역(시안 v4 "2쪽" 기준):
- y 24~78 값 칸 세 개: 라벨 14 px(top 27) 아래 값 21 px. x=12, x=96, 셋째는 오른쪽 정렬 x=228. 순간 값은 `instantaneousStyle` 색.
- y 78~100 보조 줄 14 px(top 78): 왼쪽 `평균 시간 <averageTime>`, 오른쪽 `<averageConfidence>`. `faultString`이 있으면 왼쪽에 그것(경보 색)만.
- y 100~280 그래프: 프레임 x=13, 폭 201(`HISTORY_BIN_NUM`), 높이 176, top 102. 칠하기·경고/경보 넘는 색은 기존 `ui/history.c` 열 단위 렌더링 방식을 재사용(새 함수는 위치·높이를 인자로). 그 위에 가로선 점선(tick 색), 가로선 값(14 px, 왼쪽 위, 글자 사각형은 frame 색), 경고선(경고 색 점선 + 오른쪽 `경고`), 경보선(`경보`), 마지막 칸 현재 점(5×5 이상, 값 색), 오른쪽 눈금(x=218~226). 데이터가 없으면 프레임·세로 눈금선만.
- y 280~304 시간축 14 px(top 282): 왼쪽 삼각형(5×6 사각형 계단)·`periodString`·삼각형, 그래프 가로 가운데 `halfString`, 오른쪽 `STRING_NOW`.
- y 304~320 점: `drawOverviewPageDots(1, 2, 308)`.

- [ ] **Step 1: 하네스 사례(실패 상태)** — `page2-10min`(시안 데이터: 0.128 중심, 140번째 칸 근처 0.317 피크, 마지막 0.142, 경고 0.5), `page2-1week`, `page2-night`, `page2-nodata`(모든 칸 0), `page2-fault`, `page2-big`(순간 `12.47`, 라벨 `순간 mSv/h`, 값이 수 mSv/h대인 데이터). 칸 데이터는 `getHistoryLogValue`와 같은 식으로 하네스에서 만든다(`log = 40 * log10(value / (scale * 0.02))`).
- [ ] **Step 2: 실패 확인** — 빌드 실패.
- [ ] **Step 3: 구현**
- [ ] **Step 4: 렌더 확인** — Expected: 모든 page2 사례 `magenta=0`. 시안 2쪽과 비교: 가로선 값 1, 0.5, 0.2, 0.1, 0.05와 `1 µSv/h`; 경고선이 0.5 선에 겹쳐 주황; 데이터 없음 사례에 라벨·점 없음.
- [ ] **Step 5: 커밋** — `git commit -am "feat(fs5000): overview page 2 with labeled history graph"`

### Task 8: 통합 화면 뷰와 키, 화면 정리

**Files:**
- Create: `platform.io/src/measurements/overview.c`, `platform.io/src/measurements/overview.h` (`FS5000`)
- Modify: `platform.io/src/measurements/measurements.c`, `platform.io/src/measurements/instantaneous.c/.h`(기존 `onOverviewViewEvent`를 `DISPLAY_240X320 && !FS5000`로 제한)

**Interfaces:**
- Consumes: Task 5 접근 함수, Task 4 `drawMeasurementTitleBar`, Task 6·7 그리기 함수
- Produces: FS5000에서 `void onOverviewViewEvent(ViewEvent event);`를 `measurements/overview.c`가 정의. `void resetOverview(void);`(쪽=0, 그래프 기간=10분; `resetMeasurements`에서 호출).

동작(스펙 4.1~4.3):
- FS5000의 `MeasurementView` enum과 `measurementViews[]`는 `MEASUREMENTS_VIEW_OVERVIEW` 하나. 경보 자동 전환은 FS5000에서 통합 화면 1쪽(쪽 인덱스 0으로 바꾸고 `setMeasurementViewSelect(MEASUREMENTS_VIEW_OVERVIEW)`).
- `onOverviewViewEvent`는 `onMeasurementViewEvent`보다 **먼저** `EVENT_KEY_UP`/`EVENT_KEY_DOWN`을 받는다: 2쪽이면 `selectHistoryTab(-1/+1)` + `requestViewUpdate()`, 1쪽이면 무시하고 true 처리(뷰 재선택 안 함). 나머지는 `onMeasurementViewEvent`에 넘긴다.
- `EVENT_KEY_BACK`: 쪽 전환 + `requestViewUpdate()`.
- `EVENT_KEY_RESET`(공통 처리에서 경보 해제가 아니었을 때): `triggerVibration()`, `clearInstantaneousRateValues()`, `clearAverageRateValues()`, `requestViewUpdate()`.
- `EVENT_DRAW`: `drawMeasurementTitleBar(getString(<instantaneousAveraging에 해당하는 문자열>))` 후 쪽에 맞춰 구조체를 채워 `drawOverviewPage1`/`drawOverviewPage2`.
- 1쪽 데이터: 값·단위는 `buildValueString`(기존, `measurements.c`)으로. 평균 행 스타일은 `isAverageRateDone()`이면 DONE. 누적 선량 행 스타일 `getCumulativeDoseStyle()`. 카운트는 `getCumulativeDosePulseCount()`를 기존 카운트 표기로. 시작은 `getCumulativeDoseStartTime()`을 기존 시간대 변환(`getDateTimeFromTime` 등, 상단 바 시각과 같은 기준이 되도록 확인)으로 `MM.DD HH:MM`, 12시간제면 상단 바 규칙; 값 0이거나 RTC 미설정이면 `--`. 경과는 `strcatTime(getCumulativeDoseTime())`.
- 2쪽 데이터: 칸 라벨 = `STRING_INSTANTANEOUS` + " " + 순간 값의 단위 문자열, `STRING_AVERAGE` + " " + 평균 값의 단위 문자열, 셋째 = 보조 단위 순간 값의 단위 문자열. 평균 시간 `strcatTime(getAverageRateTime())`, 신뢰구간은 기존 `±N%` 표기. 그래프: `getHistoryLogValues()`, `pulseUnits[settings.doseUnits].rate.scale`, 경고·경보 = `rateAlerts[...] * scale / pulseUnits[DOSE_UNITS_SIEVERTS].rate.scale`(설정 꺼짐이면 0).
- 튜브 고장(`getTubeFaultAlertLevel()`)이면 두 쪽 모두 `faultString = getString(STRING_ALERT_FAULT)`.

- [ ] **Step 1: 구현**
- [ ] **Step 2: 빌드** — Run: pio 빌드 / Expected: SUCCESS, 경고 0, 플래시 ≤ 81,920 B. 크기를 보고에 적는다.
- [ ] **Step 3: 다른 기기 확인** — Run: `cd platform.io && C:/Users/USER/.radpro-pio/Scripts/pio run -e gq-gmc800-en` / Expected: SUCCESS (GMC-800은 기존 통합 화면 그대로).
- [ ] **Step 4: 하네스 전체 렌더** — `bash tools/host/render.sh` 모든 사례 `magenta=0`, `bash tools/host/run-tests.sh` PASS.
- [ ] **Step 5: 커밋** — `git commit -am "feat(fs5000): single overview view with two pages"`

### Task 9: 마무리 (메인 세션이 직접)

**Files:**
- Modify: `docs/handoff/2026-11-12-fs5000-handoff.md` (또는 새 날짜 파일 `docs/handoff/2026-11-13-fs5000-handoff.md`)

- [ ] **Step 1: 전체 리뷰** — 브랜치 전체 diff를 새 리뷰어가 스펙 기준으로 검토.
- [ ] **Step 2: 서명 이미지** — `python tools/sign.py`(사용법은 스크립트 확인)로 `platform.io/.pio/build/bosean-fs5000-ko/firmware.bin` 서명.
- [ ] **Step 3: 핸드오프 갱신** — ARM 빌드 완료·크기, Windows 빌드/글꼴/하네스 절차, 플래싱 절차(`build/radpro-3.1.1/bosean-fs5000/install.bat <서명 이미지 절대경로>`, 백업 후 mass erase), 화면 개편 내용, 확인하지 못한 것.
- [ ] **Step 4: 실기 설치** — ST-Link 연결을 사용자에게 확인한 뒤 `install.bat` 실행. 결과 출력 기록.
- [ ] **Step 5: 커밋** — `git commit -am "docs: update FS-5000 handoff for overview redesign"`
