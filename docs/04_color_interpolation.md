# 04. 정점 색상과 선형 보간

이 예제는 [src/main.cpp](../src/main.cpp)의 사각형 EBO 예제를 다시 삼각형으로 바꾸고,
각 정점에 서로 다른 색상을 지정하는 과정을 설명합니다.

삼각형의 세 꼭짓점에 빨강, 초록, 파랑을 각각 지정하면 OpenGL이 삼각형 내부의 색상을 자동으로 보간합니다.
따라서 별도의 색상 계산 코드를 직접 작성하지 않아도 색상이 부드럽게 섞입니다.

## 1. 예제 결과의 구조

정점 색상은 다음과 같이 설정합니다.

```text
          빨강
           ▲
          / \
         /   \
        /     \
   초록 ◀───────▶ 파랑
```

삼각형 내부의 한 지점은 세 정점으로부터의 상대적인 거리에 따라 색상을 조합합니다.
빨강에 가까운 위치는 빨간색에 가깝고, 가운데로 갈수록 세 색상이 섞인 색상이 됩니다.

## 2. 정점 데이터 변경

기존 EBO 사각형에서는 정점 하나가 위치 두 개의 `float`였습니다.

```text
x, y
```

현재는 정점마다 위치와 색상을 함께 저장합니다.

```text
x, y, r, g, b
```

```cpp
const float vertices[] = {
     0.0f,  0.6f,  1.0f, 0.0f, 0.0f, // 위치 + 빨강
    -0.6f, -0.6f,  0.0f, 1.0f, 0.0f, // 위치 + 초록
     0.6f, -0.6f,  0.0f, 0.0f, 1.0f  // 위치 + 파랑
};
```

정점 하나는 `float` 5개이므로 메모리 구조는 다음과 같습니다.

```text
정점 0: x0, y0, r0, g0, b0
정점 1: x1, y1, r1, g1, b1
정점 2: x2, y2, r2, g2, b2
```

이제 VBO에 저장되는 정점은 3개이고, 각 정점의 크기는 `5 * sizeof(float)`입니다.

## 3. VAO 정점 속성 설정

```cpp
constexpr GLsizei stride = 5 * sizeof(float);
```

`stride`는 현재 정점에서 다음 정점까지 이동하는 바이트 간격입니다.
정점 하나에 `float` 5개가 있으므로 `5 * sizeof(float)`입니다.

### 3.1 위치 속성

```cpp
glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                      stride, nullptr);
glEnableVertexAttribArray(0);
```

* `location = 0`: 버텍스 셰이더의 위치 입력과 연결
* `2`: `x`, `y` 두 개의 값
* `stride`: 다음 정점까지 5개의 `float`
* `nullptr`: 정점 데이터의 시작 위치에서 읽기

위치는 각 정점의 첫 번째 `float`부터 시작합니다.

```text
offset 0 → x
offset sizeof(float) → y
```

### 3.2 색상 속성

```cpp
glVertexAttribPointer(
    1,
    3,
    GL_FLOAT,
    GL_FALSE,
    stride,
    reinterpret_cast<void*>(2 * sizeof(float)));
glEnableVertexAttribArray(1);
```

* `location = 1`: 버텍스 셰이더의 색상 입력과 연결
* `3`: `r`, `g`, `b` 세 개의 값
* `stride`: 다음 정점까지 5개의 `float`
* offset: 위치 `x`, `y` 뒤인 `2 * sizeof(float)`

한 정점의 메모리에서 색상은 위치 뒤에 있습니다.

```text
정점 시작
    ↓
[ x ][ y ][ r ][ g ][ b ]
              ↑
              색상 시작 offset = 2 * sizeof(float)
```

위치와 색상 모두 같은 VBO에서 읽지만, 시작 위치와 읽는 값의 개수가 다릅니다.

## 4. 버텍스 셰이더에서 색상 전달

파일: [`shaders/basic.vert`](../shaders/basic.vert)

