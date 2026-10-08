# 02. 삼각형 그리기

이 문서는 [src/main.cpp](../src/main.cpp)의 삼각형 렌더링 부분을 코드의 실행 순서에 맞춰 설명합니다.
창 생성, GLFW 초기화, OpenGL 컨텍스트 생성과 같은 내용은 제외하고, OpenGL이 준비된 이후의 코드만 다룹니다.

## 전체 흐름

삼각형을 화면에 표시하는 과정은 다음과 같습니다.

1. 셰이더 소스 파일을 읽습니다.
2. 버텍스 셰이더와 프래그먼트 셰이더를 컴파일합니다.
3. 두 셰이더를 하나의 셰이더 프로그램으로 링크합니다.
4. 삼각형의 정점 데이터를 준비합니다.
5. VBO에 정점 데이터를 복사합니다.
6. VAO에 정점 데이터의 해석 방법을 기록합니다.
7. 렌더링 루프에서 셰이더 프로그램과 VAO를 선택합니다.
8. `glDrawArrays`로 세 정점을 삼각형으로 그립니다.

OpenGL에서는 "정점 데이터를 GPU에 저장하는 것"과 "GPU가 그 데이터를 어떻게 해석하는지 지정하는 것"이 별도의 작업입니다.
이 예제에서는 VBO가 전자를 담당하고, VAO와 `glVertexAttribPointer`가 후자를 담당합니다.

## 1. 셰이더 소스 읽기

```cpp
std::string read_file(const std::string& path)
{
    std::ifstream file(path);
    if (!file)
    {
        throw std::runtime_error("Failed to open shader file: " + path);
    }

    std::stringstream contents;
    contents << file.rdbuf();
    return contents.str();
}
```

셰이더는 C++ 코드에 직접 작성하지 않고 별도의 GLSL 파일로 관리합니다.

* `std::ifstream`으로 파일을 엽니다.
* 파일을 열 수 없으면 `std::runtime_error`를 발생시킵니다.
* `std::stringstream`에 파일 전체를 복사합니다.
* `str()`로 GLSL 소스를 `std::string`으로 반환합니다.

셰이더 파일의 경로는 CMake가 정의한 `GRAPHICS_LAB_SHADER_DIR` 매크로를 사용해 만듭니다.

```cpp
shader_program = create_shader_program(
    std::string(GRAPHICS_LAB_SHADER_DIR) + "/basic.vert",
    std::string(GRAPHICS_LAB_SHADER_DIR) + "/basic.frag");
```

이 매크로는 CMake에서 프로젝트의 `shaders` 폴더를 가리키도록 정의되어 있습니다.
따라서 실행 위치가 달라도 빌드 시 지정된 셰이더 폴더를 사용할 수 있습니다.

## 2. 버텍스 셰이더 컴파일

```cpp
GLuint compile_shader(GLenum type, const std::string& source, const char* name)
{
    const GLuint shader = glCreateShader(type);
    const char* source_text = source.c_str();
    glShaderSource(shader, 1, &source_text, nullptr);
    glCompileShader(shader);
    ...
}
```

### 2.1 셰이더 객체 생성

```cpp
const GLuint shader = glCreateShader(type);
```

`glCreateShader`는 GPU에서 관리할 셰이더 객체를 생성합니다.
`type`에는 다음과 같은 종류가 전달됩니다.

* `GL_VERTEX_SHADER`: 각 정점의 최종 위치를 계산합니다.
* `GL_FRAGMENT_SHADER`: 래스터라이즈된 픽셀의 최종 색상을 계산합니다.

### 2.2 GLSL 소스 전달과 컴파일

```cpp
const char* source_text = source.c_str();
glShaderSource(shader, 1, &source_text, nullptr);
glCompileShader(shader);
```

`std::string`의 `c_str()`로 OpenGL이 요구하는 C 문자열 포인터를 얻습니다.
`glShaderSource`는 이 소스를 셰이더 객체에 연결하고, `glCompileShader`가 GLSL을 GPU가 사용할 수 있는 형태로 컴파일합니다.

