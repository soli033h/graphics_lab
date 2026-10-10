# 10. GLM 사용하기

GLM(OpenGL Mathematics)은 OpenGL에서 자주 사용하는 벡터와 행렬 계산을 제공하는 C++ 수학 라이브러리입니다.
직접 `float[16]` 배열을 만들지 않고도 3D 변환 행렬을 작성할 수 있습니다.

현재 프로젝트는 큐브와 카메라를 추가하는 단계이므로 다음 작업에 GLM을 사용할 수 있습니다.

```text
Model 행렬 생성
View 행렬 생성
Projection 행렬 생성
행렬 회전·이동·크기 변경
행렬을 셰이더 uniform으로 전달
```

## 1. GLM은 무엇을 제공하는가?

GLM은 다음과 같은 타입과 함수를 제공합니다.

```cpp
glm::vec2
glm::vec3
glm::vec4
glm::mat3
glm::mat4
```

3D 큐브에서는 주로 다음 타입을 사용합니다.

```cpp
glm::vec3 position;
glm::mat4 transform;
```

GLM의 함수는 CPU에서 행렬을 계산합니다.
OpenGL이나 GLSL 셰이더 안에서 자동으로 계산되는 것은 아닙니다.

```text
C++ 코드에서 GLM으로 행렬 생성
    ↓
glUniformMatrix4fv로 GPU에 전달
    ↓
버텍스 셰이더에서 정점에 적용
```

## 2. GLM 추가하기

GLM은 헤더 중심 라이브러리이므로, 프로젝트에 추가한 뒤 필요한 헤더를 include하면 됩니다.

현재 [CMakeLists.txt](../CMakeLists.txt)는 GLFW를 `FetchContent`로 가져오고 있습니다.
GLM도 같은 방식으로 추가할 수 있습니다.

```cmake
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glm)
```

그다음 실행 파일이 GLM의 헤더를 찾을 수 있도록 연결합니다.

```cmake
target_link_libraries(graphics_lab
    PRIVATE
        graphics_lab_options
        glm::glm
)
```

GLM은 헤더 전용이므로 별도의 `.lib` 파일을 직접 링크하는 방식이 아닙니다.
`glm::glm` 타깃을 연결하면 include 경로와 필요한 설정을 함께 사용할 수 있습니다.

## 3. GLM 헤더 include

행렬과 변환 함수를 사용하려면 다음 헤더를 include합니다.

```cpp
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
```

각 헤더의 역할은 다음과 같습니다.

| 헤더 | 역할 |
|---|---|
| `glm/glm.hpp` | `vec`, `mat` 같은 기본 타입 |
| `glm/gtc/matrix_transform.hpp` | `translate`, `rotate`, `scale`, `lookAt`, `perspective` |
| `glm/gtc/type_ptr.hpp` | 행렬 데이터를 OpenGL 함수에 전달할 포인터 |

현재 [src/main.cpp](../src/main.cpp)의 OpenGL include 근처에 추가하면 됩니다.

```cpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
```

## 4. `glm::mat4` 생성

`glm::mat4(1.0f)`는 항등 행렬을 만듭니다.

```cpp
glm::mat4 model(1.0f);
glm::mat4 view(1.0f);
glm::mat4 projection(1.0f);
```

항등 행렬은 변환을 적용하지 않는 행렬입니다.

```text
model = 항등 행렬
    → 큐브의 기본 위치와 크기

view = 항등 행렬
    → 카메라 변환 없음

projection = 항등 행렬
    → 원근 투영 없음
```

처음에는 세 행렬을 항등 행렬로 만들고, 하나씩 변환을 추가하면 각 역할을 확인하기 쉽습니다.

## 5. Model 행렬 만들기

Model 행렬은 큐브의 위치, 회전, 크기를 결정합니다.

### 이동

```cpp
glm::mat4 model(1.0f);
model = glm::translate(
    model,
    glm::vec3(0.0f, 0.0f, -3.0f));
```

이 코드는 큐브를 `z = -3.0` 방향으로 이동합니다.

### 크기 변경

```cpp
model = glm::scale(
    model,
    glm::vec3(1.0f, 1.0f, 1.0f));
```

세 값을 다르게 지정하면 축별로 크기를 변경할 수 있습니다.

```cpp
glm::vec3(2.0f, 1.0f, 1.0f)
```

이 경우 x축 방향으로 2배 늘어납니다.

### 회전

GLM의 회전 각도는 라디안 단위입니다.
도 단위를 사용하고 싶다면 `glm::radians`로 변환합니다.

```cpp
model = glm::rotate(
    model,
    glm::radians(30.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));
```

마지막 `vec3`는 회전축입니다.

```text
(1, 0, 0) → x축 회전
(0, 1, 0) → y축 회전
(0, 0, 1) → z축 회전
```

