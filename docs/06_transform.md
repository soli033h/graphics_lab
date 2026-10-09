# 06. 이동, 회전, 크기 변경

이 문서는 현재 사각형 렌더링 예제를 바탕으로 도형의 **이동(translation)**,
**회전(rotation)**, **크기 변경(scaling)** 을 이해하는 과정을 설명합니다.
이번 단계에서는 정점 배열 자체를 매번 수정하지 않고, 변환 행렬을 셰이더에 전달해 도형을 변형하는 방법을 다룹니다.

## 1. 변환이 필요한 이유

현재 정점 데이터는 다음과 같이 화면 좌표에 가까운 위치를 직접 저장합니다.

```cpp
const float vertices[] = {
    -0.6f,  0.6f,
     0.6f,  0.6f,
     0.6f, -0.6f,
    -0.6f, -0.6f
};
```

이 배열은 도형의 기본 모양을 정의합니다.
하지만 도형을 움직일 때마다 배열의 모든 좌표를 CPU에서 다시 계산하는 방식은 관리하기 어렵습니다.

```text
기본 정점 데이터
    ↓
변환 행렬 적용
    ↓
화면에 표시할 최종 위치
```

변환 행렬을 사용하면 정점 데이터는 그대로 유지하면서 도형의 위치, 방향, 크기만 바꿀 수 있습니다.

```text
같은 VBO
    ├─ 변환 없음       → 원래 위치
    ├─ 이동 행렬       → 다른 위치
    ├─ 회전 행렬       → 다른 방향
    └─ 크기 행렬       → 다른 크기
```

## 2. 변환 행렬이란?

행렬은 정점에 수학적 변환을 적용하는 도구입니다.
OpenGL에서는 보통 4x4 행렬인 `mat4`를 사용합니다.

```glsl
uniform mat4 transform;
```

정점 위치에 행렬을 **곱하면** 변환된 위치를 얻을 수 있습니다.

```glsl
gl_Position = transform * vec4(position, 0.0, 1.0);
```

현재 정점은 2차원 위치이지만, 행렬 계산을 위해 `vec4`로 확장합니다.

```text
vec2 position
    ↓
vec4(position, 0.0, 1.0)
    ↓
transform 행렬과 곱셈
    ↓
gl_Position
```

여기서:

* `z = 0.0`: 2D 도형을 깊이 0인 평면에 배치
* `w = 1.0`: 위치 좌표임을 나타냄

## 3. 이동(Translation)

이동은 도형 전체를 다른 위치로 옮기는 변환입니다.

```text
왼쪽 → 오른쪽
아래 → 위
```

예를 들어 모든 정점의 x 좌표에 같은 값을 더하면 도형 전체가 오른쪽으로 이동합니다.
행렬을 사용하면 각 정점에 직접 값을 더하지 않고 이동 행렬 하나로 같은 효과를 얻습니다.

개념적으로 이동 행렬은 다음과 같습니다.

```text
[ 1  0  0  tx ]
[ 0  1  0  ty ]
[ 0  0  1  tz ]
[ 0  0  0   1 ]
```

* `tx`: x 방향 이동량
* `ty`: y 방향 이동량
* `tz`: z 방향 이동량

2D 예제에서는 보통 `tz`를 `0`으로 둡니다.

```text
원래 정점:
(-0.6, 0.6)

오른쪽으로 tx = 0.2 이동:
(-0.4, 0.6)
```

이동은 도형의 모양이나 크기를 바꾸지 않고 위치만 변경합니다.

## 4. 회전(Rotation)

회전은 특정 기준점을 중심으로 도형의 방향을 바꾸는 변환입니다.
2D에서는 z축을 기준으로 회전합니다.

개념적인 z축 회전 행렬은 다음과 같습니다.

```text
[ cos(θ)  -sin(θ)  0  0 ]
[ sin(θ)   cos(θ)  0  0 ]
[   0        0     1  0 ]
[   0        0     0  1 ]
```

