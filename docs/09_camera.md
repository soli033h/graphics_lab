# 09. 3D 카메라 추가하기

현재 [src/main.cpp](../src/main.cpp)는 3D 큐브 정점을 준비하고 있지만, 카메라와 투영이 아직 적용되지 않은 상태입니다.
이 문서는 큐브를 입체적으로 바라보기 위해 카메라를 추가하는 과정을 설명합니다.

## 1. 카메라가 필요한 이유

3D 정점의 `x`, `y`, `z` 좌표를 그대로 `gl_Position`에 넣으면 화면에는 `x`, `y`가 직접 사용됩니다.

```glsl
gl_Position = vec4(position, 1.0);
```

이 방식에서는 큐브의 앞면과 뒷면이 화면에서 같은 위치에 겹쳐 보일 수 있습니다.
3D 공간의 위치를 카메라가 바라보는 화면으로 변환하려면 좌표 변환이 필요합니다.

```text
3D 월드
    ↓ 카메라 기준으로 변환
    ↓ 원근 투영
2D 화면
```

## 2. 3D 렌더링의 세 가지 행렬

3D 정점은 보통 다음 세 행렬을 거칩니다.

```text
Model
    물체 자체의 이동·회전·크기

View
    카메라의 위치와 방향

Projection
    3D 공간을 2D 화면에 투영하는 방식
```

최종 계산은 버텍스 셰이더에서 다음처럼 작성합니다.

```glsl
gl_Position =
    projection *
    view *
    model *
    vec4(position, 1.0);
```

행렬 곱셈은 오른쪽부터 적용됩니다.

```text
position
    ↓ model
월드 공간
    ↓ view
카메라 공간
    ↓ projection
클립 공간
```

## 3. View 행렬이란?

View 행렬은 카메라를 표현하는 행렬입니다.
실제로 카메라를 이동시키는 대신, 월드의 물체를 카메라의 반대 방향으로 이동시킵니다.

```text
카메라가 오른쪽으로 이동
    ↔
월드 전체를 왼쪽으로 이동
```

카메라가 원점에 있고 `-z` 방향을 바라본다고 가정해 보겠습니다.
큐브를 카메라 앞쪽인 `z = -3.0`에 놓으려면 View 행렬에 다음 이동을 적용할 수 있습니다.

```text
큐브 이동: z = -3.0
```

이 상태는 카메라를 이동한 것과 같은 효과를 냅니다.

```text
카메라: (0, 0, 0)
큐브:   (0, 0, -3)
```

## 4. Projection 행렬이란?

Projection 행렬은 카메라 공간의 3D 좌표를 화면에 투영합니다.

### 원근 투영

원근 투영에서는 멀리 있는 물체가 작게 보입니다.

```text
가까운 물체 → 크게 보임
먼 물체     → 작게 보임
```

원근 투영을 만들 때 필요한 값은 다음과 같습니다.

```text
field of view
aspect ratio
near clipping plane
far clipping plane
```

### 직교 투영

직교 투영에서는 거리에 따른 크기 변화가 없습니다.
2D UI나 정면에서 크기 변화를 원하지 않는 장면에 적합합니다.

처음 큐브를 표시할 때는 원근 투영을 사용하는 것이 입체감을 확인하기 쉽습니다.

## 5. 카메라와 큐브의 위치 관계

기본적인 좌표 관계를 먼저 정하는 것이 좋습니다.

```text
카메라 위치: (0, 0, 0)
카메라 방향: -z
큐브 위치:   (0, 0, -3)
```

카메라가 `-z` 방향을 바라보는 경우, 큐브를 `+z`에 배치하면 카메라 뒤에 놓이게 됩니다.

```text
z = 0   카메라
z = -3  카메라 앞
z = +3  카메라 뒤
```

큐브가 보이지 않을 때는 가장 먼저 큐브가 카메라 앞에 있는지 확인합니다.

## 6. 버텍스 셰이더 수정

파일: [`shaders/shader.vert`](../shaders/shader.vert)

현재의 단순한 버텍스 셰이더는 다음과 같은 형태입니다.

