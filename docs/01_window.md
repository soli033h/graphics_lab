# 01. 윈도우와 OpenGL 컨텍스트

이 문서는 [src/main.cpp](../src/main.cpp)의 윈도우 초기화와 기본 렌더링 루프를 코드 실행 순서에 맞춰 설명합니다.
삼각형의 셰이더, VAO, VBO, 정점 데이터는 [02_triangle.md](./02_triangle.md)에서 다룹니다.

## 전체 흐름

OpenGL 프로그램이 화면을 표시하기까지의 흐름은 다음과 같습니다.

1. 필요한 헤더를 포함합니다.
2. GLFW 오류 콜백을 등록합니다.
3. GLFW를 초기화합니다.
4. 사용할 OpenGL 버전과 프로파일을 지정합니다.
5. 윈도우를 생성합니다.
6. 윈도우의 OpenGL 컨텍스트를 현재 스레드에 연결합니다.
7. 윈도우 크기 변경 콜백을 등록합니다.
8. GLAD로 OpenGL 함수 주소를 로드합니다.
9. 매 프레임 화면을 지우고 버퍼를 교체합니다.
10. 종료 시 윈도우와 GLFW를 정리합니다.

## 1. 헤더 포함 순서

```cpp
#include <cstdlib>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
```

### 표준 라이브러리 헤더

* `<cstdlib>`: `EXIT_SUCCESS`, `EXIT_FAILURE`를 사용하기 위해 포함합니다.
* `<iostream>`: 오류 메시지를 `std::cerr`로 출력하기 위해 포함합니다.

### GLAD와 GLFW

```cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
```

`glad/glad.h`를 `GLFW/glfw3.h`보다 먼저 포함합니다.

* **GLFW**는 윈도우 생성, 입력 이벤트, OpenGL 컨텍스트 생성과 같은 운영체제와의 연결을 담당합니다.
* **GLAD**는 실행 중인 환경에서 OpenGL 함수의 실제 주소를 찾아 사용할 수 있게 합니다.

OpenGL 함수 선언과 플랫폼별 정의가 충돌하지 않도록 GLAD를 먼저 포함하는 것이 중요합니다.

## 2. GLFW 오류 콜백 등록

```cpp
glfwSetErrorCallback([](int error_code, const char* description)
{
    std::cerr << "GLFW error (" << error_code << "): "
              << description << '\n';
});
```

GLFW가 오류를 감지했을 때 호출할 함수를 등록합니다.

* `error_code`: GLFW가 제공하는 오류 번호입니다.
* `description`: 오류 원인을 설명하는 문자열입니다.
* 람다 함수 내부에서 오류를 표준 에러 출력으로 보냅니다.

이 콜백은 `glfwInit()`보다 먼저 등록하는 것이 좋습니다.
초기화 과정에서 오류가 발생하더라도 메시지를 놓치지 않기 위해서입니다.

## 3. GLFW 초기화

```cpp
if (!glfwInit())
    return EXIT_FAILURE;
```

`glfwInit`는 GLFW 라이브러리를 초기화합니다.
키보드와 마우스 입력, 윈도우 시스템, OpenGL 컨텍스트 생성에 필요한 내부 상태가 준비됩니다.

초기화에 실패하면 윈도우를 만들 수 없으므로 즉시 실패 코드로 종료합니다.
아직 생성된 윈도우가 없기 때문에 이 시점에는 `glfwTerminate()`를 호출할 대상도 없습니다.

## 4. OpenGL 컨텍스트 버전과 프로파일 지정

```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
```

`glfwWindowHint`는 다음에 생성할 윈도우의 OpenGL 컨텍스트 속성을 지정합니다.

### OpenGL 버전

```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
```

OpenGL 3.3 컨텍스트를 요청합니다.
셰이더 파일의 `#version 330 core`와도 일치해야 합니다.

### Core Profile

```cpp
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
```

