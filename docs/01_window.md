# OpenGL 창의 시작과 렌더링 루프

이 파일은 다음 작업을 수행하는 OpenGL 프로그램의 기본 골격입니다.

- GLFW로 창을 생성합니다.
- OpenGL 3.3 Core Profile을 요청합니다.
- GLAD로 OpenGL 함수 주소를 로드합니다.
- 창 크기 변경과 키보드 입력을 처리합니다.
- 매 프레임 화면을 어두운 색으로 지웁니다.
- 종료 시 생성한 리소스를 정리합니다.

현재 프로그램은 도형을 그리지 않으므로, 실행하면 어두운 배경의 OpenGL 창이 표시됩니다.

## 1. 헤더 파일 포함

```cpp
#include <cstdlib>
#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
```

### `<cstdlib>`

C 표준 라이브러리의 기능을 사용하기 위한 헤더입니다. 이 코드에서는 프로그램 종료 상태를 나타내는 다음 매크로를 사용합니다.

```cpp
EXIT_SUCCESS
EXIT_FAILURE
```

```cpp
return EXIT_FAILURE;
```

는 실패로 종료되었음을, 다음 코드는 정상 종료되었음을 운영체제에 전달합니다.

```cpp
return EXIT_SUCCESS;
```

### `<iostream>`

C++ 입출력 기능을 제공합니다. 이 코드에서는 오류 메시지를 출력할 때 `std::cerr`를 사용합니다.

```cpp
std::cerr << "Failed to initialize GLAD.\n";
```

`std::cerr`는 표준 오류 출력 스트림입니다.

### GLAD와 GLFW

```cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
```

- **GLAD**: 그래픽 드라이버가 제공하는 OpenGL 함수의 실제 주소를 로드합니다.
- **GLFW**: 창, OpenGL 컨텍스트, 키보드/마우스 입력, 이벤트, 버퍼 교체 등을 관리합니다.

## 2. 이름 없는 namespace

```cpp
namespace
{
    void framebuffer_size_callback(GLFWwindow*, int width, int height)
    {
        glViewport(0, 0, width, height);
    }
} // namespace
```

이름 없는 namespace(anonymous namespace)에 들어 있는 함수와 변수는 현재 `.cpp` 파일에서만 사용할 수 있습니다.

따라서 `framebuffer_size_callback`은 다른 `.cpp` 파일에 공개되지 않습니다. 이 함수는 [main.cpp](./src/main.cpp)에서 GLFW 콜백으로 등록하는 용도일 뿐 외부에서 직접 호출할 필요가 없으므로, 파일 내부 전용으로 제한하는 것이 적절합니다.

개념적으로 다음과 비슷한 목적을 가집니다.

```cpp
static void framebuffer_size_callback(...);
```

이러한 파일 전용 특성은 **internal linkage(내부 연결)**라고 합니다. 같은 이름의 함수가 다른 소스 파일에 있어도 충돌할 가능성을 줄여 줍니다.

## 3. 프레임버퍼 크기 변경 콜백

```cpp
void framebuffer_size_callback(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}
```

창의 프레임버퍼 크기가 변경될 때 GLFW가 호출하는 함수입니다.

첫 번째 매개변수는 `GLFWwindow*` 타입이지만 이름이 없습니다. 함수 내부에서 창 포인터를 사용하지 않기 때문에 매개변수 이름을 생략한 것입니다.

```cpp
GLFWwindow* window
```

처럼 이름을 붙이는 것도 가능하지만, 사용하지 않는 매개변수의 이름을 생략하면 불필요한 미사용 매개변수 경고를 피할 수 있습니다.

### `glViewport`

```cpp
glViewport(0, 0, width, height);
```

OpenGL이 그릴 영역을 지정합니다.

```cpp
glViewport(x, y, width, height);
```

이 코드에서는 다음과 같습니다.

- `0`: 왼쪽 시작 위치
- `0`: 아래쪽 시작 위치
- `width`: 그리기 영역의 너비
- `height`: 그리기 영역의 높이

창 크기를 변경했을 때 viewport도 함께 갱신해야 렌더링 영역이 실제 프레임버퍼 크기와 일치합니다.

## 4. `main` 함수

```cpp
int main()
{
```

프로그램이 시작되는 진입점입니다. 반환하는 정수는 프로그램의 종료 상태로 운영체제에 전달됩니다.