### 2.3 컴파일 결과 확인

```cpp
GLint success = GL_FALSE;
glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
if (success != GL_TRUE)
{
    GLint log_length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
    std::string log(static_cast<std::size_t>(log_length), '\0');
    glGetShaderInfoLog(shader, log_length, nullptr, log.data());
    glDeleteShader(shader);
    throw std::runtime_error(std::string("Failed to compile ") + name + ":\n" + log);
}
```

OpenGL 함수는 오류를 자동으로 C++ 예외로 바꾸지 않으므로, 컴파일 직후 상태를 직접 확인해야 합니다.

* `GL_COMPILE_STATUS`가 `GL_TRUE`이면 컴파일 성공입니다.
* 실패하면 `GL_INFO_LOG_LENGTH`로 오류 메시지에 필요한 공간을 확인합니다.
* `glGetShaderInfoLog`로 GLSL 문법 오류나 타입 오류를 가져옵니다.
* 실패한 셰이더 객체는 `glDeleteShader`로 삭제합니다.
* 오류 메시지를 포함한 예외를 호출자에게 전달합니다.

## 3. 셰이더 프로그램 링크

```cpp
const GLuint vertex_shader =
    compile_shader(GL_VERTEX_SHADER, vertex_source, "vertex shader");
const GLuint fragment_shader =
    compile_shader(GL_FRAGMENT_SHADER, fragment_source, "fragment shader");
const GLuint program = glCreateProgram();

glAttachShader(program, vertex_shader);
glAttachShader(program, fragment_shader);
glLinkProgram(program);
```

버텍스 셰이더와 프래그먼트 셰이더는 각각 따로 컴파일되지만, 실제 렌더링에서는 함께 동작해야 합니다.

1. `glCreateProgram`으로 프로그램 객체를 생성합니다.
2. `glAttachShader`로 두 셰이더를 프로그램에 붙입니다.
3. `glLinkProgram`으로 셰이더 간 입출력과 전체 실행 구성을 검증합니다.

이 예제의 버텍스 셰이더는 위치만 출력하고, 프래그먼트 셰이더는 별도의 입력 없이 고정된 색상을 출력합니다.
따라서 두 셰이더 사이에 추가로 연결할 사용자 정의 값은 없습니다.

```cpp
GLint success = GL_FALSE;
glGetProgramiv(program, GL_LINK_STATUS, &success);
glDeleteShader(vertex_shader);
glDeleteShader(fragment_shader);
```

링크가 끝나면 셰이더 객체 자체는 삭제해도 됩니다.
셰이더가 프로그램에 이미 연결되어 있으므로, 프로그램 객체가 사용하는 내부 정보는 유지됩니다.
이후에는 링크 결과인 `program`만 렌더링에 사용합니다.

링크 실패 시에는 `glGetProgramInfoLog`로 프로그램 링크 로그를 출력하고 `glDeleteProgram`으로 프로그램 객체를 정리합니다.

## 4. 버텍스 셰이더 분석

파일: [`shaders/basic.vert`](../shaders/basic.vert)

```glsl
#version 330 core

layout (location = 0) in vec2 position;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
}
```

* `#version 330 core`: OpenGL 3.3 Core Profile 문법을 사용합니다.
* `layout (location = 0)`: C++에서 설정할 정점 속성 번호를 0으로 지정합니다.
* `in vec2 position`: 정점마다 2개의 `float`로 된 위치를 입력받습니다.
* `vec4(position, 0.0, 1.0)`: 2차원 위치를 동차 좌표 `(x, y, z, w)`로 확장합니다.
* `gl_Position`: 버텍스 셰이더가 반드시 작성해야 하는 최종 정점 위치입니다.

이 예제는 이미 정점 위치를 정규화 장치 좌표로 사용하므로 별도의 변환 행렬이 필요하지 않습니다.
`z = 0.0`은 화면 앞뒤 깊이의 기준값이고, `w = 1.0`은 일반적인 위치 좌표를 의미합니다.