시간에 따라 회전하려면 `glfwGetTime()`을 이용할 수 있습니다.

```cpp
const float angle =
    static_cast<float>(glfwGetTime()) * rotation_speed;

model = glm::rotate(
    model,
    angle,
    glm::vec3(0.0f, 1.0f, 0.0f));
```

## 6. View 행렬 만들기

View 행렬은 카메라의 위치와 방향을 표현합니다.
가장 이해하기 쉬운 방법은 `glm::lookAt`을 사용하는 것입니다.

```cpp
const glm::vec3 camera_position(0.0f, 0.0f, 3.0f);
const glm::vec3 camera_target(0.0f, 0.0f, 0.0f);
const glm::vec3 camera_up(0.0f, 1.0f, 0.0f);

const glm::mat4 view = glm::lookAt(
    camera_position,
    camera_target,
    camera_up);
```

각 인자의 의미는 다음과 같습니다.

```text
camera_position
    카메라의 위치

camera_target
    카메라가 바라보는 위치

camera_up
    카메라의 위쪽 방향
```

위 예에서는 카메라가 `(0, 0, 3)`에 있고 원점의 큐브를 바라봅니다.

## 7. Projection 행렬 만들기

원근 투영은 `glm::perspective`로 만들 수 있습니다.

```cpp
const float field_of_view = glm::radians(45.0f);
const float aspect_ratio =
    static_cast<float>(width) / static_cast<float>(height);
const float near_plane = 0.1f;
const float far_plane = 100.0f;

const glm::mat4 projection = glm::perspective(
    field_of_view,
    aspect_ratio,
    near_plane,
    far_plane);
```

각 값의 의미는 다음과 같습니다.

```text
field_of_view
    카메라의 시야각

aspect_ratio
    화면 너비 / 화면 높이

near_plane
    이 거리보다 가까운 물체는 잘림

far_plane
    이 거리보다 먼 물체는 잘림
```

화면 비율은 논리적인 창 크기보다 framebuffer 크기를 사용하는 것이 좋습니다.
고해상도 디스플레이에서는 둘이 다를 수 있습니다.

```cpp
int framebuffer_width = 0;
int framebuffer_height = 0;
glfwGetFramebufferSize(
    window,
    &framebuffer_width,
    &framebuffer_height);
```

높이가 0인 경우에는 aspect ratio를 계산하지 않도록 확인해야 합니다.

## 8. 버텍스 셰이더 수정

파일: [`shaders/shader.vert`](../shaders/shader.vert)

GLM 행렬을 사용하더라도 셰이더 쪽 선언은 일반 GLSL `mat4`입니다.

```glsl
#version 330 core

layout (location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position =
        projection *
        view *
        model *
        vec4(position, 1.0);
}
```

GLM의 `glm::mat4`와 GLSL의 `mat4`가 서로 직접 공유되는 것은 아닙니다.
CPU에서 만든 GLM 행렬을 OpenGL uniform 함수로 업로드해야 합니다.

## 9. GLM 행렬을 uniform으로 전달

행렬을 전달하기 전에 셰이더 프로그램을 사용합니다.

```cpp
glUseProgram(shader_program);
```

uniform 위치는 프로그램 링크가 끝난 뒤 조회합니다.

```cpp
const GLint model_location =
    glGetUniformLocation(shader_program, "model");
const GLint view_location =
    glGetUniformLocation(shader_program, "view");
const GLint projection_location =
    glGetUniformLocation(shader_program, "projection");
```

GLM 행렬의 첫 번째 원소 주소는 `glm::value_ptr`로 얻습니다.

```cpp
glUniformMatrix4fv(
    model_location,
    1,
    GL_FALSE,
    glm::value_ptr(model));

glUniformMatrix4fv(
    view_location,
    1,
    GL_FALSE,
    glm::value_ptr(view));

glUniformMatrix4fv(
    projection_location,
    1,
    GL_FALSE,
    glm::value_ptr(projection));
```

여기서:

```text
model_location
    model uniform 위치

1
    mat4 하나

GL_FALSE
    전치하지 않음

glm::value_ptr(model)
    GLM 행렬 내부의 16개 float 시작 주소
```

GLM은 OpenGL에 전달하기 적합한 column-major 행렬 배치를 기본으로 사용하므로 일반적으로 `GL_FALSE`를 전달합니다.

## 10. 초기화와 렌더링 루프의 구분

행렬의 종류에 따라 계산 위치가 다릅니다.

### 한 번만 계산해도 되는 행렬

카메라가 움직이지 않고 창 크기도 고정이라면 다음은 초기화 때 계산할 수 있습니다.

```text
view
projection
```

### 매 프레임 계산해야 하는 행렬

큐브가 회전하거나 이동한다면 다음은 매 프레임 다시 계산합니다.