## 5. GLFW 오류 콜백 등록

```cpp
glfwSetErrorCallback([](int error_code, const char* description) {
    std::cerr << "GLFW error (" << error_code << "): " << description << '\n';
});
```

GLFW에서 오류가 발생했을 때 실행할 함수를 등록합니다.

### 람다 함수

```cpp
[](int error_code, const char* description) {
    ...
}
```

이름 없이 즉석에서 정의하는 함수인 **람다(lambda)**입니다. 별도의 이름을 붙인 함수로 작성하면 다음과 비슷합니다.

```cpp
void error_callback(int error_code, const char* description)
{
    std::cerr << "GLFW error (" << error_code << "): "
              << description << '\n';
}
```

여기서 `[]`는 람다의 캡처 목록입니다. 비어 있으므로 바깥쪽 변수를 사용하지 않는 람다입니다.

- `error_code`: GLFW 오류 번호
- `description`: 오류 설명 문자열
- `const`: 오류 설명 문자열을 수정하지 않겠다는 의미

## 6. GLFW 초기화

```cpp
if (!glfwInit())
    return EXIT_FAILURE;
```

`glfwInit()`으로 GLFW를 초기화합니다. 초기화에 실패하면 프로그램을 종료합니다.

`!`는 논리 부정 연산자이므로 다음과 같은 의미입니다.

```cpp
if (glfwInit() == false)
{
    return EXIT_FAILURE;
}
```

실행할 문장이 하나뿐이어서 `if` 뒤의 중괄호를 생략할 수 있습니다.

## 7. OpenGL 버전과 프로파일 설정

```cpp
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
```

창을 생성하기 전에 원하는 OpenGL 컨텍스트의 조건을 GLFW에 전달합니다.

첫 두 줄은 OpenGL 3.3을 요청합니다.

```text
major = 3
minor = 3
```

마지막 줄은 **Core Profile**을 사용하겠다는 뜻입니다. Core Profile은 오래된 고정 기능 방식보다 현대적인 OpenGL 기능을 중심으로 사용합니다.

- 셰이더
- Vertex Buffer Object(VBO)
- Vertex Array Object(VAO)
- Vertex Attribute
- Uniform

## 8. 창 생성

```cpp
GLFWwindow* window =
    glfwCreateWindow(800, 600, "OpenGL Window", nullptr, nullptr);
```

`glfwCreateWindow`로 800×600 크기의 창을 생성합니다.

각 인자는 다음과 같습니다.

| 인자 | 의미 |
|---|---|
| `800` | 창 너비 |
| `600` | 창 높이 |
| `"OpenGL Window"` | 창 제목 |
| 첫 번째 `nullptr` | 공유할 모니터가 없음: 일반 창 모드 |
| 두 번째 `nullptr` | 다른 창과 OpenGL 리소스를 공유하지 않음 |

`GLFWwindow*`는 GLFW가 관리하는 창 객체를 가리키는 포인터입니다.

## 9. 창 생성 실패 처리

```cpp
if (!window)
{
    glfwTerminate();
    return EXIT_FAILURE;
}
```

창 생성에 실패하면 `window`는 `nullptr`이 됩니다.

```cpp
if (!window)
```

는 다음과 같은 뜻입니다.

```cpp
if (window == nullptr)
```

이미 `glfwInit()`을 성공적으로 호출했으므로, 실패하더라도 `glfwTerminate()`로 GLFW를 정리한 뒤 종료합니다.

## 10. OpenGL 컨텍스트 활성화

```cpp
glfwMakeContextCurrent(window);
```

생성한 창의 OpenGL 컨텍스트를 현재 스레드에서 사용하도록 설정합니다. 이후의 OpenGL 명령이 이 창의 화면에 적용됩니다.

## 11. 창 크기 콜백 등록

```cpp
glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
```

`window`의 프레임버퍼 크기가 바뀌었을 때 `framebuffer_size_callback`을 호출하도록 GLFW에 등록합니다.

여기서 함수 이름 뒤에 괄호가 없는 점이 중요합니다.

```cpp
framebuffer_size_callback
```

은 함수를 실행하는 것이 아니라 함수 자체를 전달한다는 뜻입니다. 반면 다음은 즉시 함수를 실행하는 표현입니다.