```glsl
layout (location = 0) in vec3 position;

void main()
{
    gl_Position = vec4(position, 1.0);
}
```

카메라를 사용하려면 세 개의 `mat4` uniform을 추가합니다.

```glsl
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

프래그먼트 셰이더는 카메라와 직접 관련이 없습니다.
카메라 변환은 정점 위치를 계산하는 버텍스 셰이더에서 수행합니다.

## 7. CPU에서 행렬 준비하기

행렬은 CPU에서 만들고 uniform으로 셰이더에 전달합니다.

개념적으로 다음 세 행렬을 준비합니다.

```text
model
    큐브의 위치와 회전

view
    카메라의 위치와 방향

projection
    원근 투영 설정
```

현재 프로젝트는 GLM을 아직 사용하지 않으므로 두 가지 방법이 있습니다.

### 직접 행렬 만들기

이전 [06_transform.md](./06_transform.md)에서 다룬 방식처럼 `float[16]` 배열을 직접 만들 수 있습니다.
이 방법은 행렬의 메모리 배치와 계산 원리를 이해하는 데 도움이 됩니다.

### GLM 사용하기

GLM을 사용하면 다음과 같은 함수를 이용할 수 있습니다.

```cpp
glm::mat4 model(1.0f);
glm::mat4 view(1.0f);
glm::mat4 projection(1.0f);
```

원근 투영과 카메라 행렬도 라이브러리 함수로 만들 수 있습니다.

```text
glm::perspective
glm::lookAt
glm::translate
glm::rotate
```

어떤 방법을 사용하든 셰이더에 전달되는 최종 결과는 `mat4`입니다.

## 8. `lookAt`으로 카메라 만들기

카메라를 직접 회전 행렬로 만들기보다 위치, 바라보는 점, 위쪽 방향으로 정의하면 이해하기 쉽습니다.

```text
eye
    카메라 위치

center
    카메라가 바라보는 위치

up
    카메라의 위쪽 방향
```

개념적으로는 다음과 같습니다.

```text
카메라 위치:   (0, 0, 0)
바라보는 위치: (0, 0, -1)
위쪽 방향:     (0, 1, 0)
```

이 세 정보로 View 행렬을 만들 수 있습니다.

```text
view = lookAt(eye, center, up)
```

카메라를 `(0, 0, 3)`에 놓고 원점을 바라보는 방식도 자주 사용합니다.

```text
eye    = (0, 0, 3)
center = (0, 0, 0)
up     = (0, 1, 0)
```

이 경우 큐브를 원점에 두어도 카메라에서 떨어진 위치에 있는 것처럼 볼 수 있습니다.

## 9. 원근 투영 설정

원근 투영은 다음 값을 사용합니다.

```text
시야각(FOV): 보통 45도
화면 비율:   width / height
near:        0.1
far:         100.0
```

현재 창 크기가 `800 × 600`이라면 aspect ratio는 다음과 같습니다.

```text
800 / 600 = 1.333...
```

화면 비율을 잘못 넣으면 큐브가 가로 또는 세로로 늘어나 보입니다.

창 크기가 바뀔 때는 framebuffer 크기를 사용해 aspect ratio를 다시 계산해야 합니다.

```text
aspect = framebuffer_width / framebuffer_height
```

높이가 `0`인 특수한 경우에는 나눗셈을 피해야 합니다.

## 10. `glUniformMatrix4fv`로 전달하기

프로그램을 사용한 뒤 각 행렬의 uniform 위치를 조회하고 값을 전달합니다.

```cpp
glUseProgram(shader_program);

GLint model_location =
    glGetUniformLocation(shader_program, "model");
GLint view_location =
    glGetUniformLocation(shader_program, "view");
GLint projection_location =
    glGetUniformLocation(shader_program, "projection");
```

그다음 `glUniformMatrix4fv`를 사용합니다.

```cpp
glUniformMatrix4fv(model_location, 1, GL_FALSE, model_data);
glUniformMatrix4fv(view_location, 1, GL_FALSE, view_data);
glUniformMatrix4fv(
    projection_location,
    1,
    GL_FALSE,
    projection_data);
```

각 인자의 의미는 다음과 같습니다.

```text
location
    uniform 위치

