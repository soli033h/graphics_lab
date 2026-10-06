# Rendering a Triangle with OpenGL

이 문서는 [src/main.cpp](../src/main.cpp), [shaders/basic.vert](../shaders/basic.vert), [shaders/basic.frag](../shaders/basic.frag)를 함께 읽으며, OpenGL에서 삼각형을 그리는 과정을 설명합니다.

## 이 단계에서 추가된 것

이전 단계인 [01_window.md](./01_window.md)에서는 창을 만들고 배경색으로 화면을 지우는 것까지 다루었습니다.

이번 단계에서는 다음 요소를 추가했습니다.

- GPU에서 실행할 버텍스 셰이더와 프래그먼트 셰이더
- 셰이더 파일을 읽는 함수
- 셰이더 컴파일과 프로그램 링크
- 삼각형의 버텍스 데이터
- VBO(Vertex Buffer Object)
- VAO(Vertex Array Object)
- `glDrawArrays`를 사용한 삼각형 렌더링
- 셰이더 컴파일/링크 실패 처리

전체 흐름은 다음과 같습니다.

```text
삼각형 버텍스 데이터
        ↓
VBO에 GPU 데이터로 업로드
        ↓
VAO에 버텍스 해석 방법 기록
        ↓
버텍스 셰이더 실행
        ↓
래스터라이제이션
        ↓
프래그먼트 셰이더 실행
        ↓
화면에 삼각형 표시
```

## 프로젝트 구조

```text
graphics_lab/
├── src/
│   └── main.cpp
├── shaders/
│   ├── basic.vert
│   └── basic.frag
└── docs/
    ├── 01_window.md
    └── 02_triangle.md
```

`main.cpp`는 OpenGL 객체를 만들고 렌더링 순서를 제어합니다. 셰이더의 실제 코드는 `shaders` 폴더의 별도 파일에 있습니다.

## 1. 필요한 헤더

```cpp
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
```

이전 단계에서 사용한 헤더 외에 파일과 문자열, 예외 처리를 위한 헤더가 추가되었습니다.

### `<fstream>`

```cpp
#include <fstream>
```

파일을 읽고 쓰기 위한 스트림을 제공합니다. 이번 코드에서는 셰이더 파일을 읽을 때 `std::ifstream`을 사용합니다.

### `<sstream>`

```cpp
#include <sstream>
```

문자열 스트림을 제공합니다. 파일 전체 내용을 하나의 `std::string`으로 모으는 데 사용합니다.

### `<string>`

```cpp
#include <string>
```

`std::string`을 사용하기 위한 헤더입니다. 셰이더 소스와 오류 메시지를 문자열로 다룹니다.

### `<stdexcept>`

```cpp
#include <stdexcept>
```

표준 예외 타입인 `std::runtime_error`를 사용하기 위한 헤더입니다. 파일을 열지 못하거나 셰이더 컴파일에 실패하면 예외를 발생시킵니다.

## 2. 이름 없는 namespace

```cpp
namespace 
{
    ...
}
```

`read_file`, `create_shader`, `create_shader_program`, `framebuffer_size_callback`은 이 소스 파일에서만 사용됩니다. 따라서 이름 없는 namespace 안에 넣어 다른 소스 파일에 공개되지 않도록 했습니다.

이전 단계에서 설명한 것처럼, 이 구조는 파일 전용 함수의 내부 연결을 표현합니다. namespace 안에 여러 함수를 넣어도 모두 현재 `.cpp` 파일의 구현 세부 사항으로 제한됩니다.

## 3. 셰이더 파일 읽기

```cpp
std::string read_file(const char* path)
{
    std::ifstream file(path);
    if (!file)
    {
        throw std::runtime_error(
            "Failed to open shader file: " + std::string(path));
    }

    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}
```

OpenGL의 `glShaderSource`는 셰이더 소스를 문자열로 받습니다. 따라서 먼저 디스크의 셰이더 파일을 읽어 `std::string`으로 반환해야 합니다.

### 파일 열기

```cpp
std::ifstream file(path);
```

`path`에 지정된 파일을 읽기 모드로 엽니다.

```cpp
if (!file)
```

파일을 열지 못했는지 확인합니다. 파일 경로가 잘못되었거나 파일이 존재하지 않으면 스트림 상태가 실패 상태가 됩니다.