```cpp
framebuffer_size_callback();
```

## 12. GLAD로 OpenGL 함수 로드

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

OpenGL 함수는 운영체제와 그래픽 드라이버에 따라 실제 주소가 다를 수 있습니다. GLAD는 프로그램 실행 시 필요한 OpenGL 함수 주소를 가져와 사용할 수 있도록 준비합니다.

### `glfwGetProcAddress`

```cpp
glfwGetProcAddress
```

OpenGL 함수의 실제 주소를 찾아주는 GLFW 함수입니다.

### `gladLoadGLLoader`

```cpp
gladLoadGLLoader(...)
```

함수 주소를 조회하는 방법을 받아 GLAD가 필요한 OpenGL 함수들을 로드합니다.

### `reinterpret_cast`

```cpp
reinterpret_cast<GLADloadproc>(glfwGetProcAddress)
```

`glfwGetProcAddress`의 함수 포인터 타입을 GLAD가 요구하는 `GLADloadproc` 타입으로 변환합니다. 라이브러리 간 함수 포인터 타입을 맞추기 위한 명시적 변환입니다.

### 실패 시 정리

GLAD 초기화에 실패하면 다음 순서로 처리합니다.

1. 오류 메시지를 출력합니다.
2. 이미 생성한 창을 `glfwDestroyWindow`으로 제거합니다.
3. `glfwTerminate`로 GLFW를 종료합니다.
4. `EXIT_FAILURE`를 반환합니다.

## 13. 렌더링 반복문

```cpp
while (!glfwWindowShouldClose(window))
{
    ...
}
```

창을 닫아야 한다는 상태가 아닐 동안 계속 반복합니다.

`glfwWindowShouldClose`는 다음과 같은 경우 참이 될 수 있습니다.

- 사용자가 창 닫기 버튼을 클릭한 경우
- `glfwSetWindowShouldClose`를 호출한 경우

앞의 `!` 때문에 닫기 요청이 들어오기 전까지 반복문이 실행됩니다.

## 14. ESC 키 처리

```cpp
if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
```

ESC 키가 눌리면 창을 닫아야 한다는 상태를 설정합니다.

### 키 상태 확인

```cpp
glfwGetKey(window, GLFW_KEY_ESCAPE)
```

지정한 창에서 ESC 키의 현재 상태를 가져옵니다. 반환값이 `GLFW_PRESS`이면 키가 눌린 상태입니다.

### 닫기 상태 설정

```cpp
glfwSetWindowShouldClose(window, GLFW_TRUE);
```

창을 즉시 파괴하지 않고 “닫아야 함”이라는 상태만 설정합니다. 그러면 다음 반복 조건에서 `glfwWindowShouldClose(window)`가 참이 되어 반복문이 종료됩니다.

## 15. 화면을 지울 색 지정

```cpp
glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
```

`glClear`가 색상 버퍼를 지울 때 사용할 색을 지정합니다.

함수의 인자는 다음과 같습니다.

```cpp
glClearColor(red, green, blue, alpha);
```

```text
red   = 0.08
green = 0.08
blue  = 0.12
alpha = 1.00
```

파란색 값이 조금 더 높으므로 어두운 남색 계열의 배경이 됩니다.

숫자 뒤의 `f`는 `float` 타입임을 나타냅니다.

```cpp
0.08   // double
0.08f  // float
```

OpenGL 함수가 `float` 값을 사용하므로 `f`를 붙이는 것이 적절합니다.

## 16. 색상 버퍼 지우기

```cpp
glClear(GL_COLOR_BUFFER_BIT);
```

앞에서 설정한 색으로 색상 버퍼를 지웁니다.

보통 다음 두 호출은 한 쌍으로 사용합니다.

```cpp
glClearColor(...); // 지울 색 지정
glClear(...);      // 실제로 버퍼 지우기
```

현재 코드는 도형을 그리지 않기 때문에 화면 전체가 이 배경색으로 채워집니다.

## 17. 버퍼 교체

```cpp
glfwSwapBuffers(window);
```

OpenGL은 일반적으로 두 개의 버퍼를 사용합니다.

- **프론트 버퍼**: 현재 사용자에게 보이는 화면
- **백 버퍼**: 다음 프레임을 그리는 화면

프로그램은 백 버퍼에 그림을 그린 뒤 버퍼를 교체하여 완성된 프레임을 화면에 표시합니다.