## 5. 프래그먼트 셰이더 분석

파일: [`shaders/basic.frag`](../shaders/basic.frag)

```glsl
#version 330 core

out vec4 fragment_color;

void main()
{
    fragment_color = vec4(0.2, 0.7, 1.0, 1.0);
}
```

래스터라이제이션 단계가 삼각형 내부에 해당하는 프래그먼트를 만들면 프래그먼트 셰이더가 실행됩니다.

`vec4(0.2, 0.7, 1.0, 1.0)`은 각각 `(빨강, 초록, 파랑, 알파)` 값입니다.
따라서 삼각형은 밝은 파란색으로 출력됩니다.
각 색상 값은 일반적으로 `0.0`에서 `1.0` 사이로 표현합니다.

## 6. 삼각형 정점 데이터

```cpp
const float vertices[] = {
     0.0f,  0.6f,
    -0.6f, -0.6f,
     0.6f, -0.6f
};
```

정점 하나당 `x`, `y` 두 개의 `float`를 사용합니다.

| 정점 | x | y | 위치 |
|---|---:|---:|---|
| 0 | 0.0 | 0.6 | 위쪽 꼭짓점 |
| 1 | -0.6 | -0.6 | 왼쪽 아래 꼭짓점 |
| 2 | 0.6 | -0.6 | 오른쪽 아래 꼭짓점 |

좌표는 정규화 장치 좌표(NDC)입니다.
화면의 중심은 `(0, 0)`, 왼쪽·아래쪽 끝은 대략 `(-1, -1)`, 오른쪽·위쪽 끝은 대략 `(1, 1)`입니다.

`GL_TRIANGLES`에서는 정점 3개가 하나의 삼각형을 구성합니다.
따라서 이 배열의 순서대로 정점 0, 1, 2를 사용하면 하나의 삼각형이 만들어집니다.

## 7. VAO와 VBO 생성

```cpp
glGenVertexArrays(1, &vertex_array);
glGenBuffers(1, &vertex_buffer);
glBindVertexArray(vertex_array);
glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
```

* `glGenVertexArrays`: VAO 이름을 생성합니다.
* `glGenBuffers`: VBO 이름을 생성합니다.
* `glBindVertexArray`: 이후 정점 속성 설정을 기록할 VAO를 선택합니다.
* `glBindBuffer(GL_ARRAY_BUFFER, ...)`: 정점 데이터를 저장할 VBO를 현재 배열 버퍼로 선택합니다.

OpenGL 객체 생성 함수가 반환하는 값은 실제 C++ 객체 포인터가 아니라 OpenGL이 관리하는 객체 이름(handle)입니다.
이 예제에서는 각각 `vertex_array`와 `vertex_buffer`에 저장합니다.

## 8. VBO에 데이터 복사

```cpp
glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(vertices),
    vertices,
    GL_STATIC_DRAW);
```

현재 `GL_ARRAY_BUFFER`로 바인딩된 VBO에 CPU의 `vertices` 배열을 복사합니다.

* `GL_ARRAY_BUFFER`: 정점 속성용 버퍼라는 의미입니다.
* `sizeof(vertices)`: 복사할 전체 바이트 수입니다. `float` 6개이므로 `6 * sizeof(float)`입니다.
* `vertices`: 복사할 CPU 메모리의 시작 주소입니다.
* `GL_STATIC_DRAW`: 데이터가 자주 바뀌지 않고 그리기에 반복 사용된다는 힌트입니다.

이 호출이 끝나면 렌더링 시 GPU가 사용할 정점 데이터가 준비됩니다.

## 9. 정점 속성 형식 지정

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

VBO에 숫자만 저장되어 있기 때문에, GPU가 그 숫자들을 어떻게 묶어 읽어야 하는지 알려줘야 합니다.

`glVertexAttribPointer`의 인자는 다음과 같습니다.

