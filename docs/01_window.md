# 01. Window 

## 1. 헤더 포함 및 초기화 순서

* **`#include <glad/glad.h>` 우선 포함**: GLAD 헤더는 반드시 GLFW 헤더보다 먼저 포함되어야 합니다. 그렇지 않으면 OpenGL 심볼 중복 정의 오류가 발생합니다.
* **오류 콜백 사전 설정**: `glfwInit()`을 호출하기 전에 `glfwSetErrorCallback()`을 통해 오류 발생 시 콘솔 로그를 출력하도록 람다 함수를 먼저 등록합니다.

---

## 2. 창 생성 매개변수 (`glfwCreateWindow`)

```cpp
GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Window", nullptr, nullptr);

```

* **4번째 인자 (`monitor`)**: 전체 화면(Fullscreen) 여부를 지정합니다. 일반 창 모드로 띄울 경우 `nullptr`을 전달합니다.
* **5번째 인자 (`share`)**: 다른 윈도우 창과 그래픽 자원(텍스처, 버퍼 등)을 공유할지 결정합니다. 공유할 창이 없으면 `nullptr`을 전달합니다.

---

## 3. OpenGL 컨텍스트(Context)의 개념

* **컨텍스트 정의**: OpenGL의 모든 상태 값(칠할 색상, 사용 중인 텍스처, 화면 크기 등)을 저장하는 작업 공간(도화지)입니다.
* **`glfwMakeContextCurrent(window)`**: 생성된 창의 컨텍스트를 현재 실행 중인 C++ 스레드의 메인 작업 대상으로 지정합니다. 이후 실행되는 모든 OpenGL 명령어(`gl...`)는 이 창의 컨텍스트에 적용됩니다.

---

## 4. GLAD 로딩과 `reinterpret_cast`

```cpp
gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))

```

* **사용 이유**:
* `gladLoadGLLoader`가 요구하는 함수 포인터 형태(`GLADloadproc`)와 GLFW가 제공하는 `glfwGetProcAddress` 함수 포인터의 반환 타입이 C++ 타입 시스템상 미세하게 다릅니다.
* 둘 다 "OpenGL 함수의 메모리 주소를 찾는 함수"라는 목적은 동일하므로, `reinterpret_cast`를 사용해 비트 단위로 형변환하여 전달합니다.



---

## 5. 메인 루프와 더블 버퍼링 (Double Buffering)

### `glClearColor` vs `glClear`

* `glClearColor(R, G, B, A)`: 지울 때 사용할 색상 물감을 설정합니다 (상태 지정).
* `glClear(GL_COLOR_BUFFER_BIT)`: 설정한 색상으로 실제 색상 버퍼(도화지)를 비웁니다 (행동 실행).

### `glfwSwapBuffers(window)`

* **프론트 버퍼 (Front Buffer)**: 현재 모니터 화면에 출력 중인 완성된 도화지.
* **백 버퍼 (Back Buffer)**: 화면 뒤에서 GPU가 새로 그리고 있는 도화지.
* **역할**: 백 버퍼에 그림이 다 그려지면 프론트 버퍼와 위치를 교체하여 사용자에게 보여줍니다. 그림이 그려지는 중간 과정이 보여 발생하는 화면 찢어짐(Tearing)이나 깜빡임을 방지합니다.
* **단색 배경에서도 호출하는 이유**: GLFW는 기본 동작이 더블 버퍼링으로 설정되어 있습니다. 백 버퍼에 그린 내용을 프론트 버퍼로 교체해주지 않으면 화면 업데이트가 이루어지지 않습니다.

### `glfwPollEvents()`

* 키보드/마우스 입력, 창 이동, 크기 변경 등의 운영체제 이벤트를 수집하고 처리합니다.
* 루프 내에서 이 함수를 지속적으로 호출해야 창이 '응답 없음' 상태가 되지 않고 등록된 콜백 함수들이 정상 작동합니다.

---

## 6. `EXIT_SUCCESS` / `EXIT_FAILURE` 매크로 사용 이유

* **운영체제 이식성 (Cross-platform)**: OS 환경마다 성공/실패를 나타내는 반환 코드 값이 다를 수 있으나, 이 매크로를 사용하면 컴파일 타임에 해당 OS 규격에 맞는 상수로 전환됩니다.
* **C/C++ 표준 호환성**: C 표준 라이브러리(`<stdlib.h>`) 시절부터 OS 및 런타임 인터페이스 규약으로 정의된 표준 매크로입니다.
* **코드 의도 전달**: 단순 숫자를 반환하는 것보다 비정상 종료라는 발생 의도를 명확하게 전달합니다.