### 예외 발생

```cpp
throw std::runtime_error(
    "Failed to open shader file: " + std::string(path));
```

파일을 열 수 없을 때 조용히 빈 문자열을 반환하지 않고 명확한 오류를 전달합니다.

- `throw`: 현재 함수의 정상 흐름을 중단하고 예외를 발생시킵니다.
- `std::runtime_error`: 실행 중 발생한 오류를 표현하는 표준 예외 타입입니다.
- `std::string(path)`: C 문자열인 `path`를 C++ 문자열로 변환합니다.

### 파일 전체 읽기

```cpp
std::ostringstream contents;
contents << file.rdbuf();
return contents.str();
```

- `file.rdbuf()`: 파일 스트림의 전체 버퍼를 가져옵니다.
- `contents << ...`: 파일 내용을 문자열 스트림에 넣습니다.
- `contents.str()`: 모인 내용을 `std::string`으로 반환합니다.

## 4. 셰이더 하나 컴파일하기

```cpp
GLuint create_shader(
    GLenum type,
    const std::string& source,
    const char* name)
{
    const GLuint shader = glCreateShader(type);
    const char* source_data = source.c_str();
    glShaderSource(shader, 1, &source_data, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[512]{};
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        glDeleteShader(shader);
        throw std::runtime_error(
            "Failed to compile " + std::string(name) + ":\n" + log);
    }

    return shader;
}
```

이 함수는 버텍스 셰이더나 프래그먼트 셰이더 하나를 생성하고 컴파일합니다.

### `GLuint`와 `GLenum`

```cpp
GLuint
GLenum
```

OpenGL이 정의한 정수형 타입입니다.

- `GLuint`: OpenGL 객체 이름에 사용하는 부호 없는 정수 타입
- `GLenum`: OpenGL의 종류나 옵션을 나타내는 열거형 값 타입

`glCreateShader`가 반환하는 셰이더 핸들은 `GLuint`로 저장합니다.

### 셰이더 객체 생성

```cpp
const GLuint shader = glCreateShader(type);
```

OpenGL에 셰이더 객체를 생성합니다. `type`은 다음 중 하나입니다.

```cpp
GL_VERTEX_SHADER
GL_FRAGMENT_SHADER
```

### 소스 연결

```cpp
const char* source_data = source.c_str();
glShaderSource(shader, 1, &source_data, nullptr);
```

`std::string`의 `c_str()`는 C 스타일 문자열 포인터를 반환합니다. `glShaderSource`는 셰이더 소스 문자열의 포인터를 요구하므로 이 변환이 필요합니다.

`glShaderSource`의 주요 인자는 다음과 같습니다.

```cpp
glShaderSource(shader, 1, &source_data, nullptr);
```

- `shader`: 소스를 넣을 셰이더 객체
- `1`: 소스 문자열의 개수
- `&source_data`: 소스 문자열 포인터의 주소
- `nullptr`: 문자열 길이를 직접 지정하지 않음

### 컴파일

```cpp
glCompileShader(shader);
```

셰이더의 GLSL 소스 코드를 GPU 드라이버가 이해할 수 있는 형태로 컴파일합니다.

### 컴파일 결과 확인

```cpp
GLint success = GL_FALSE;
glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
```

컴파일 성공 여부를 조회합니다. OpenGL 함수가 결과를 포인터로 기록하므로 `&success`를 전달합니다.

### 컴파일 로그 출력

```cpp
char log[512]{};
glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
```

컴파일에 실패하면 드라이버가 제공하는 오류 로그를 가져옵니다.

- `char log[512]{}`: 512바이트 버퍼를 0으로 초기화합니다.
- `sizeof(log)`: 버퍼의 크기를 전달합니다.
- 마지막 `log`: 오류 메시지를 기록할 버퍼입니다.

오류가 발생한 셰이더를 삭제한 뒤 예외를 발생시킵니다.

```cpp
glDeleteShader(shader);
throw std::runtime_error(...);
```

## 5. 셰이더 프로그램 만들기