* `θ`: 회전 각도
* `cos(θ)`, `sin(θ)`: 각도에 따른 회전 계산

각도 단위에 주의해야 합니다.
대부분의 C++ 수학 함수와 GLM 함수는 각도를 라디안으로 받습니다.

```text
180도 = π 라디안
90도  = π / 2 라디안
```

회전은 기본적으로 원점 `(0, 0)`을 중심으로 적용됩니다.
따라서 화면 중앙이 아닌 다른 지점을 중심으로 회전하려면 이동을 함께 사용해야 합니다.

## 5. 크기 변경(Scaling)

크기 변경은 도형을 확대하거나 축소하는 변환입니다.

개념적인 크기 행렬은 다음과 같습니다.

```text
[ sx  0   0  0 ]
[ 0   sy  0  0 ]
[ 0   0   sz 0 ]
[ 0   0   0  1 ]
```

* `sx`: x 방향 크기 배율
* `sy`: y 방향 크기 배율
* `sz`: z 방향 크기 배율

값의 의미는 다음과 같습니다.

```text
sx = 1.0 → x 크기 그대로
sx = 2.0 → x 방향으로 2배 확대
sx = 0.5 → x 방향으로 절반 축소
```

`sx`와 `sy`를 다르게 설정하면 도형이 늘어나거나 찌그러집니다.

```text
sx = sy       → 비율을 유지한 확대·축소
sx != sy      → 가로·세로 비율이 달라짐
```

크기 변경도 기본적으로 원점 기준으로 적용됩니다.

## 6. 변환 순서

이동, 회전, 크기 변경을 함께 사용하면 **행렬을 곱하는 순서가 중요합니다.**
행렬 곱셈은 일반적으로 교환되지 않습니다.

```text
A × B != B × A
```

예를 들어 다음 두 순서는 서로 다른 결과를 만듭니다.

```text
크기 변경 → 회전 → 이동
이동 → 회전 → 크기 변경
```

일반적으로 물체를 만들 때 다음 순서를 많이 사용합니다.

```text
Scale
  ↓
Rotate
  ↓
Translate
```

이를 모델 행렬로 표현하면 보통 다음처럼 작성합니다.

```text
Model = Translation × Rotation × Scale
```

셰이더에서 정점에 적용할 때는 오른쪽에 있는 변환부터 적용되는 것으로 이해하면 됩니다.

```text
Model × position
    = Translation × Rotation × Scale × position
```

따라서:

1. 기본 모양의 크기를 조절합니다.
2. 조절된 도형을 회전합니다.
3. 회전된 도형을 원하는 위치로 이동합니다.

## 7. 모델 행렬(Model Matrix)

모델 행렬은 하나의 물체를 월드 공간에 배치하는 행렬입니다.

```text
Model = Translation × Rotation × Scale
```

현재 예제의 사각형을:

* 원점 기준으로 확대하고
* 회전한 다음
* 화면의 오른쪽으로 옮기는

작업을 하나의 `model` 행렬로 표현할 수 있습니다.

버텍스 셰이더에서는 다음과 같이 사용합니다.

```glsl
uniform mat4 model;

void main()
{
    gl_Position = model * vec4(position, 0.0, 1.0);
}
```

`model`은 `uniform`이므로 CPU에서 계산한 행렬을 셰이더에 전달합니다.

## 8. 뷰 행렬(View Matrix)

뷰 행렬은 카메라의 위치와 방향을 반영합니다.
카메라가 움직인다는 것은 실제로는 월드 전체를 카메라의 반대 방향으로 변환하는 것과 같습니다.

```text
월드 공간
    ↓ View Matrix
카메라 기준 공간
```

2D 예제에서는 카메라가 원점에 고정되어 있어 뷰 행렬을 생략하는 경우가 많습니다.
하지만 3D로 확장하면 카메라 위치와 방향을 표현하기 위해 필요합니다.