1
    mat4 하나

GL_FALSE
    전치하지 않음

model_data / view_data / projection_data
    16개 float 행렬의 시작 주소
```

uniform 위치는 프로그램 링크 후 한 번만 조회하고 저장해도 됩니다.
매 프레임 조회하는 것보다 초기화 단계에서 미리 가져오는 편이 좋습니다.

## 11. 렌더링 루프에서의 순서

행렬이 시간에 따라 변하지 않는다면 초기화 이후 한 번 전달해도 됩니다.
큐브를 회전시킬 때는 `model`만 매 프레임 갱신합니다.

기본적인 렌더링 순서는 다음과 같습니다.

```text
프레임 시작
    ↓
색상 버퍼와 깊이 버퍼 지우기
    ↓
셰이더 프로그램 사용
    ↓
model, view, projection 전달
    ↓
VAO 바인딩
    ↓
큐브 그리기
    ↓
버퍼 교환
    ↓
이벤트 처리
```

현재 코드의 깊이 버퍼 초기화는 다음 형태여야 합니다.

```cpp
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

그리고 깊이 테스트를 초기화 단계에서 활성화합니다.

```cpp
glEnable(GL_DEPTH_TEST);
```

## 12. 큐브를 회전시켜 여러 면 보기

카메라가 큐브를 정확히 정면에서 바라보면 앞면만 보이는 것이 정상입니다.
여러 면을 보려면 Model 행렬에 회전을 적용합니다.

```text
model = 회전 행렬
```

시간에 따라 회전 각도를 바꾸면 다음 흐름이 됩니다.

```text
현재 시간
    ↓
회전 각도
    ↓
model 행렬
    ↓
model uniform
```

카메라가 아니라 큐브를 회전시키는 것이므로 View 행렬은 그대로 두고 Model 행렬만 바꿉니다.

## 13. 카메라 이동과 큐브 이동의 차이

다음 두 방법은 화면에서 비슷한 결과를 만들 수 있습니다.

```text
카메라를 뒤로 이동
큐브를 앞쪽으로 이동
```

하지만 개념적으로 역할은 다릅니다.

```text
Model
    물체를 월드에 배치

View
    카메라의 시점 결정
```

카메라가 여러 물체를 바라보는 장면을 만들 계획이라면 View 행렬을 카메라 전용으로 유지하는 것이 좋습니다.

## 14. 구현 순서

현재 큐브 예제에 카메라를 추가할 때는 다음 순서가 안전합니다.

```text
1. shader.vert의 position을 vec3로 설정
2. model, view, projection uniform 추가
3. gl_Position에 세 행렬 적용
4. CPU에서 세 행렬 준비
5. 큐브와 카메라의 위치 결정
6. projection의 aspect ratio 설정
7. glUseProgram 이후 행렬 uniform 전달
8. glEnable(GL_DEPTH_TEST) 확인
9. glClear에 GL_DEPTH_BUFFER_BIT 추가
10. 정적 큐브 표시 확인
11. model 행렬에 회전 추가
```

## 15. 보이지 않을 때 확인할 목록

```text
□ position이 vec3인가?
□ stride가 3 * sizeof(float)인가?
□ gl_Position에 z가 포함되는가?
□ 세 행렬 uniform 이름이 셰이더와 같은가?
□ 큐브가 카메라 앞에 있는가?
□ projection의 near/far 범위가 올바른가?
□ aspect ratio가 0으로 나누어지지 않는가?
□ 깊이 테스트가 활성화되어 있는가?
□ 매 프레임 깊이 버퍼를 지우는가?
□ glDrawArrays의 정점 개수가 36인가?
```

## 핵심 요약

```text
Model
    물체 변환

View
    카메라 변환

Projection
    원근 투영
```

```glsl
gl_Position =
    projection *
    view *
    model *
    vec4(position, 1.0);
```

카메라를 추가한다고 해서 프래그먼트 셰이더를 바꿀 필요는 없습니다.
카메라와 원근에 관련된 계산은 버텍스 셰이더에서 수행하며, 프래그먼트 셰이더는 기존처럼 단색을 출력해도 됩니다.
