# 05. Uniform으로 셰이더에 값 전달하기

이 문서는 C++에서 계산한 값을 GLSL 셰이더로 전달하는 `uniform`의 개념과 사용 순서를 설명합니다.
예제 코드는 수정하지 않고, 현재 프로젝트의 셰이더 프로그램 구조에 적용할 수 있는 형태로 정리합니다.

## 1. Uniform이란?

`uniform`은 CPU에서 셰이더 프로그램으로 값을 전달하는 GLSL 변수입니다.
한 번 값을 설정하면 해당 셰이더 프로그램을 사용하는 하나의 그리기 호출(draw call) 동안 모든 정점 또는 프래그먼트가 같은 값을 읽습니다.

```glsl
uniform float time;
```

정점마다 다른 데이터인 VBO의 정점 속성과 비교하면 다음과 같습니다.

```text
정점 속성:
정점 0, 정점 1, 정점 2마다 다른 값

uniform:
현재 셰이더 프로그램과 draw call 전체에서 공유되는 값
```

주로 다음과 같은 값을 uniform으로 전달합니다.

* 전체 도형에 적용할 색상
* 경과 시간
* 이동·회전·크기 변환 행렬
* 카메라 행렬
* 텍스처 샘플러
* 조명 위치와 색상

## 2. CPU에서 GPU로 값이 전달되는 흐름

Uniform을 사용하는 과정은 다음과 같습니다.

```text
1. GLSL 셰이더에 uniform 변수 선언
2. 셰이더 프로그램 링크
3. uniform 변수의 위치 조회
4. 사용할 셰이더 프로그램 선택
5. glUniform 계열 함수로 값 전달
6. 셰이더에서 uniform 값 사용
```

전체 흐름을 코드 형태로 나타내면 다음과 같습니다.

```glsl
// GLSL
uniform float value;
```

```cpp
// C++
GLint value_location =
    glGetUniformLocation(shader_program, "value");

glUseProgram(shader_program);
glUniform1f(value_location, value);
```

`uniform`은 VBO처럼 `glBufferData`로 업로드하지 않습니다.
셰이더 프로그램 안에 있는 uniform 저장 공간에 `glUniform` 함수로 직접 값을 설정합니다.

## 3. GLSL에서 uniform 선언하기

프래그먼트 셰이더에서 전체 도형의 색상을 전달받는 예시는 다음과 같습니다.

```glsl
#version 330 core

uniform vec4 object_color;

out vec4 fragment_color;

void main()
{
    fragment_color = object_color;
}
```

`object_color`는 `vec4` 타입의 uniform입니다.
일반적으로 순서는 다음과 같습니다.

```text
vec4(R, G, B, A)
```

예를 들어 빨간색은 다음과 같습니다.

```glsl
vec4(1.0, 0.0, 0.0, 1.0)
```

* 빨강: `1.0`
* 초록: `0.0`
* 파랑: `0.0`
* 알파: `1.0`

## 4. Uniform 위치 조회

셰이더 프로그램을 링크한 뒤 C++에서 uniform의 위치를 조회합니다.

```cpp
const GLint color_location =
    glGetUniformLocation(shader_program, "object_color");
```

`glGetUniformLocation`은 문자열 이름으로 셰이더 프로그램 내부의 uniform 위치를 찾습니다.
반환된 위치는 이후 `glUniform` 함수에 전달합니다.

### 4.1 왜 `GLint`를 사용하나요?

uniform 위치는 음수가 될 수 있으므로 `GLint`를 사용합니다.

```cpp
if (color_location < 0)
{
    // uniform을 찾지 못한 경우
}
```

`glGetUniformLocation`이 `-1`을 반환하는 대표적인 경우는 다음과 같습니다.

* 셰이더에 해당 이름의 uniform이 없음
* C++의 이름과 GLSL의 이름이 다름
* 셰이더 컴파일 또는 프로그램 링크가 실패함
* uniform을 셰이더에서 실제로 사용하지 않아 컴파일러가 제거함

반면 `GLuint`는 음수를 표현할 수 없으므로 uniform 위치를 저장하는 데 적합하지 않습니다.

### 4.2 언제 위치를 조회하나요?

uniform 위치는 셰이더 프로그램이 링크된 뒤 조회해야 합니다.