```cpp
GLuint create_shader_program(
    const char* vertex_path,
    const char* fragment_path)
{
    const GLuint vertex_shader =
        create_shader(
            GL_VERTEX_SHADER,
            read_file(vertex_path),
            "vertex shader");
    const GLuint fragment_shader =
        create_shader(
            GL_FRAGMENT_SHADER,
            read_file(fragment_path),
            "fragment shader");
    const GLuint program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    GLint success = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    if (!success)
    {
        char log[512]{};
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        glDeleteProgram(program);
        throw std::runtime_error(
            "Failed to link shader program:\n" + std::string(log));
    }

    return program;
}
```

버텍스 셰이더와 프래그먼트 셰이더를 각각 컴파일한 뒤 하나의 실행 가능한 셰이더 프로그램으로 연결(link)합니다.

### 두 셰이더 컴파일

```cpp
const GLuint vertex_shader =
    create_shader(
        GL_VERTEX_SHADER,
        read_file(vertex_path),
        "vertex shader");
```

파일을 읽고 `GL_VERTEX_SHADER` 타입으로 컴파일합니다. 프래그먼트 셰이더도 같은 방식으로 처리합니다.

### 프로그램 객체 생성과 연결

```cpp
const GLuint program = glCreateProgram();

glAttachShader(program, vertex_shader);
glAttachShader(program, fragment_shader);
glLinkProgram(program);
```

- `glCreateProgram`: 여러 셰이더를 담을 프로그램 객체 생성
- `glAttachShader`: 컴파일된 셰이더를 프로그램에 연결
- `glLinkProgram`: 셰이더 단계들을 하나의 실행 가능한 프로그램으로 연결

셰이더는 각각 따로 컴파일되지만, 실제 렌더링 때는 링크된 프로그램을 사용합니다.

### 링크 결과 확인

```cpp
glGetProgramiv(program, GL_LINK_STATUS, &success);
```

셰이더 각각의 컴파일이 성공해도 서로의 입력과 출력이 맞지 않으면 프로그램 링크가 실패할 수 있습니다. 따라서 컴파일 결과와 별도로 링크 결과도 확인해야 합니다.

### 셰이더 객체 삭제

```cpp
glDeleteShader(vertex_shader);
glDeleteShader(fragment_shader);
```

링크가 끝난 뒤에는 개별 셰이더 객체를 삭제할 수 있습니다. 링크된 프로그램은 셰이더 코드에 필요한 정보를 이미 가지고 있으므로, 렌더링에 필요한 프로그램 객체는 계속 유지됩니다.

링크 실패 시에는 프로그램 객체도 삭제합니다.

```cpp
glDeleteProgram(program);
```

## 6. `main` 함수에서 셰이더 프로그램 생성

```cpp
try
{
    const GLuint shader_program = create_shader_program(
        GRAPHICS_LAB_SHADER_DIR "/basic.vert",
        GRAPHICS_LAB_SHADER_DIR "/basic.frag");
```

셰이더 파일을 읽고 컴파일하는 과정은 실패할 수 있으므로 `try` 블록 안에서 실행합니다.

`GRAPHICS_LAB_SHADER_DIR`는 [CMakeLists.txt](../CMakeLists.txt)에서 컴파일 정의로 전달됩니다.

```cmake
target_compile_definitions(graphics_lab
    PRIVATE
        GRAPHICS_LAB_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shaders"
)
```

따라서 실행 파일은 빌드 시 결정된 절대 경로로 `basic.vert`와 `basic.frag`를 찾습니다. 현재 구성에서는 실행 파일을 어느 작업 디렉터리에서 실행하더라도 셰이더 경로가 달라지지 않는 장점이 있습니다.

## 7. 삼각형 버텍스 데이터

```cpp
const float vertices[] =
{
     0.0f,  0.6f,
    -0.6f, -0.6f,
     0.6f, -0.6f
};
```

삼각형의 세 꼭짓점을 정의합니다. 각 꼭짓점은 `x`, `y` 두 값으로 구성되어 있습니다.

```text
첫 번째 점: ( 0.0,  0.6)
두 번째 점: (-0.6, -0.6)
세 번째 점: ( 0.6, -0.6)
```

OpenGL의 기본 클립 공간에서는 일반적으로 다음 범위를 사용합니다.

```text
x: -1.0 ~ 1.0
y: -1.0 ~ 1.0
```

따라서 이 좌표들은 화면 중앙 근처에 삼각형을 만듭니다.

배열의 메모리 구조는 다음과 같습니다.

```text
0.0, 0.6, -0.6, -0.6, 0.6, -0.6
 \___/    \____/    \____/
  점 1      점 2      점 3
```