Core Profile을 사용합니다.
Core Profile은 오래된 고정 기능 파이프라인보다 현재 OpenGL의 명시적인 방식인 셰이더, VAO, VBO 사용을 요구합니다.

요청한 버전을 시스템이 지원하지 않으면 이후 `glfwCreateWindow`가 실패할 수 있습니다.

## 5. 윈도우 생성

```cpp
GLFWwindow* window =
    glfwCreateWindow(800, 600, "OpenGL Window", nullptr, nullptr);
```

`glfwCreateWindow`는 윈도우와 해당 윈도우에서 사용할 OpenGL 컨텍스트를 생성합니다.

| 인자 | 값 | 의미 |
|---|---|---|
| `width` | `800` | 윈도우의 가로 크기 |
| `height` | `600` | 윈도우의 세로 크기 |
| `title` | `"OpenGL Window"` | 제목 표시줄에 표시할 문자열 |
| `monitor` | `nullptr` | 일반 창 모드. 모니터를 전달하면 전체 화면 모드 |
| `share` | `nullptr` | 다른 윈도우와 OpenGL 자원을 공유하지 않음 |

생성 결과는 `GLFWwindow*` 포인터로 반환됩니다.

```cpp
if (!window)
{
    glfwTerminate();
    return EXIT_FAILURE;
}
```

윈도우 생성에 실패하면 더 진행할 수 없으므로 GLFW를 종료하고 실패 코드로 반환합니다.
예를 들어 요청한 OpenGL 버전을 지원하지 않거나, 운영체제가 새 윈도우를 만들 수 없는 경우 실패할 수 있습니다.

## 6. OpenGL 컨텍스트를 현재 스레드에 연결

```cpp
glfwMakeContextCurrent(window);
```

OpenGL 컨텍스트는 OpenGL의 상태를 보관하는 작업 공간입니다.
현재 사용 중인 셰이더 프로그램, 바인딩된 버퍼, 색상 버퍼와 같은 상태가 컨텍스트에 저장됩니다.

`glfwMakeContextCurrent`는 생성된 윈도우의 컨텍스트를 현재 실행 중인 스레드의 현재 컨텍스트로 지정합니다.
이 호출 이후의 OpenGL 명령은 해당 윈도우에 연결됩니다.

OpenGL 함수는 현재 컨텍스트가 있어야 정상적으로 동작하므로, GLAD를 로드하거나 `gl...` 함수를 호출하기 전에 실행해야 합니다.

## 7. 프레임버퍼 크기 변경 콜백

```cpp
void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}
```

```cpp
glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
```

윈도우 크기가 변경될 때 OpenGL이 실제로 렌더링할 영역도 함께 변경해야 합니다.
콜백은 프레임버퍼의 새 크기를 받아 `glViewport`에 전달합니다.

```cpp
glViewport(0, 0, width, height);
```

* 첫 번째 `0`: 뷰포트의 왼쪽 시작 위치
* 두 번째 `0`: 뷰포트의 아래쪽 시작 위치
* `width`: 렌더링 영역의 가로 크기
* `height`: 렌더링 영역의 세로 크기

윈도우 크기와 프레임버퍼 크기는 운영체제의 DPI 배율에 따라 다를 수 있습니다.
따라서 윈도우 크기를 직접 계산하기보다 GLFW가 콜백으로 전달하는 프레임버퍼 크기를 사용하는 것이 안전합니다.

## 8. GLAD로 OpenGL 함수 로드

```cpp
if (!gladLoadGLLoader(
        reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
{
    std::cerr << "Failed to initialize GLAD.\n";
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_FAILURE;
}
```

OpenGL 함수는 운영체제와 그래픽 드라이버에 따라 실제 메모리 주소가 달라질 수 있습니다.
GLAD는 함수 이름에 해당하는 주소를 현재 컨텍스트와 드라이버에서 찾아 함수 포인터를 초기화합니다.

### `glfwGetProcAddress`