```text
백 버퍼에 그리기
        ↓
버퍼 교체
        ↓
사용자에게 완성된 프레임 표시
```

이 방식을 **더블 버퍼링(double buffering)**이라고 하며, 화면이 그려지는 중간 과정이 사용자에게 보이는 현상을 줄여 줍니다.

## 18. 이벤트 처리

```cpp
glfwPollEvents();
```

운영체제에서 발생한 창과 입력 이벤트를 처리합니다.

- 키보드 입력
- 마우스 입력
- 창 닫기
- 창 이동
- 창 크기 변경
- 최소화와 복원

이 호출이 있어야 GLFW가 이벤트 상태를 업데이트하고 등록된 콜백을 실행할 수 있습니다.

## 19. 종료와 리소스 정리

```cpp
glfwDestroyWindow(window);
glfwTerminate();
return EXIT_SUCCESS;
```

반복문이 끝나면 생성했던 리소스를 정리합니다.

### 창 제거

```cpp
glfwDestroyWindow(window);
```

`glfwCreateWindow`로 생성한 창을 제거합니다.

### GLFW 종료

```cpp
glfwTerminate();
```

GLFW가 사용한 전역 리소스와 내부 상태를 정리합니다.

### 정상 종료

```cpp
return EXIT_SUCCESS;
```

프로그램이 정상적으로 끝났음을 운영체제에 알립니다.

## 전체 실행 순서

```text
프로그램 시작
    ↓
GLFW 오류 콜백 등록
    ↓
GLFW 초기화
    ↓
OpenGL 3.3 Core Profile 요청
    ↓
창 생성
    ↓
OpenGL 컨텍스트 활성화
    ↓
창 크기 변경 콜백 등록
    ↓
GLAD로 OpenGL 함수 로드
    ↓
렌더링 반복
    ├─ ESC 입력 확인
    ├─ 배경색 지정
    ├─ 화면 지우기
    ├─ 버퍼 교체
    └─ 이벤트 처리
    ↓
창 닫기 요청
    ↓
창 제거 및 GLFW 종료
    ↓
프로그램 종료
```

## 핵심 C++ 문법 요약

### 포인터

```cpp
GLFWwindow* window;
```

`window`가 `GLFWwindow` 객체 자체가 아니라 객체의 주소를 저장하는 포인터라는 뜻입니다.

### `nullptr`

```cpp
nullptr
```

유효한 객체를 가리키지 않는 포인터를 나타냅니다.

### 람다

```cpp
[](int error_code, const char* description) {
    ...
}
```

이름 없이 정의하는 함수입니다.

### 이름 없는 namespace

```cpp
namespace
{
    ...
}
```

현재 `.cpp` 파일에서만 사용할 함수와 변수를 정의합니다.

### `reinterpret_cast`

```cpp
reinterpret_cast<GLADloadproc>(glfwGetProcAddress)
```

값이나 함수 포인터를 다른 타입으로 명시적으로 변환합니다. 이 코드에서는 GLAD와 GLFW가 요구하는 함수 포인터 타입을 맞추는 데 사용됩니다.

### 논리 부정 `!`

```cpp
!glfwInit()
```

참과 거짓을 반대로 바꿉니다.

```cpp
!true  // false
!false // true
```

### 비교 연산자 `==`

```cpp
glfwGetKey(...) == GLFW_PRESS
```

두 값이 같은지 비교합니다.

## 현재 프로그램의 렌더링 결과

현재 코드에는 삼각형이나 사각형을 그리는 명령이 없습니다. 렌더링 부분은 사실상 다음 두 줄입니다.

```cpp
glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT);
```

따라서 실행 결과는 **어두운 색으로 채워진 800×600 OpenGL 창**입니다.

도형을 추가하려면 일반적으로 다음 요소가 더 필요합니다.

- GLSL 버텍스 셰이더
- GLSL 프래그먼트 셰이더
- Vertex Array Object(VAO)
- Vertex Buffer Object(VBO)
- 버텍스 데이터
- `glDrawArrays` 또는 `glDrawElements`

현재 [main.cpp](./src/main.cpp)는 이러한 도형 렌더링 단계로 넘어가기 전의 **OpenGL 초기화와 렌더링 루프 기본 골격**입니다.