## 9. 투영 행렬(Projection Matrix)

투영 행렬은 카메라 공간을 화면에 투영하는 방법을 결정합니다.

```text
카메라 공간
    ↓ Projection Matrix
클립 공간
    ↓ OpenGL의 정규화 장치 좌표 변환
화면
```

대표적인 투영 방식은 두 가지입니다.

### 직교 투영(Orthographic Projection)

원근감이 없는 투영입니다.
멀리 있는 물체도 같은 크기로 보입니다.

```text
2D UI, 스프라이트, 평면적인 게임 화면
```

### 원근 투영(Perspective Projection)

거리에 따라 물체가 작아지는 투영입니다.

```text
3D 장면, 카메라, 큐브, 모델
```

## 10. MVP 행렬

모델, 뷰, 투영 행렬을 모두 사용하는 경우 다음처럼 구성합니다.

```text
MVP = Projection × View × Model
```

버텍스 셰이더에서는 다음과 같이 적용합니다.

```glsl
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position =
        projection * view * model
        * vec4(position, 0.0, 1.0);
}
```

각 행렬의 역할은 다음과 같습니다.

```text
Model
    물체 자체의 위치·회전·크기

View
    카메라 기준으로 장면 변환

Projection
    3차원 장면을 화면에 투영
```

현재 2D 사각형에서 단순히 이동·회전·크기 변경만 할 때는 `model` 행렬 하나부터 시작하면 됩니다.

## 11. GLM으로 행렬 만들기

행렬을 직접 계산할 수도 있지만, 일반적으로 GLM(Graphics Mathematics)을 사용합니다.
GLM은 GLSL과 비슷한 문법의 C++ 수학 라이브러리입니다.

개념적인 사용 순서는 다음과 같습니다.

```cpp
glm::mat4 model(1.0f);
model = glm::translate(model, position);
model = glm::rotate(model, angle, axis);
model = glm::scale(model, scale);
```

각 함수의 역할은 다음과 같습니다.

* `glm::mat4(1.0f)`: 단위 행렬로 시작
* `glm::translate`: 이동 행렬 적용
* `glm::rotate`: 회전 행렬 적용
* `glm::scale`: 크기 행렬 적용

각도는 라디안으로 변환해야 합니다.

```cpp
float angle = glm::radians(45.0f);
```

현재 프로젝트에 GLM이 연결되어 있지 않다면, 먼저 의존성을 추가하는 단계가 필요합니다.
그 전에는 행렬의 개념과 변환 순서를 먼저 이해하는 것이 좋습니다.

## 12. 행렬을 셰이더에 전달하는 위치

Uniform 값의 일반적인 흐름은 다음과 같습니다.

```text
셰이더에 uniform mat4 선언
    ↓
프로그램 링크 후 위치 조회
    ↓
렌더링 전에 셰이더 프로그램 선택
    ↓
glUniformMatrix4fv로 행렬 전달
    ↓
draw call
```

셰이더:

```glsl
uniform mat4 model;
```

C++:

```cpp
GLint model_location =
    glGetUniformLocation(shader_program, "model");
```

렌더링 시:

```cpp
glUseProgram(shader_program);
glUniformMatrix4fv(
    model_location,
    1,
    GL_FALSE,
    matrix_data);
```

시간이나 입력에 따라 행렬이 바뀐다면 행렬 계산과 uniform 업데이트를 렌더링 루프에서 수행합니다.
행렬이 고정되어 있다면 위치 조회와 값 설정을 한 번만 수행해도 됩니다.

## 13. 회전 중심(Pivot)

현재 정점은 원점 주변에 배치되어 있으므로 원점을 중심으로 회전하면 자연스럽게 보입니다.
하지만 도형이 원점에서 멀리 떨어져 있으면 원점 주위를 크게 돌아갑니다.

특정 점을 중심으로 회전하려면 다음 순서가 필요합니다.