| 인자 | 값 | 의미 |
|---|---|---|
| `index` | `0` | 셰이더의 `layout (location = 0)`과 연결 |
| `size` | `2` | 정점 하나에 `x`, `y` 두 값 |
| `type` | `GL_FLOAT` | 각 값의 자료형 |
| `normalized` | `GL_FALSE` | 정수 정규화를 사용하지 않음 |
| `stride` | `2 * sizeof(float)` | 다음 정점까지 이동할 바이트 간격 |
| `pointer` | `nullptr` | 첫 정점의 시작 위치는 버퍼의 처음 |

배열 메모리는 다음처럼 해석됩니다.

```text
정점 0: x0 y0 | 정점 1: x1 y1 | 정점 2: x2 y2
```

`glEnableVertexAttribArray(0)`은 0번 정점 속성 입력을 활성화합니다.
이 호출이 없으면 버텍스 셰이더의 `position`이 VBO에서 읽히지 않습니다.

## 10. 바인딩 해제

```cpp
glBindBuffer(GL_ARRAY_BUFFER, 0);
glBindVertexArray(0);
```

현재 VAO와 VBO를 해제합니다.
반드시 필요한 작업은 아니지만, 이후 코드가 현재 객체 상태에 실수로 의존하는 것을 줄이고 어떤 객체를 사용할지 명확하게 만들 수 있습니다.

중요한 점은 `glVertexAttribPointer`가 호출될 당시의 VAO와 VBO 관계가 VAO에 기록된다는 것입니다.
렌더링할 때 VBO를 다시 직접 설정하지 않고 VAO만 바인딩해도 되는 이유가 여기에 있습니다.

## 11. 렌더링 루프

창 처리 코드는 생략하고, 삼각형 그리기에 해당하는 부분만 보면 다음과 같습니다.

```cpp
glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT);

glUseProgram(shader_program);
glBindVertexArray(vertex_array);
glDrawArrays(GL_TRIANGLES, 0, 3);
```

### 11.1 화면 지우기

`glClearColor`는 화면을 지울 때 사용할 색상을 지정합니다.
`glClear(GL_COLOR_BUFFER_BIT)`가 실제로 색상 버퍼를 해당 색상으로 채웁니다.

매 프레임 화면을 지우지 않으면 이전 프레임의 결과가 남을 수 있으므로, 보통 그리기 전에 색상 버퍼를 초기화합니다.

### 11.2 셰이더 프로그램 선택

```cpp
glUseProgram(shader_program);
```

이후의 그리기 명령이 사용할 셰이더 프로그램을 선택합니다.
이 호출을 하지 않으면 현재 프로그램이 없거나 다른 프로그램이 선택된 상태로 그려질 수 있습니다.

### 11.3 VAO 선택

```cpp
glBindVertexArray(vertex_array);
```

앞서 저장한 정점 속성 설정을 다시 활성화합니다.
VAO에는 어떤 속성이 활성화되어 있는지, 각 속성이 어떤 형식인지, 어느 VBO에서 읽는지에 대한 상태가 저장되어 있습니다.

### 11.4 삼각형 그리기

```cpp
glDrawArrays(GL_TRIANGLES, 0, 3);
```

인자의 의미는 다음과 같습니다.

* `GL_TRIANGLES`: 정점 3개를 하나의 삼각형으로 해석합니다.
* `0`: 정점 배열의 0번 정점부터 시작합니다.
* `3`: 총 3개의 정점을 사용합니다.

실행 흐름은 다음과 같습니다.

1. VBO에서 정점 0의 `x`, `y`를 읽습니다.
2. 버텍스 셰이더가 이를 `gl_Position`으로 변환합니다.
3. 정점 1과 정점 2에도 같은 작업을 수행합니다.
4. 세 위치를 바탕으로 삼각형 내부의 프래그먼트를 생성합니다.
5. 각 프래그먼트에서 프래그먼트 셰이더가 파란색을 출력합니다.
6. 결과가 색상 버퍼에 기록됩니다.