## 8. VAO와 VBO 생성

```cpp
GLuint vao = 0;
GLuint vbo = 0;
glGenVertexArrays(1, &vao);
glGenBuffers(1, &vbo);
```

### VBO

**VBO(Vertex Buffer Object)**는 버텍스 데이터를 GPU 메모리에 저장하는 OpenGL 객체입니다.

```cpp
glGenBuffers(1, &vbo);
```

VBO의 이름을 생성합니다.

### VAO

**VAO(Vertex Array Object)**는 버텍스 데이터를 어떻게 해석할지에 대한 설정을 기억하는 객체입니다.

이번 코드에서는 다음 정보를 VAO에 기록합니다.

- 어떤 VBO를 사용할지
- 버텍스 속성 위치가 어디인지
- 한 버텍스가 몇 개의 값으로 구성되는지
- 각 값의 타입과 간격

## 9. 버텍스 데이터 업로드와 속성 설정

```cpp
glBindVertexArray(vao);
glBindBuffer(GL_ARRAY_BUFFER, vbo);
glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(vertices),
    vertices,
    GL_STATIC_DRAW);
glVertexAttribPointer(
    0,
    2,
    GL_FLOAT,
    GL_FALSE,
    2 * sizeof(float),
    nullptr);
glEnableVertexAttribArray(0);
glBindVertexArray(0);
```

### VAO 바인딩

```cpp
glBindVertexArray(vao);
```

이후의 버텍스 관련 설정을 `vao`에 기록하도록 합니다.

### VBO 바인딩

```cpp
glBindBuffer(GL_ARRAY_BUFFER, vbo);
```

`vbo`를 현재 `GL_ARRAY_BUFFER`로 지정합니다. 이후 `glBufferData`가 이 VBO에 데이터를 넣게 됩니다.

### GPU로 데이터 복사

```cpp
glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(vertices),
    vertices,
    GL_STATIC_DRAW);
```

인자의 의미는 다음과 같습니다.

- `GL_ARRAY_BUFFER`: 버텍스 데이터 버퍼
- `sizeof(vertices)`: 데이터 전체 크기(바이트)
- `vertices`: 복사할 CPU 메모리의 시작 주소
- `GL_STATIC_DRAW`: 자주 변경하지 않고 그릴 데이터라는 사용 목적

여기서 `sizeof(vertices)`는 배열 요소 개수가 아니라 배열 전체의 바이트 크기입니다. `float` 6개이므로 보통 `6 * sizeof(float)`과 같은 값입니다.

### 버텍스 속성 해석 방법

```cpp
glVertexAttribPointer(
    0,
    2,
    GL_FLOAT,
    GL_FALSE,
    2 * sizeof(float),
    nullptr);
```

이 함수는 VBO의 바이트 데이터를 버텍스 속성으로 해석하는 방법을 지정합니다.

- `0`: 속성 위치 0
- `2`: 한 버텍스의 값 개수(`x`, `y`)
- `GL_FLOAT`: 각 값의 타입
- `GL_FALSE`: 정규화하지 않음
- `2 * sizeof(float)`: 다음 버텍스까지의 간격(stride)
- `nullptr`: 첫 번째 값이 버퍼 시작 위치에 있음

현재 데이터는 다음과 같은 구조입니다.

```text
x, y, x, y, x, y
```

따라서 한 버텍스의 크기는 `float` 2개이고, stride는 `2 * sizeof(float)`입니다.

### 속성 활성화

```cpp
glEnableVertexAttribArray(0);
```

속성 위치 0을 사용하도록 활성화합니다. 이 위치는 버텍스 셰이더의 다음 선언과 연결됩니다.

```glsl
layout (location = 0) in vec2 position;
```

## 10. 버텍스 셰이더

파일: [shaders/basic.vert](../shaders/basic.vert)

```glsl
#version 330 core

layout (location = 0) in vec2 position;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
}
```

버텍스 셰이더는 입력으로 들어온 각 버텍스마다 실행됩니다.

### GLSL 버전

```glsl
#version 330 core
```

OpenGL 3.3 Core Profile에 맞는 GLSL 버전을 사용합니다.

### 버텍스 입력

```glsl
layout (location = 0) in vec2 position;
```