```text
셰이더 생성
    ↓
셰이더 컴파일
    ↓
프로그램 링크
    ↓
glGetUniformLocation
```

링크 전에는 프로그램 내부 uniform의 최종 위치가 정해지지 않았을 수 있습니다.
따라서 프로그램 링크가 끝난 뒤 위치를 가져옵니다.

## 5. 셰이더 프로그램 선택

uniform 값을 설정하기 전에 사용할 프로그램을 선택해야 합니다.

```cpp
glUseProgram(shader_program);
```

그 다음 uniform 값을 전달합니다.

```cpp
glUniform4f(
    color_location,
    0.2f,
    0.7f,
    1.0f,
    1.0f);
```

`glUniform` 함수는 현재 사용 중인 셰이더 프로그램의 uniform을 수정합니다.
따라서 다음 순서가 중요합니다.

```cpp
glUseProgram(shader_program);
glUniform4f(color_location, 0.2f, 0.7f, 1.0f, 1.0f);
```

다른 셰이더 프로그램이 현재 선택되어 있거나 아무 프로그램도 선택되지 않은 상태에서 uniform을 설정하면 원하는 프로그램에 값이 전달되지 않을 수 있습니다.

## 6. `glUniform` 함수 이름 읽는 법

OpenGL은 uniform 타입과 값의 개수에 따라 여러 함수를 제공합니다.
함수 이름의 숫자와 마지막 문자를 읽으면 어떤 값을 전달하는지 알 수 있습니다.

```text
glUniform + 개수 + 자료형
```

예를 들어:

```cpp
glUniform1f(location, value);
```

는 다음 의미입니다.

```text
1 → 값 하나
f → float
```

즉, `float` 하나를 전달합니다.

## 7. 자주 사용하는 uniform 함수

| 함수 | 의미 | GLSL 타입 |
|---|---|---|
| `glUniform1f` | `float` 하나 | `float` |
| `glUniform2f` | `float` 두 개 | `vec2` |
| `glUniform3f` | `float` 세 개 | `vec3` |
| `glUniform4f` | `float` 네 개 | `vec4` |
| `glUniform1i` | 정수 하나 | `int` |
| `glUniform1i` | 불리언 값 전달에도 사용 | `bool` |
| `glUniformMatrix4fv` | 4x4 행렬 전달 | `mat4` |

예시는 다음과 같습니다.

### `float`

```glsl
uniform float opacity;
```

```cpp
glUniform1f(opacity_location, 0.5f);
```

### `vec2`

```glsl
uniform vec2 offset;
```

```cpp
glUniform2f(offset_location, 0.2f, -0.1f);
```

### `vec3`

```glsl
uniform vec3 light_color;
```

```cpp
glUniform3f(light_color_location, 1.0f, 0.8f, 0.4f);
```

### `vec4`

```glsl
uniform vec4 object_color;
```

```cpp
glUniform4f(
    color_location,
    0.2f,
    0.7f,
    1.0f,
    1.0f);
```

### 정수와 불리언

```glsl
uniform int texture_unit;
uniform bool use_texture;
```

```cpp
glUniform1i(texture_unit_location, 0);
glUniform1i(use_texture_location, GL_TRUE);
```

정수 계열 uniform에는 `glUniform1i`를 사용합니다.
`float`가 필요한 곳에 `glUniform1i`를 사용하거나, 정수 uniform에 `glUniform1f`를 사용하는 것은 타입이 맞지 않습니다.

## 8. 시간 전달 예시

시간은 매 프레임 달라지는 값이므로 렌더링 루프에서 업데이트합니다.

셰이더:

```glsl
uniform float time;

void main()
{
    float brightness = 0.5 + 0.5 * sin(time);
    fragment_color = vec4(brightness, 0.7, 1.0, 1.0);
}
```

C++:

```cpp
const GLint time_location =
    glGetUniformLocation(shader_program, "time");
```

uniform 위치 조회는 프로그램 링크 후 한 번만 수행합니다.
값 전달은 렌더링 루프에서 매 프레임 실행합니다.

```cpp
glUseProgram(shader_program);
glUniform1f(
    time_location,
    static_cast<float>(glfwGetTime()));
```

`glfwGetTime()`은 GLFW 초기화 이후 경과한 시간을 초 단위의 `double`로 반환합니다.
셰이더의 `time`이 `float`이므로 `static_cast<float>`로 변환해 전달합니다.