## 12. 리소스 정리

```cpp
glDeleteVertexArrays(1, &vertex_array);
glDeleteBuffers(1, &vertex_buffer);
glDeleteProgram(shader_program);
```

프로그램이 끝날 때 생성했던 OpenGL 객체를 삭제합니다.

* `glDeleteVertexArrays`: VAO 삭제
* `glDeleteBuffers`: VBO 삭제
* `glDeleteProgram`: 링크된 셰이더 프로그램 삭제

운영체제가 프로세스 종료 시 자원을 회수하더라도, 명시적으로 삭제하는 습관은 장시간 실행되는 프로그램이나 여러 장면을 교체하는 프로그램에서 중요합니다.

## 핵심 요약

이 예제의 핵심은 다음 세 가지 연결입니다.

1. C++의 정점 배열을 `glBufferData`로 VBO에 복사합니다.
2. `glVertexAttribPointer(0, 2, ...)`로 VBO의 두 `float`를 버텍스 셰이더의 `location = 0` 입력과 연결합니다.
3. 렌더링 루프에서 셰이더 프로그램과 VAO를 바인딩한 뒤 `glDrawArrays(GL_TRIANGLES, 0, 3)`을 호출합니다.

즉, **정점 데이터(VBO) → 해석 규칙(VAO) → 셰이더 프로그램 → 그리기 명령**의 순서로 삼각형이 화면에 출력됩니다.

---

## Q&A

### Q1. 사각형을 그리려면 정점이 6개 필요한가요?

현재 예제처럼 `glDrawArrays(GL_TRIANGLES, ...)`를 사용한다면 정점 6개가 필요합니다.
OpenGL은 사각형을 한 번에 그리기보다 삼각형 2개로 나누어 그립니다.

```text
A ───── B
│     / │
│   /   │
│ /     │
D ───── C
```

대각선 `A-C`를 기준으로 나누면 다음 두 삼각형이 됩니다.

```text
삼각형 1: A, B, C
삼각형 2: A, C, D
```

따라서 정점 배열에는 `A`와 `C`가 두 번 등장합니다.

```cpp
const float vertices[] = {
    // 삼각형 1: A, B, C
    -0.6f,  0.6f,  // A
     0.6f,  0.6f,  // B
     0.6f, -0.6f,  // C

    // 삼각형 2: A, C, D
    -0.6f,  0.6f,  // A
     0.6f, -0.6f,  // C
    -0.6f, -0.6f   // D
};
```

### Q2. 사각형을 그릴 때 `glDrawArrays`의 정점 개수를 6으로 바꾸면 되나요?

네. 정점 배열에 정점 6개가 들어 있으므로 마지막 인자를 `3`에서 `6`으로 변경합니다.

```cpp
glDrawArrays(GL_TRIANGLES, 0, 6);
```

`GL_TRIANGLES`는 정점 3개마다 삼각형 하나를 만듭니다.

```text
정점 0, 1, 2 → 삼각형 1
정점 3, 4, 5 → 삼각형 2
```

정점 하나가 여전히 `x`, `y` 두 개의 `float`로 구성되어 있으므로 `glBufferData`와 `glVertexAttribPointer` 설정은 그대로 사용할 수 있습니다.

### Q3. 사각형의 꼭짓점은 4개인데 왜 배열에는 6개를 넣나요?

사각형의 실제 꼭짓점은 `A`, `B`, `C`, `D` 네 개입니다.
하지만 `GL_TRIANGLES`는 정점 3개씩 묶어 삼각형을 만들기 때문에 다음 순서가 필요합니다.

```text
A, B, C, A, C, D
```

`A`와 `C`는 두 삼각형 모두에 필요하므로 정점 배열에서 중복됩니다.
`glDrawArrays`는 정점에 이름을 붙여 재사용하지 않고 배열을 순서대로 읽기 때문에, 같은 좌표라도 직접 다시 기록해야 합니다.