```text
회전 중심을 원점으로 이동
    ↓
회전
    ↓
원래 위치로 되돌리기
```

행렬로 표현하면 다음과 같은 구조입니다.

```text
Model = Translate(position)
      × Translate(pivot)
      × Rotate(angle)
      × Translate(-pivot)
      × Scale(scale)
```

이 원리는 문을 경첩 기준으로 회전하거나, 캐릭터의 팔을 어깨 기준으로 회전할 때 사용합니다.

## 14. 화면 밖으로 나가는 이유

현재 정점 좌표는 대략 `-1.0 ~ 1.0` 범위의 정규화 장치 좌표를 사용합니다.
변환 결과가 이 범위를 크게 벗어나면 도형 일부 또는 전체가 화면 밖으로 잘릴 수 있습니다.

```text
-1.0 ~ 1.0 안쪽 → 화면에 보일 수 있음
범위를 벗어남   → 클리핑될 수 있음
```

예를 들어 크기 배율을 지나치게 키우거나 이동량을 크게 설정하면 도형이 사라진 것처럼 보일 수 있습니다.
이는 VAO나 VBO 오류가 아니라 변환된 위치가 화면 영역 밖에 있기 때문일 수 있습니다.

## 15. 종횡비(Aspect Ratio)

윈도우가 정사각형이 아니면 x축과 y축의 화면상 길이가 다르게 보일 수 있습니다.
현재 800x600 창에서는 같은 NDC 거리라도 가로와 세로의 픽셀 비율이 다릅니다.

```text
윈도우 비율 = width / height
```

회전이나 원형 도형을 다룰 때 종횡비를 고려하지 않으면 모양이 찌그러져 보일 수 있습니다.
이 문제는 이후 투영 행렬이나 뷰포트 크기 보정으로 해결합니다.

## 16. 변환 디버깅 순서

변환이 예상대로 동작하지 않을 때는 다음 순서로 확인합니다.

1. 셰이더의 `uniform mat4` 이름과 C++의 uniform 이름이 같은지 확인합니다.
2. 프로그램 링크 후 uniform 위치를 조회했는지 확인합니다.
3. `glUseProgram` 후 행렬을 전달하는지 확인합니다.
4. 단위 행렬부터 사용해 원래 모양이 보이는지 확인합니다.
5. 이동만 적용해 위치가 바뀌는지 확인합니다.
6. 크기 변경만 적용해 크기가 바뀌는지 확인합니다.
7. 회전만 적용해 방향이 바뀌는지 확인합니다.
8. 마지막으로 여러 변환을 합칩니다.
9. 변환 결과가 `-1.0 ~ 1.0` 범위를 지나치게 벗어나지 않는지 확인합니다.

한 번에 모든 행렬을 적용하기보다 변환 하나씩 확인하면 순서 문제를 찾기 쉽습니다.

## 핵심 요약

```text
이동:
도형의 위치 변경

회전:
도형의 방향 변경

크기 변경:
도형의 크기 변경

모델 행렬:
이동 × 회전 × 크기를 하나로 결합

셰이더:
gl_Position = model * position
```

현재 학습 단계에서의 핵심 흐름은 다음과 같습니다.

```text
기본 정점 데이터
    ↓
CPU에서 변환 행렬 계산
    ↓
uniform mat4로 셰이더에 전달
    ↓
버텍스 셰이더에서 정점에 행렬 적용
    ↓
변환된 도형 렌더링
```

정점 배열을 직접 수정하는 대신 행렬을 변경하면 같은 VBO와 EBO를 사용하면서도 도형을 이동하고, 회전하고, 확대·축소할 수 있습니다.

---

## Q&A

### Q1. `glUniformMatrix4fv`의 인자는 각각 무엇을 의미하나요?

행렬 uniform을 셰이더에 전달할 때 다음 함수를 사용합니다.