- `layout (location = 0)`: CPU 코드에서 지정한 속성 위치 0과 연결
- `in`: 버텍스 셰이더 입력
- `vec2`: 두 개의 `float` 값
- `position`: 입력 변수 이름

CPU 쪽의 다음 설정과 서로 맞아야 합니다.

```cpp
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                      2 * sizeof(float), nullptr);
```

### 클립 공간 위치

```glsl
gl_Position = vec4(position, 0.0, 1.0);
```

`position`은 `vec2`이므로 `z`와 `w`를 추가해 `vec4`로 만듭니다.

```text
vec2(x, y) → vec4(x, y, 0.0, 1.0)
```

`gl_Position`은 버텍스의 최종 위치를 나타내는 GLSL 내장 출력 변수입니다.

- `x`, `y`: 화면에서의 위치
- `z = 0.0`: 깊이
- `w = 1.0`: 동차 좌표의 기본값

## 11. 프래그먼트 셰이더

파일: [shaders/basic.frag](../shaders/basic.frag)

```glsl
#version 330 core

out vec4 fragment_color;

void main()
{
    fragment_color = vec4(0.2, 0.7, 1.0, 1.0);
}
```

프래그먼트 셰이더는 삼각형 내부의 각 프래그먼트가 어떤 색으로 표시될지 결정합니다.

### 출력 변수

```glsl
out vec4 fragment_color;
```

프래그먼트 셰이더의 색상 출력을 선언합니다. `vec4`의 네 값은 다음과 같습니다.

```text
red, green, blue, alpha
```

### 색상 지정

```glsl
fragment_color = vec4(0.2, 0.7, 1.0, 1.0);
```

삼각형을 다음 색으로 그립니다.

```text
R = 0.2
G = 0.7
B = 1.0
A = 1.0
```

따라서 밝은 하늘색 계열의 삼각형이 표시됩니다.

## 12. 렌더링 루프에서 삼각형 그리기

```cpp
while (!glfwWindowShouldClose(window))
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_program);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);
    glfwPollEvents();
}
```

### 셰이더 프로그램 사용

```cpp
glUseProgram(shader_program);
```

이후의 그리기 명령에서 사용할 셰이더 프로그램을 선택합니다.

### VAO 선택

```cpp
glBindVertexArray(vao);
```

앞에서 설정한 VAO를 다시 활성화합니다. VAO가 기억하고 있는 VBO와 버텍스 속성 설정을 사용하게 됩니다.

### 삼각형 그리기

```cpp
glDrawArrays(GL_TRIANGLES, 0, 3);
```

배열에 저장된 버텍스를 사용해 그립니다.

- `GL_TRIANGLES`: 세 버텍스마다 하나의 삼각형 생성
- `0`: 사용할 첫 번째 버텍스의 인덱스
- `3`: 사용할 버텍스 개수

버텍스 3개가 있으므로 삼각형 하나가 만들어집니다.

`glDrawArrays`를 호출하면 대략 다음 순서가 진행됩니다.

```text
버텍스 0, 1, 2 읽기
        ↓
각 버텍스에 버텍스 셰이더 실행
        ↓
삼각형 조립
        ↓
삼각형 내부를 프래그먼트로 분할
        ↓
각 프래그먼트에 프래그먼트 셰이더 실행
        ↓
색상 버퍼에 기록
```

## 13. 리소스 정리

```cpp
glDeleteVertexArrays(1, &vao);
glDeleteBuffers(1, &vbo);
glDeleteProgram(shader_program);
```

OpenGL 객체를 더 이상 사용하지 않을 때 삭제합니다.

- `glDeleteVertexArrays`: VAO 삭제
- `glDeleteBuffers`: VBO 삭제
- `glDeleteProgram`: 셰이더 프로그램 삭제

생성한 리소스를 종료 전에 정리하면 명시적으로 소유권을 관리할 수 있고, 프로그램이 커졌을 때 리소스 누수를 추적하기 쉬워집니다.

## 14. 예외 처리

```cpp
catch (const std::exception& error)
{
    std::cerr << error.what() << '\n';
    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_FAILURE;
}
```

`try` 블록 내부에서 다음과 같은 오류가 발생하면 `catch`가 처리합니다.

- 셰이더 파일을 열 수 없음
- 버텍스 셰이더 컴파일 실패
- 프래그먼트 셰이더 컴파일 실패
- 셰이더 프로그램 링크 실패