### Q4. 정점 중복을 없애고 사각형의 꼭짓점 4개만 사용할 수 있나요?

가능합니다. VBO에는 꼭짓점 4개만 저장하고, 별도의 인덱스 버퍼인 EBO를 사용하면 됩니다.

```cpp
const float vertices[] = {
    -0.6f,  0.6f,  // A: 0
     0.6f,  0.6f,  // B: 1
     0.6f, -0.6f,  // C: 2
    -0.6f, -0.6f   // D: 3
};

const unsigned int indices[] = {
    0, 1, 2,
    0, 2, 3
};
```

인덱스는 다음과 같이 정점을 재사용합니다.

```text
0, 1, 2 → A, B, C
0, 2, 3 → A, C, D
```

이 경우 `glDrawArrays` 대신 `glDrawElements`를 사용합니다.

```cpp
glDrawElements(
    GL_TRIANGLES,
    6,
    GL_UNSIGNED_INT,
    nullptr);
```

두 방식의 차이는 다음과 같습니다.

| 방식 | 저장하는 데이터 | 그리기 함수 |
|---|---|---|
| `glDrawArrays` | 정점 6개 | `glDrawArrays` |
| `glDrawElements` | 정점 4개 + 인덱스 6개 | `glDrawElements` |

현재 단계에서는 `glDrawArrays`로 정점 6개를 사용하는 방법이 구조를 이해하기 쉽고, 이후 EBO를 배우면 정점 재사용 방식으로 확장할 수 있습니다.

### Q5. 시간에 따라 도형의 색상을 바꿀 수 있나요?

가능합니다. CPU에서 현재 시간을 가져와 셰이더의 `uniform` 변수로 전달하면 됩니다.
`uniform`은 모든 정점 또는 프래그먼트가 공통으로 읽을 수 있는 셰이더 입력값입니다.

프래그먼트 셰이더에는 시간값을 받을 변수를 선언합니다.

```glsl
uniform float time;
```

그리고 `sin` 함수를 이용해 색상 성분을 반복적으로 변화시킵니다.

```glsl
const float two_pi_over_three = 2.0943951;
const vec3 color = 0.5 + 0.5 * vec3(
    sin(time),
    sin(time + two_pi_over_three),
    sin(time + 2.0 * two_pi_over_three));

fragment_color = vec4(color, 1.0);
```

`sin`의 결과는 `-1.0`부터 `1.0` 사이입니다.
여기에 `0.5`를 곱하고 `0.5`를 더하면 색상에 사용할 수 있는 `0.0`부터 `1.0` 사이로 변환됩니다.

```text
sin(time)             → -1.0 ~ 1.0
0.5 + 0.5 * sin(time) →  0.0 ~ 1.0
```

RGB 성분마다 위상(phase)을 조금씩 다르게 주었기 때문에 세 색상이 서로 다른 시점에 밝아지며 색상이 계속 변합니다.
`2.0943951`은 `2π / 3`에 해당하는 값입니다.

### Q6. C++에서 셰이더에 시간을 어떻게 전달하나요?

먼저 셰이더 프로그램에서 `time` uniform의 위치를 얻습니다.

```cpp
const GLint time_location = glGetUniformLocation(shader_program, "time");
```

그 다음 렌더링 루프에서 현재 시간을 전달합니다.

```cpp
glUseProgram(shader_program);
glUniform1f(
    time_location,
    static_cast<float>(glfwGetTime()));
```

`glfwGetTime()`은 GLFW가 초기화된 이후 경과한 시간을 초 단위의 `double`로 반환합니다.
`glUniform1f`는 `float` 타입의 uniform을 설정하므로 `static_cast<float>`로 변환합니다.

`glUseProgram`을 먼저 호출하는 이유는 uniform 값이 현재 선택된 셰이더 프로그램에 기록되기 때문입니다.
시간을 매 프레임 다시 전달하므로 프래그먼트 셰이더가 매 프레임 다른 색상을 계산합니다.