`glfwGetProcAddress`는 OpenGL 함수 이름을 받아 해당 함수의 주소를 반환합니다.
이 함수를 GLAD의 로더에 전달하면 GLAD가 필요한 OpenGL 함수들을 순서대로 가져옵니다.

### `reinterpret_cast<GLADloadproc>`

```cpp
reinterpret_cast<GLADloadproc>(glfwGetProcAddress)
```

`gladLoadGLLoader`는 `GLADloadproc` 타입의 함수 포인터를 요구합니다.
`glfwGetProcAddress`의 함수 포인터 타입과 목적은 같지만 C++ 타입 표현이 정확히 일치하지 않을 수 있으므로 `reinterpret_cast`로 요구되는 타입에 맞춰 전달합니다.

### 로드 실패 처리

GLAD 로드에 실패하면 OpenGL 함수를 안전하게 호출할 수 없습니다.
따라서 윈도우를 먼저 삭제하고 GLFW를 종료한 뒤 실패 코드로 반환합니다.

## 9. 메인 렌더링 루프

```cpp
while (!glfwWindowShouldClose(window))
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 셰이더와 도형 그리기

    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

`glfwWindowShouldClose`가 `GLFW_TRUE`가 될 때까지 프레임을 반복합니다.
창의 닫기 버튼을 누르거나 코드에서 종료 상태를 설정하면 루프가 끝납니다.

### ESC 키 처리

```cpp
if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
```

현재 프레임에서 ESC 키가 눌렸는지 확인하고, 눌렸다면 창을 닫아야 한다는 상태를 설정합니다.
실제 윈도우 삭제는 루프가 끝난 뒤에 수행됩니다.

### 화면 지우기

```cpp
glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT);
```

두 함수는 역할이 다릅니다.

* `glClearColor`: 색상 버퍼를 지울 때 사용할 색상을 상태로 지정합니다.
* `glClear(GL_COLOR_BUFFER_BIT)`: 지정한 색상으로 현재 색상 버퍼를 실제로 지웁니다.

여기서는 어두운 남색 계열의 `(R, G, B, A)` 값인 `(0.08, 0.08, 0.12, 1.0)`을 사용합니다.
매 프레임 먼저 화면을 지워야 이전 프레임의 렌더링 결과가 남지 않습니다.

### 더블 버퍼링과 `glfwSwapBuffers`

```cpp
glfwSwapBuffers(window);
```

일반적인 GLFW 윈도우는 프론트 버퍼와 백 버퍼를 사용합니다.

* **프론트 버퍼**: 현재 모니터에 표시되는 완성된 화면
* **백 버퍼**: 다음 화면을 그리는 동안 사용자에게 직접 표시되지 않는 버퍼

OpenGL 명령은 보통 백 버퍼에 렌더링됩니다.
`glfwSwapBuffers`는 완성된 백 버퍼와 프론트 버퍼를 교체하여 새 프레임을 화면에 표시합니다.

그리는 중간 상태가 화면에 노출되는 깜빡임이나 화면 찢어짐을 줄이기 위해 사용합니다.
단색 배경만 그리는 경우에도 백 버퍼의 결과를 표시하려면 호출해야 합니다.

### 이벤트 처리와 `glfwPollEvents`

```cpp
glfwPollEvents();
```

운영체제에 쌓인 키보드, 마우스, 창 이동, 창 크기 변경 등의 이벤트를 처리합니다.
이 호출을 반복해서 실행해야 창 닫기 요청과 등록한 콜백이 정상적으로 반영되고, 윈도우가 응답 없음 상태가 되지 않습니다.

## 10. 종료와 리소스 정리

```cpp
glfwDestroyWindow(window);
glfwTerminate();
return EXIT_SUCCESS;
```

렌더링 루프가 끝나면 생성 순서의 반대 방향으로 자원을 정리합니다.

* `glfwDestroyWindow(window)`: 생성한 윈도우와 연결된 자원을 삭제합니다.
* `glfwTerminate()`: GLFW 내부 자원을 해제하고 라이브러리를 종료합니다.
* `EXIT_SUCCESS`: 정상 종료를 나타내는 반환 코드입니다.

초기화 실패나 GLAD 로드 실패 경로에서도 이미 생성된 윈도우를 `glfwDestroyWindow`로 삭제하고 `glfwTerminate`를 호출합니다.
실패 경로에서도 자원 정리를 수행하는 것이 중요합니다.

## 11. `EXIT_SUCCESS`와 `EXIT_FAILURE`

```cpp
return EXIT_FAILURE;
return EXIT_SUCCESS;
```

운영체제에 프로그램 종료 상태를 전달하는 표준 매크로입니다.

* `EXIT_SUCCESS`: 정상 종료
* `EXIT_FAILURE`: 오류로 인한 비정상 종료

단순히 `return 0`이나 `return 1`을 사용하는 것보다 코드의 의도를 분명하게 표현할 수 있고, 플랫폼별 규약을 따를 수 있습니다.

---

## Q&A

### Q1. 왜 `glad/glad.h`를 `GLFW/glfw3.h`보다 먼저 포함하나요?

GLAD 헤더가 OpenGL 함수와 타입을 제공하고, GLFW 헤더도 플랫폼에 따라 OpenGL 관련 선언을 포함할 수 있습니다.
GLAD를 먼저 포함하면 OpenGL 선언이 일관되게 준비되어 심볼 중복 정의나 선언 충돌을 피할 수 있습니다.

### Q2. `glfwCreateWindow`의 네 번째 인자와 다섯 번째 인자는 무엇인가요?

```cpp
glfwCreateWindow(800, 600, "OpenGL Window", nullptr, nullptr);
```

* 네 번째 인자 `monitor`는 전체 화면에 사용할 모니터입니다. `nullptr`이면 일반 창 모드입니다.
* 다섯 번째 인자 `share`는 다른 GLFW 윈도우와 OpenGL 자원을 공유할 때 사용할 윈도우입니다. 공유하지 않으면 `nullptr`입니다.

### Q3. OpenGL 컨텍스트란 무엇인가요?

컨텍스트는 OpenGL이 관리하는 상태들의 묶음입니다.
현재 셰이더, 버퍼, 텍스처, 색상 버퍼와 같은 정보가 컨텍스트에 저장됩니다.
`glfwMakeContextCurrent(window)`를 호출하면 해당 윈도우의 컨텍스트가 현재 스레드의 작업 대상이 됩니다.

### Q4. `glClearColor`와 `glClear`의 차이는 무엇인가요?

`glClearColor`는 지우는 데 사용할 색상을 설정하고, `glClear(GL_COLOR_BUFFER_BIT)`는 그 색상으로 색상 버퍼를 실제로 지웁니다.
전자는 상태 설정이고 후자는 설정된 상태를 사용하는 실행 명령입니다.

### Q5. 단색 배경만 그려도 `glfwSwapBuffers`가 필요한가요?

필요합니다.
렌더링 결과는 백 버퍼에 그려지며, `glfwSwapBuffers`를 호출해야 백 버퍼가 프론트 버퍼와 교체되어 모니터에 표시됩니다.

### Q6. `glfwPollEvents`를 호출하지 않으면 어떻게 되나요?

운영체제가 전달한 키보드, 마우스, 창 닫기와 크기 변경 이벤트가 처리되지 않습니다.
반복적으로 호출하지 않으면 윈도우가 응답 없음 상태가 될 수 있고 콜백도 정상적으로 실행되지 않습니다.

### Q7. `glfwInit` 실패 때와 윈도우 생성 실패 때의 정리가 다른 이유는 무엇인가요?

`glfwInit`가 실패하면 아직 윈도우가 생성되지 않았습니다.
반면 `glfwCreateWindow`가 실패하면 GLFW 자체는 초기화된 상태이므로 `glfwTerminate`를 호출해 초기화된 GLFW 자원을 해제해야 합니다.