```cpp
error.what()
```

는 예외에 저장된 오류 설명을 반환합니다.

오류를 출력한 뒤 이미 생성된 창과 GLFW를 정리하고 실패 상태로 종료합니다. 오류를 무시하고 계속 진행하지 않기 때문에 원인을 확인하기 쉽습니다.

## 15. CPU 코드와 셰이더의 연결

이번 코드에서 가장 중요한 연결은 다음 두 부분입니다.

### CPU 쪽

```cpp
glVertexAttribPointer(
    0,
    2,
    GL_FLOAT,
    GL_FALSE,
    2 * sizeof(float),
    nullptr);
glEnableVertexAttribArray(0);
```

### 버텍스 셰이더 쪽

```glsl
layout (location = 0) in vec2 position;
```

두 코드 모두 위치 `0`을 사용하고, CPU 쪽은 두 개의 `float`를 전달하며 셰이더 쪽은 `vec2`로 받습니다.

만약 CPU 코드의 위치나 데이터 형식과 셰이더의 입력 선언이 일치하지 않으면 버텍스 위치가 잘못 해석될 수 있습니다.

## 16. 자주 헷갈리는 개념

### VBO와 VAO의 차이

| 객체 | 역할 |
|---|---|
| VBO | 실제 버텍스 데이터 저장 |
| VAO | 버텍스 데이터를 해석하는 설정 저장 |

VBO는 데이터 자체이고, VAO는 그 데이터를 읽는 방법을 기억한다고 생각할 수 있습니다.

### 컴파일과 링크의 차이

| 단계 | 의미 |
|---|---|
| 셰이더 컴파일 | 각각의 GLSL 파일이 문법적으로 올바른지 확인하고 컴파일 |
| 프로그램 링크 | 컴파일된 셰이더 단계들을 하나의 실행 프로그램으로 연결 |

각 셰이더의 컴파일이 성공해도 셰이더 간 인터페이스가 맞지 않으면 링크가 실패할 수 있습니다.

### 셰이더 객체와 셰이더 프로그램

`vertex_shader`와 `fragment_shader`는 각각의 셰이더 객체입니다. `shader_program`은 이 셰이더들을 연결한 결과이며, 렌더링 시 `glUseProgram`으로 선택하는 대상입니다.

## 17. 실행 결과

현재 프로그램은 다음과 같은 결과를 보여 줍니다.

- 800×600 크기의 GLFW 창
- 어두운 남색 계열의 배경
- 중앙에 밝은 하늘색 삼각형
- 창 크기를 변경하면 viewport가 새 크기에 맞게 조정됨
- ESC 키를 누르면 종료됨

## 18. 전체 실행 순서

```text
GLFW 초기화
    ↓
OpenGL 컨텍스트 생성
    ↓
GLAD로 OpenGL 함수 로드
    ↓
basic.vert 파일 읽기
    ↓
버텍스 셰이더 컴파일
    ↓
basic.frag 파일 읽기
    ↓
프래그먼트 셰이더 컴파일
    ↓
두 셰이더를 프로그램으로 링크
    ↓
삼각형 버텍스 데이터를 VBO에 업로드
    ↓
VAO에 버텍스 속성 설정
    ↓
반복 시작
    ├─ 화면 지우기
    ├─ 셰이더 프로그램 선택
    ├─ VAO 선택
    ├─ 삼각형 그리기
    ├─ 버퍼 교체
    └─ 이벤트 처리
    ↓
VAO, VBO, 셰이더 프로그램 삭제
    ↓
창과 GLFW 정리
```

## 핵심 요약

이 단계에서 삼각형을 그리는 핵심은 다음 세 가지입니다.

1. 버텍스 데이터를 VBO에 저장합니다.
2. VAO에 버텍스 데이터를 해석하는 방법을 설정합니다.
3. 셰이더 프로그램을 사용한 뒤 `glDrawArrays`를 호출합니다.

```cpp
glUseProgram(shader_program);
glBindVertexArray(vao);
glDrawArrays(GL_TRIANGLES, 0, 3);
```

`main.cpp`는 CPU 측에서 GPU 리소스와 그리기 순서를 관리하고, 셰이더 파일은 GPU에서 각 버텍스와 프래그먼트가 어떻게 처리되는지를 정의합니다.