```text
glGetUniformLocation
    → 위치를 찾는 작업. 보통 한 번

glUniform1f
    → 현재 값을 쓰는 작업. 시간이 변하면 매 프레임
```

## 9. 정적 값과 동적 값

모든 uniform을 매 프레임 갱신할 필요는 없습니다.

### 정적 값

도형의 색상처럼 실행 중 거의 바뀌지 않는 값은 프로그램을 사용하기 시작할 때 한 번 설정할 수 있습니다.

```cpp
glUseProgram(shader_program);
glUniform4f(color_location, 0.2f, 0.7f, 1.0f, 1.0f);
```

### 동적 값

시간, 입력에 따른 위치, 카메라 위치처럼 계속 바뀌는 값은 변경될 때마다 다시 설정해야 합니다.

```cpp
while (!glfwWindowShouldClose(window))
{
    glUseProgram(shader_program);
    glUniform1f(time_location, current_time);
    // draw
}
```

uniform은 마지막으로 설정된 값을 유지합니다.
따라서 값이 바뀌지 않는다면 매 프레임 다시 설정할 필요가 없습니다.

## 10. Uniform과 VBO의 차이

| 항목 | VBO 정점 데이터 | `uniform` |
|---|---|---|
| 저장 위치 | GPU 버퍼 | 셰이더 프로그램 상태 |
| 데이터 단위 | 정점마다 다를 수 있음 | draw call 전체에서 공유 |
| 설정 함수 | `glBufferData` | `glUniform` 계열 |
| 대표 데이터 | 위치, 정점 색상, UV | 시간, 색상, 행렬 |
| 변경 빈도 | 버퍼 사용 방식에 따라 다름 | 값이 바뀔 때 업데이트 |

예를 들어 VBO의 정점 색상은 정점마다 다릅니다.

```text
정점 0 → 빨강
정점 1 → 초록
정점 2 → 파랑
```

반면 uniform 색상은 전체 도형에 공통으로 적용됩니다.

```text
정점 0 → 같은 object_color
정점 1 → 같은 object_color
정점 2 → 같은 object_color
```

## 11. Uniform과 `in`/`out` 변수의 차이

버텍스 셰이더와 프래그먼트 셰이더 사이의 `in`/`out` 변수는 파이프라인을 따라 전달되는 값입니다.

```glsl
// vertex shader
out vec3 color;

// fragment shader
in vec3 color;
```

이 값은 기본적으로 정점 사이에서 보간될 수 있습니다.
반면 uniform은 CPU가 설정한 하나의 값을 셰이더 프로그램이 공통으로 읽습니다.

```glsl
uniform float time;
```

정리하면:

```text
in/out:
정점별 값 → 파이프라인을 따라 전달 → 필요하면 보간

uniform:
CPU가 설정한 공통 값 → 모든 셰이더 호출에서 공유
```

## 12. Uniform 배열과 구조체

Uniform은 배열이나 구조체로도 선언할 수 있습니다.

```glsl
uniform vec3 light_positions[2];
```

이때 위치를 조회할 때 배열의 첫 항목 이름을 사용합니다.

```cpp
GLint location =
    glGetUniformLocation(shader_program, "light_positions[0]");
```

구조체도 멤버 이름을 포함해 조회합니다.

```glsl
struct Material
{
    vec4 color;
    float shininess;
};

uniform Material material;
```

```cpp
GLint color_location =
    glGetUniformLocation(shader_program, "material.color");
```

처음에는 단일 `float`, `vec3`, `vec4` uniform부터 익히고 배열과 구조체는 조명이나 재질을 다룰 때 사용하는 것이 좋습니다.

## 13. 행렬 uniform 미리보기

이후 도형 이동·회전·크기 변경을 배울 때는 `mat4`를 uniform으로 전달합니다.

```glsl
uniform mat4 transform;

void main()
{
    gl_Position = transform * vec4(position, 0.0, 1.0);
}
```

C++에서는 보통 16개의 `float`로 구성된 행렬을 `glUniformMatrix4fv`로 전달합니다.

```cpp
glUniformMatrix4fv(
    transform_location,
    1,
    GL_FALSE,
    transform_data);
```