```glsl
layout (location = 0) in vec2 position;
layout (location = 1) in vec3 vertex_color;

out vec3 color;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    color = vertex_color;
}
```

버텍스 셰이더는 두 종류의 입력을 받습니다.

* `position`: 위치 속성. C++의 location 0과 연결됩니다.
* `vertex_color`: 색상 속성. C++의 location 1과 연결됩니다.

`color`는 버텍스 셰이더의 출력 변수입니다.
각 정점에서 읽은 색상을 그대로 다음 렌더링 단계로 전달합니다.

## 5. 프래그먼트 셰이더의 색상 입력

파일: [`shaders/basic.frag`](../shaders/basic.frag)

```glsl
in vec3 color;

out vec4 fragment_color;

void main()
{
    fragment_color = vec4(color, 1.0);
}
```

버텍스 셰이더의:

```glsl
out vec3 color;
```

와 프래그먼트 셰이더의:

```glsl
in vec3 color;
```

는 같은 이름과 타입으로 연결됩니다.
프래그먼트 셰이더는 전달받은 RGB 색상에 알파값 `1.0`을 붙여 최종 색상으로 출력합니다.

## 6. 선형 보간은 어디서 일어나나요?

버텍스 셰이더는 세 정점에 대해 세 개의 색상을 출력합니다.

```text
정점 0: 빨강
정점 1: 초록
정점 2: 파랑
```

래스터라이제이션 단계는 세 정점 사이에 생성되는 각 프래그먼트에 대해 `color` 값을 자동으로 보간합니다.
프래그먼트 셰이더는 이미 보간된 값을 입력으로 받습니다.

```text
버텍스 셰이더 출력
빨강, 초록, 파랑
        ↓
래스터라이제이션 단계에서 보간
        ↓
프래그먼트 셰이더 입력
삼각형 위치별로 서로 다른 color
```

개념적으로 어떤 프래그먼트의 색상은 다음처럼 계산됩니다.

```text
보간된 색상 =
    빨강 × 가중치 0
  + 초록 × 가중치 1
  + 파랑 × 가중치 2
```

세 가중치의 합은 1이며, 프래그먼트가 각 정점에 가까울수록 해당 정점의 가중치가 커집니다.
이 방식은 삼각형 내부의 색상이 위치에 따라 선형적으로 변하도록 합니다.

## 7. 왜 `flat`을 사용하지 않나요?

GLSL의 기본 보간 방식은 `smooth`입니다.
따라서 다음과 같이 별도 보간 지정자를 쓰지 않아도 색상이 자동으로 보간됩니다.

```glsl
out vec3 color;
```

만약 다음처럼 `flat`을 사용하면 보간되지 않습니다.

```glsl
flat out vec3 color;
```

`flat`은 삼각형 내부 전체에서 한 정점의 값을 그대로 사용합니다.
현재 예제의 목표는 색상 선형 보간이므로 기본 `smooth` 동작을 사용합니다.

## 8. EBO와 시간 uniform의 변경

이번 예제는 정점 3개만 사용하는 단일 삼각형이므로 EBO가 필요하지 않습니다.
따라서 다음 요소를 제거했습니다.

* `element_buffer` 변수
* `indices` 배열
* `glGenBuffers(1, &element_buffer)`
* `glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ...)`
* EBO에 대한 `glBufferData`
* `glDrawElements`
* `glDeleteBuffers(1, &element_buffer)`

대신 다음을 사용합니다.

```cpp
glDrawArrays(GL_TRIANGLES, 0, 3);
```

또한 이전 예제의 시간 기반 색상 변경은 정점별 색상 보간을 보여주기 위해 제거했습니다.

* 프래그먼트 셰이더의 `uniform float time` 제거
* `glGetUniformLocation` 제거
* 렌더링 루프의 `glUniform1f` 제거
* `sin` 기반 색상 계산 제거