```text
model
```

일반적인 구조는 다음과 같습니다.

```text
초기화:
    view 생성
    projection 생성
    uniform 위치 조회

렌더링 루프:
    model 생성
    model uniform 전달
    view uniform 전달
    projection uniform 전달
    큐브 그리기
```

처음에는 세 행렬을 매 프레임 전달해도 동작에는 문제가 없습니다.
나중에 카메라나 창 크기가 바뀔 때 필요한 행렬만 갱신하도록 정리할 수 있습니다.

## 11. GLM을 이용한 전체 흐름

```cpp
glm::mat4 model(1.0f);
model = glm::rotate(
    model,
    glm::radians(30.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

glm::mat4 view = glm::lookAt(
    glm::vec3(0.0f, 0.0f, 3.0f),
    glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

glm::mat4 projection = glm::perspective(
    glm::radians(45.0f),
    aspect_ratio,
    0.1f,
    100.0f);
```

그다음 셰이더에 전달합니다.

```text
model
    → glUniformMatrix4fv

view
    → glUniformMatrix4fv

projection
    → glUniformMatrix4fv
```

셰이더에서는 다음 계산이 수행됩니다.

```text
projection × view × model × position
```

## 12. 변환 순서 주의하기

GLM의 변환 함수는 기존 행렬을 받아 새로운 행렬을 반환합니다.

```cpp
model = glm::translate(model, position);
model = glm::rotate(model, angle, axis);
```

여러 변환을 적용할 때는 호출 순서와 최종 결과를 확인해야 합니다.
일반적으로 물체 변환은 다음 개념으로 이해합니다.

```text
Scale
    ↓
Rotate
    ↓
Translate
```

사용 목적에 따라 다음처럼 하나의 체인으로 작성할 수도 있습니다.

```cpp
glm::mat4 model(1.0f);
model = glm::translate(model, position);
model = glm::rotate(model, angle, axis);
model = glm::scale(model, scale);
```

처음에는 이동, 회전, 크기를 각각 한 줄씩 적용하고 결과를 확인하는 것이 좋습니다.

## 13. 현재 코드에 적용할 때의 위치

현재 [src/main.cpp](../src/main.cpp)에 GLM을 적용하는 순서는 다음과 같습니다.

```text
1. include 영역에 GLM 헤더 추가
2. CMakeLists.txt에 GLM FetchContent 추가
3. graphics_lab 실행 파일에 glm::glm 연결
4. GLAD 초기화 이후 행렬 생성
5. shader.vert에 mat4 uniform 추가
6. glGetUniformLocation으로 uniform 위치 조회
7. 렌더링 루프에서 glUseProgram 호출
8. glm::value_ptr로 행렬 전달
9. glBindVertexArray 후 큐브 그리기
```

GLM은 셰이더 파일에서 include하지 않습니다.

```text
C++:
    GLM 사용

GLSL:
    mat4 사용
```

## 14. 자주 발생하는 문제

### `glm/glm.hpp`를 찾을 수 없는 경우

```text
GLM이 CMake에 추가되지 않음
glm::glm이 실행 파일에 연결되지 않음
CMake 재구성이 되지 않음
```

`CMakeLists.txt`를 수정한 뒤에는 CMake configure/reconfigure가 다시 수행되어야 합니다.

### 큐브가 보이지 않는 경우

```text
큐브가 카메라 뒤에 있음
view와 projection을 전달하지 않음
uniform 이름이 셰이더와 다름
projection의 near/far 범위가 잘못됨
```

### 큐브가 평평하게 보이는 경우

```text
model 회전이 없음
projection 행렬이 없음
view 행렬이 없음
```

카메라가 정면을 보고 있으면 회전하지 않은 큐브의 한 면만 보이는 것이 정상입니다.

### `GL_FALSE`와 전치 문제

GLM은 OpenGL에 맞는 column-major 배치를 기본으로 사용하므로 다음처럼 전달하는 것이 일반적입니다.

```cpp
glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
```

`GL_TRUE`로 바꾸기 전에 행렬 배치와 셰이더 계산 순서를 먼저 확인해야 합니다.

## 핵심 요약

```text
GLM
    → C++에서 벡터와 행렬 계산

glm::mat4
    → Model, View, Projection 표현

glm::value_ptr
    → 행렬 데이터를 OpenGL에 전달할 주소 제공

glUniformMatrix4fv
    → GLM 행렬을 셰이더 uniform으로 업로드
```

```glsl
gl_Position =
    projection *
    view *
    model *
    vec4(position, 1.0);
```

GLM은 렌더링 자체를 수행하는 라이브러리가 아니라, 렌더링에 필요한 벡터·행렬 계산을 간단하게 해주는 라이브러리입니다.