* 두 번째 인자 `1`: 행렬 하나를 전달
* 세 번째 인자 `GL_FALSE`: 전치하지 않음
* 네 번째 인자: 행렬 데이터의 시작 주소

행렬 계산은 직접 구현할 수도 있지만, 일반적으로 GLM 같은 수학 라이브러리를 사용합니다.

## 14. 주의할 점

### 프로그램을 먼저 사용하기

```cpp
glUseProgram(shader_program);
glUniform1f(location, value);
```

uniform을 설정할 프로그램을 먼저 선택합니다.

### 이름을 정확히 맞추기

GLSL:

```glsl
uniform float time;
```

C++:

```cpp
glGetUniformLocation(shader_program, "time");
```

대소문자를 포함해 이름이 정확히 같아야 합니다.

### 타입 맞추기

```glsl
uniform vec4 color;
```

에는 `glUniform4f`가 적절합니다.

```cpp
glUniform4f(location, r, g, b, a);
```

`vec4`에 `glUniform1f`를 사용하면 필요한 값의 개수가 맞지 않습니다.

### 위치를 매 프레임 조회하지 않기

다음처럼 위치를 매 프레임 조회할 수도 있지만 불필요한 작업입니다.

```cpp
while (...)
{
    GLint location =
        glGetUniformLocation(shader_program, "time");
}
```

프로그램 링크 후 위치를 한 번 저장하고, 루프에서는 값만 업데이트하는 편이 좋습니다.

## 핵심 요약

```text
GLSL:
uniform float time;

프로그램 링크 후:
glGetUniformLocation(shader_program, "time");

렌더링 루프:
glUseProgram(shader_program);
glUniform1f(time_location, current_time);
```

`uniform`은 VBO에 저장되는 정점 데이터가 아니라, CPU가 현재 셰이더 프로그램에 전달하는 공통 입력값입니다.
위치 조회는 프로그램 링크 후 한 번, 값 설정은 값이 바뀌는 시점에 수행합니다.

전체 흐름은 다음과 같습니다.

```text
GLSL uniform 선언
    ↓
셰이더 프로그램 링크
    ↓
uniform 위치 조회
    ↓
셰이더 프로그램 사용
    ↓
glUniform으로 값 전달
    ↓
셰이더에서 값 사용
```

---

## Q&A

### Q1. 왜 `glUseProgram → glUniform1f → glBindVertexArray → glDrawElements` 순서로 작성하나요?

각 함수가 서로 다른 OpenGL 상태를 설정하기 때문입니다.

```cpp
glUseProgram(shader_program);
glUniform1f(time_location, current_time);
glBindVertexArray(vertex_array);
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
```

각 함수의 역할은 다음과 같습니다.

```text
glUseProgram
    → 이번 그리기에 사용할 셰이더 프로그램 선택

glUniform1f
    → 현재 선택된 셰이더 프로그램의 time uniform에 값 기록

glBindVertexArray
    → 이번 그리기에 사용할 정점 입력 상태 선택

glDrawElements
    → 현재 셰이더 프로그램과 현재 VAO를 사용해 실제로 그림
```

### Q2. `glUseProgram`을 `glUniform1f`보다 먼저 호출해야 하는 이유는 무엇인가요?

`glUniform1f`는 셰이더 프로그램을 인자로 직접 전달받지 않습니다.
대신 **현재 사용 중인 셰이더 프로그램**의 uniform을 수정합니다.

따라서 먼저 다음과 같이 프로그램을 선택해야 합니다.

```cpp
glUseProgram(shader_program);
```

그 다음 현재 프로그램의 uniform에 값을 씁니다.

```cpp
glUniform1f(time_location, current_time);
```

올바른 관계는 다음과 같습니다.

```text
현재 프로그램 선택
    ↓
그 프로그램의 uniform 값 변경
```

반대로 다음 순서라면:

```cpp
glUniform1f(time_location, current_time);
glUseProgram(shader_program);
```

`glUniform1f` 호출 시점에 원하는 프로그램이 현재 프로그램으로 선택되어 있지 않을 수 있습니다.
그러면 값이 다른 프로그램에 적용되거나, 유효한 프로그램이 없어 오류가 발생할 수 있습니다.

### Q3. `glBindVertexArray`를 `glDrawElements`보다 먼저 호출해야 하는 이유는 무엇인가요?