이번 예제에서 색상은 각 정점 데이터에서 직접 결정됩니다.

## 9. 전체 렌더링 흐름

```text
1. 위치와 색상이 포함된 정점 배열 준비
2. 정점 배열을 VBO에 업로드
3. location 0에 위치 속성 연결
4. location 1에 색상 속성 연결
5. 버텍스 셰이더에서 위치와 색상 전달
6. 삼각형 내부에서 색상 자동 보간
7. 프래그먼트 셰이더에서 보간된 색상 출력
```

## 핵심 요약

```text
VBO:
정점마다 x, y, r, g, b 저장

VAO:
location 0 → x, y
location 1 → r, g, b

버텍스 셰이더:
위치와 색상을 다음 단계로 전달

래스터라이제이션:
삼각형 내부에서 색상 선형 보간

프래그먼트 셰이더:
보간된 색상을 최종 픽셀 색상으로 출력
```

즉, **정점별 색상 지정 → 셰이더 간 전달 → 래스터라이제이션 단계의 자동 보간 → 최종 색상 출력** 순서로 동작합니다.

---

## Q&A

### Q1. `flat`을 사용하면 각 정점의 색상만 따로 보이나요?

아닙니다. `flat`은 색상을 없애는 것이 아니라, **색상 보간을 끄고 하나의 정점 색상을 프리미티브 전체에 적용**합니다.

현재 정점 색상이 다음과 같다고 가정하겠습니다.

```text
정점 0: 빨강
정점 1: 초록
정점 2: 파랑
```

기본 보간 방식인 `smooth`에서는 세 정점의 색상이 삼각형 내부에서 섞입니다.

```text
빨강 + 초록 + 파랑
        ↓
삼각형 내부에서 위치에 따라 부드럽게 보간
```

반면 `flat`에서는 세 정점 중 하나의 색상만 선택되어 삼각형 전체에 동일하게 적용됩니다.

```text
정점 색상 3개
        ↓ flat
특정 정점 하나의 색상 선택
        ↓
삼각형 전체에 동일한 색상 적용
```

따라서 `flat`을 사용한 결과는 색상이 없는 삼각형이 아니라 **단색 삼각형**입니다.

### Q2. GLSL에서 `flat`은 어떻게 작성하나요?

버텍스 셰이더와 프래그먼트 셰이더의 대응하는 변수에 `flat`을 지정합니다.

```glsl
// vertex shader
flat out vec3 color;
```

```glsl
// fragment shader
flat in vec3 color;
```

현재 코드처럼 보간 지정자를 생략하면 기본적으로 `smooth`가 사용됩니다.

```glsl
// 기본 동작은 smooth
out vec3 color;
in vec3 color;
```

명시적으로 작성하면 다음과 같은 의미입니다.

```glsl
smooth out vec3 color;
smooth in vec3 color;
```

### Q3. `flat`에서는 어떤 정점의 색상이 사용되나요?

`flat`은 프리미티브의 **provoking vertex**라고 불리는 정점의 값을 사용합니다.
현재처럼 삼각형 하나를 `glDrawArrays`로 그리는 경우에는 기본 설정에서 보통 마지막 정점이 provoking vertex가 됩니다.

```cpp
glDrawArrays(GL_TRIANGLES, 0, 3);
```

정점 순서가 다음과 같다면:

```text
정점 0: 빨강
정점 1: 초록
정점 2: 파랑
```

삼각형 전체가 마지막 정점인 정점 2의 색상, 즉 파란색으로 출력될 수 있습니다.

```text
smooth:
정점 색상 3개 → 삼각형 내부에서 선형 보간

flat:
provoking vertex의 색상 1개 → 삼각형 전체에 동일하게 적용
```

따라서 `flat`은 각 꼭짓점을 서로 다른 색으로 표시하는 기능이 아니라, 정점 속성의 보간을 막고 프리미티브 단위로 하나의 값을 사용하는 기능입니다.