```cpp
glUniformMatrix4fv(
    transform_location,
    1,
    GL_FALSE,
    transform);
```

각 인자의 의미는 다음과 같습니다.

```text
transform_location
    → transform uniform의 위치

1
    → 전달할 mat4 행렬의 개수

GL_FALSE
    → 행렬을 전치하지 않음

transform
    → 16개 float 배열의 시작 주소
```

함수 이름도 다음처럼 나눠 읽을 수 있습니다.

```text
glUniformMatrix4fv
            │ │ │
            │ │ └─ float 자료형
            │ └─── 4x4 행렬
            └───── uniform 행렬 설정
```

### Q2. `transform_location`은 무엇인가요?

```cpp
GLint transform_location =
    glGetUniformLocation(shader_program, "transform");
```

`transform_location`은 셰이더의 `transform` uniform이 프로그램 내부에서 어느 위치에 있는지 나타내는 번호입니다.
이 값 자체가 행렬 데이터는 아닙니다.

```text
"transform"
    ↓ glGetUniformLocation
uniform 위치 번호
    ↓ glUniformMatrix4fv
행렬 데이터 기록
```

uniform 위치는 셰이더 프로그램이 링크된 뒤 한 번 조회합니다.
행렬 값은 애니메이션처럼 계속 바뀔 수 있으므로 필요할 때마다 다시 전달합니다.

위치를 찾지 못하면 `glGetUniformLocation`은 `-1`을 반환할 수 있으므로 `GLint`를 사용합니다.

### Q3. 두 번째 인자 `1`은 왜 필요한가요?

```cpp
glUniformMatrix4fv(
    transform_location,
    1,
    GL_FALSE,
    transform);
```

두 번째 인자는 전달할 행렬의 개수입니다.

```text
1 → mat4 하나
2 → mat4 두 개
```

현재는 도형 하나에 적용할 변환 행렬 하나만 전달하므로 `1`을 사용합니다.
여러 행렬을 연속된 배열로 전달하는 경우에는 개수를 늘릴 수 있습니다.

행렬 하나의 크기는 `4 × 4 = 16`개의 `float`입니다.
하지만 두 번째 인자는 `float` 개수가 아니라 **mat4 객체의 개수**라는 점에 주의해야 합니다.

### Q4. `GL_FALSE`는 무엇을 전치하지 않는다는 뜻인가요?

행렬의 행과 열을 서로 바꾸는 연산을 전치(transpose)라고 합니다.

원래 행렬:

```text
[ a b c d ]
[ e f g h ]
[ i j k l ]
[ m n o p ]
```

전치한 행렬:

```text
[ a e i m ]
[ b f j n ]
[ c g k o ]
[ d h l p ]
```

`glUniformMatrix4fv`의 세 번째 인자가 `GL_FALSE`이면 OpenGL이 전달된 행렬을 전치하지 않고 사용합니다.

```cpp
GL_FALSE
```

는 다음 의미입니다.

```text
CPU에서 준비한 행렬 배치를 그대로 사용
```

반대로 `GL_TRUE`를 전달하면 OpenGL이 행과 열을 바꾼 뒤 사용합니다.

현재 예제처럼 OpenGL 방식의 column-major 배열을 직접 구성했다면 일반적으로 다음을 사용합니다.

```cpp
GL_FALSE
```

행렬이 예상과 다르게 회전하거나 이동한다면 배열의 저장 순서와 `GL_FALSE` 설정이 서로 맞는지 확인해야 합니다.

### Q5. `transform`은 왜 16개 `float` 배열의 시작 주소인가요?

GLSL의 `mat4`는 4행 4열의 행렬이므로 숫자가 총 16개 필요합니다.

```text
4 × 4 = 16
```

C++에서는 이를 다음처럼 연속된 배열로 표현할 수 있습니다.