`glDrawElements`는 정점 데이터를 직접 인자로 받지 않습니다.
대신 현재 바인딩된 VAO에서 다음 정보를 가져옵니다.

* 정점 속성 형식
* 정점 속성 활성화 상태
* VBO 연결
* EBO 연결

따라서 그리기 직전에 원하는 VAO를 선택해야 합니다.

```cpp
glBindVertexArray(vertex_array);
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
```

실행 흐름은 다음과 같습니다.

```text
vertex_array 선택
    ↓
VAO에 저장된 VBO/EBO와 정점 속성 확인
    ↓
glDrawElements가 EBO의 인덱스로 VBO의 정점 선택
    ↓
셰이더 실행
```

VAO를 바인딩하지 않거나 다른 VAO가 선택된 상태에서 그리면, 원하는 정점 데이터가 사용되지 않을 수 있습니다.
OpenGL 3.3 Core Profile에서는 VAO가 필요한 상태에서 VAO 없이 그리려고 하면 오류가 발생할 수 있습니다.

### Q4. `glUniform1f`와 `glBindVertexArray`의 순서도 반드시 고정인가요?

아닙니다. 중요한 것은 다음 두 관계입니다.

```text
glUseProgram → glUniform1f
glBindVertexArray → glDrawElements
```

uniform 설정과 VAO 바인딩은 서로 독립적인 상태를 설정하므로, 다음 순서도 동작할 수 있습니다.

```cpp
glBindVertexArray(vertex_array);
glUseProgram(shader_program);
glUniform1f(time_location, current_time);
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
```

다만 다음과 같이 프로그램을 먼저 선택하고 uniform을 설정한 뒤 VAO를 선택하는 순서는 역할이 분명해서 자주 사용됩니다.

```cpp
glUseProgram(shader_program);
glUniform1f(time_location, current_time);
glBindVertexArray(vertex_array);
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
```

결국 `glDrawElements`가 실행되는 순간에 원하는 셰이더 프로그램과 VAO가 모두 현재 상태로 선택되어 있으면 됩니다.

### Q5. 왜 `glUniform1f`를 `glDrawElements` 전에 호출하나요?

`glDrawElements`가 실행될 때 버텍스 셰이더와 프래그먼트 셰이더가 동작하기 때문입니다.
그리기 전에 uniform 값을 설정해야 셰이더가 이번 draw call에서 사용할 최신 값을 읽을 수 있습니다.

```text
시간 계산
    ↓
uniform 업데이트
    ↓
draw call
    ↓
셰이더가 최신 시간 사용
```

그리기 이후에 uniform을 변경하면 이미 완료된 draw call의 결과가 바뀌지 않습니다.
변경된 값은 다음 draw call부터 사용됩니다.

### Q6. 이 순서는 매 프레임 반복해야 하나요?

값의 변경 여부에 따라 다릅니다.

`glUseProgram`과 `glBindVertexArray`는 draw call을 실행할 때 현재 상태로 선택되어 있어야 합니다.
시간처럼 계속 변하는 값은 매 프레임 uniform을 업데이트해야 합니다.

```cpp
while (!glfwWindowShouldClose(window))
{
    glUseProgram(shader_program);
    glUniform1f(time_location, current_time);
    glBindVertexArray(vertex_array);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}
```

반면 고정 색상처럼 바뀌지 않는 uniform은 프로그램을 선택한 뒤 한 번 설정하고 그대로 사용할 수도 있습니다.
다만 여러 프로그램이나 draw call을 번갈아 사용한다면 각 draw call에 필요한 상태가 선택되어 있는지 확인해야 합니다.

### Q7. 네 호출을 한 줄로 요약하면 어떻게 되나요?

```text
1. 어떤 셰이더로 그릴지 선택한다.
2. 그 셰이더가 사용할 공통 값(time)을 설정한다.
3. 어떤 정점 데이터 구성(VAO)으로 그릴지 선택한다.
4. 선택된 셰이더와 VAO로 그리기를 실행한다.
```

즉, 다음 코드는:

```cpp
glUseProgram(shader_program);
glUniform1f(time_location, current_time);
glBindVertexArray(vertex_array);
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
```

**셰이더 선택 → 셰이더 입력 업데이트 → 정점 입력 선택 → 렌더링 실행**의 순서를 표현합니다.