```cpp
float transform[16] = {
    // column 0
     1.0f, 0.0f, 0.0f, 0.0f,
    // column 1
     0.0f, 1.0f, 0.0f, 0.0f,
    // column 2
     0.0f, 0.0f, 1.0f, 0.0f,
    // column 3
     0.0f, 0.0f, 0.0f, 1.0f
};
```

배열 이름 `transform`은 함수 인자로 전달될 때 첫 번째 요소의 주소로 변환됩니다.

```cpp
transform
```

은 다음과 비슷하게 사용됩니다.

```cpp
&transform[0]
```

따라서 `glUniformMatrix4fv`는 `transform`이 가리키는 메모리에서 16개의 `float`를 읽어 `mat4` 하나로 셰이더에 복사합니다.

```text
C++ 배열:
[ f0 ][ f1 ][ f2 ] ... [ f15 ]
   ↑
transform 또는 &transform[0]
```

`glUniformMatrix4fv`의 마지막 인자는 `const GLfloat*`에 해당하는 행렬 데이터 주소를 요구하므로, `float[16]` 배열 이름을 그대로 전달할 수 있습니다.

### Q6. 배열의 저장 순서가 왜 중요한가요?

행렬의 숫자 16개가 메모리에 어떤 순서로 저장되는지 OpenGL이 알아야 행렬을 올바르게 해석할 수 있습니다.

현재 예제처럼 column-major 순서를 사용하면 사람이 읽는 행렬:

```text
[ cosθ  -sinθ  0  offset ]
[ sinθ   cosθ  0    0    ]
[  0      0    1    0    ]
[  0      0    0    1    ]
```

를 C++ 배열에서는 열 단위로 저장합니다.

```cpp
float transform[16] = {
    cosine,  sine,    0.0f, 0.0f,
   -sine,    cosine,  0.0f, 0.0f,
    0.0f,    0.0f,    1.0f, 0.0f,
    offset,  0.0f,    0.0f, 1.0f
};
```

열 4의 첫 번째 값인 `offset`이 배열의 인덱스 `12`에 위치하는 이유도 이 저장 순서 때문입니다.

```text
transform[12] → x축 이동값
transform[13] → y축 이동값
transform[14] → z축 이동값
```

행 우선 순서로 배열을 작성하고 `GL_FALSE`를 사용하면 회전 방향이나 이동 결과가 예상과 다르게 나올 수 있습니다.

### Q7. `transform`은 CPU 변수인가요, GPU 변수인가요?

`transform`이라는 이름은 코드 위치에 따라 서로 다른 대상을 가리킬 수 있습니다.

```cpp
float transform[16];
```

는 CPU 메모리에 있는 C++ 배열입니다.

```glsl
uniform mat4 transform;
```

는 GPU의 셰이더 프로그램 안에 있는 GLSL uniform입니다.

둘은 자동으로 연결되지 않습니다.
다음 호출이 CPU 배열의 내용을 GPU uniform으로 복사합니다.

```cpp
glUniformMatrix4fv(
    transform_location,
    1,
    GL_FALSE,
    transform);
```

관계는 다음과 같습니다.

```text
C++ transform[16]
    ↓ 마지막 인자로 주소 전달
glUniformMatrix4fv
    ↓ 16개 float 복사
GLSL uniform mat4 transform
    ↓
gl_Position 계산에 사용
```

### Q8. 행렬을 전달하는 시점은 언제인가요?

행렬을 사용할 셰이더 프로그램을 먼저 선택한 뒤, draw call 전에 전달합니다.

```cpp
glUseProgram(shader_program);
glUniformMatrix4fv(
    transform_location,
    1,
    GL_FALSE,
    transform);

glBindVertexArray(vertex_array);
glDrawElements(...);
```

시간에 따라 행렬이 바뀌는 경우에는 행렬 배열을 매 프레임 새로 계산하고 `glUniformMatrix4fv`도 매 프레임 호출합니다.
반면 고정된 행렬이라면 한 번 설정한 값을 여러 draw call에서 사용할 수 있습니다.
