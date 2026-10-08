# 03. EBO로 사각형 그리기

이 문서는 [src/main.cpp](../src/main.cpp)을 `glDrawArrays` 방식에서 EBO를 사용하는
`glDrawElements` 방식으로 변경한 과정을 설명합니다.

## 1. EBO를 사용하는 이유

현재 도형은 사각형이지만 OpenGL에서는 삼각형 2개로 그립니다.
`GL_TRIANGLES`는 정점 3개마다 삼각형 하나를 만들기 때문입니다.

EBO를 사용하지 않으면 사각형을 다음처럼 정점 6개로 작성해야 합니다.

```text
A, B, C, A, C, D
```

여기서 `A`와 `C`는 두 삼각형에 공통으로 사용되지만 배열에 중복됩니다.
EBO(Element Buffer Object)를 사용하면 정점 데이터는 4개만 저장하고, 어떤 정점을 사용할지 인덱스로 표현할 수 있습니다.

```text
VBO: A, B, C, D
EBO: 0, 1, 2, 0, 2, 3
```

EBO는 정점 데이터를 저장하지 않습니다.

* **VBO**: 정점의 실제 속성 저장
* **EBO**: VBO의 정점을 사용할 순서 저장
* **VAO**: 정점 속성 설정과 VBO/EBO 연결 상태 저장

## 2. 변경 전과 변경 후

### 변경 전: `glDrawArrays`

변경 전에는 사각형을 만들기 위해 정점을 직접 중복해서 저장합니다.

```cpp
const float vertices[] = {
    A, B, C,
    A, C, D
};
```

그리고 VBO를 순서대로 읽습니다.

```cpp
glDrawArrays(GL_TRIANGLES, 0, 6);
```

`glDrawArrays`는 인덱스 버퍼를 사용하지 않고 VBO의 정점을 다음처럼 순서대로 사용합니다.

```text
정점 0, 1, 2 → 삼각형 A, B, C
정점 3, 4, 5 → 삼각형 A, C, D
```

### 변경 후: `glDrawElements`

변경 후에는 꼭짓점 4개만 VBO에 저장합니다.

```cpp
const float vertices[] = {
    -0.6f,  0.6f,  // A: 0
     0.6f,  0.6f,  // B: 1
     0.6f, -0.6f,  // C: 2
    -0.6f, -0.6f   // D: 3
};
```

인덱스 배열은 VBO의 번호를 사용합니다.

```cpp
const unsigned int indices[] = {
    0, 1, 2,
    0, 2, 3
};
```

그림으로 표현하면 다음과 같습니다.

```text
0 ───── 1
│     / │
│   /   │
│ /     │
3 ───── 2
```

인덱스를 3개씩 묶으면 다음 두 삼각형이 됩니다.

```text
0, 1, 2 → A, B, C
0, 2, 3 → A, C, D
```

## 3. 버퍼 객체 선언

```cpp
GLuint vertex_array = 0;
GLuint vertex_buffer = 0;
GLuint element_buffer = 0;
```

기존에는 VAO와 VBO만 필요했지만, 이제 인덱스를 저장할 EBO 핸들이 추가됩니다.

```text
vertex_array  → VAO
vertex_buffer → VBO
element_buffer → EBO
```

`element_buffer`는 C++ 배열을 직접 가리키는 포인터가 아닙니다.
OpenGL이 관리하는 EBO 객체의 이름을 저장하는 `GLuint` 핸들입니다.

## 4. 인덱스 개수 계산

```cpp
constexpr GLsizei index_count =
    static_cast<GLsizei>(sizeof(indices) / sizeof(indices[0]));
```

`indices`에는 `unsigned int` 6개가 있으므로 인덱스 개수는 6입니다.

```text
sizeof(indices)
    전체 배열의 바이트 수

sizeof(indices[0])
    인덱스 하나의 바이트 수

전체 바이트 수 / 요소 하나의 바이트 수
    배열 요소 개수
```

이 값을 사용하면 나중에 인덱스 배열의 길이를 변경해도 `glDrawElements` 호출의 개수를 함께 수정할 필요가 없습니다.
`GLsizei`는 OpenGL의 그리기 개수 인자에 맞는 정수 타입입니다.

## 5. VAO, VBO, EBO 생성

```cpp
glGenVertexArrays(1, &vertex_array);
glGenBuffers(1, &vertex_buffer);
glGenBuffers(1, &element_buffer);
```

각 함수는 OpenGL 객체의 이름을 생성합니다.

* `glGenVertexArrays`: VAO 생성
* 첫 번째 `glGenBuffers`: VBO 생성
* 두 번째 `glGenBuffers`: EBO 생성

아직 데이터가 업로드된 것은 아닙니다.
이 단계에서는 이후 바인딩할 객체를 식별할 핸들만 준비합니다.

## 6. VAO와 VBO 설정

```cpp
glBindVertexArray(vertex_array);
glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(vertices),
    vertices,
    GL_STATIC_DRAW);
```

먼저 VAO를 바인딩한 뒤 VBO를 `GL_ARRAY_BUFFER` 대상으로 바인딩합니다.
`glBufferData`는 CPU의 `vertices` 배열을 현재 VBO로 복사합니다.

이 예제의 VBO는 다음 네 정점을 저장합니다.

```text
VBO[0] = A = (-0.6,  0.6)
VBO[1] = B = ( 0.6,  0.6)
VBO[2] = C = ( 0.6, -0.6)
VBO[3] = D = (-0.6, -0.6)
```

정점 속성 설정은 이전과 같습니다.

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

정점 하나가 여전히 `x`, `y` 두 개의 `float`이므로 VBO의 레이아웃은 바뀌지 않았습니다.
따라서 `glVertexAttribPointer`도 변경할 필요가 없습니다.

## 7. EBO 생성과 인덱스 업로드

```cpp
glBindBuffer(GL_ARRAY_BUFFER, 0);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    sizeof(indices),
    indices,
    GL_STATIC_DRAW);
```

### 7.1 EBO 바인딩

```cpp
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
```

`element_buffer`를 현재 EBO로 선택합니다.
`GL_ELEMENT_ARRAY_BUFFER`는 정점 인덱스를 저장하는 버퍼 대상입니다.

### 7.2 인덱스 배열 업로드

```cpp
glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    sizeof(indices),
    indices,
    GL_STATIC_DRAW);
```

이 코드가 CPU에 있던 `indices` 배열을 GPU의 EBO로 복사합니다.

```text
CPU:
indices = [0, 1, 2, 0, 2, 3]
                │
                │ glBufferData
                ▼
GPU:
EBO = [0, 1, 2, 0, 2, 3]
```

이 업로드가 없으면 `indices`는 C++ 메모리에 선언만 되어 있을 뿐 렌더링에 사용되지 않습니다.
`glDrawElements`는 C++ 배열을 직접 읽지 않고 GPU에 업로드된 EBO에서 인덱스를 읽습니다.

## 8. EBO와 VAO의 연결

EBO를 바인딩하는 시점에 `vertex_array`가 바인딩되어 있으므로, EBO 연결 정보가 현재 VAO에 저장됩니다.

```text
VAO
 ├── 정점 속성 0 설정
 ├── VBO 연결
 └── EBO 연결
```

그래서 설정이 끝난 뒤 VAO를 해제합니다.

```cpp
glBindVertexArray(0);
```

렌더링할 때 다시 VAO를 바인딩하면, VAO에 저장된 정점 속성·VBO·EBO 연결이 함께 선택됩니다.

EBO는 VAO와 연결된 상태로 관리되므로, 다른 EBO를 바인딩하거나 EBO 연결을 해제할 때는 원하는 VAO가 현재 바인딩되어 있는지 확인해야 합니다.

## 9. 그리기 함수 변경

### 이전 코드

```cpp
glDrawArrays(GL_TRIANGLES, 0, 6);
```

VBO의 정점을 0번부터 6개 직접 읽었습니다.

### 현재 코드

```cpp
glBindVertexArray(vertex_array);
glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, nullptr);
```

`glDrawElements`의 인자는 다음과 같습니다.

| 인자 | 값 | 의미 |
|---|---|---|
| mode | `GL_TRIANGLES` | 인덱스 3개마다 삼각형 하나 |
| count | `index_count` | 읽을 인덱스 개수, 현재 6 |
| type | `GL_UNSIGNED_INT` | EBO 원소의 자료형 |
| indices | `nullptr` | EBO의 시작 위치부터 읽음 |

실제 선택 순서는 다음과 같습니다.

```text
EBO[0] = 0 → VBO[0] = A
EBO[1] = 1 → VBO[1] = B
EBO[2] = 2 → VBO[2] = C

EBO[3] = 0 → VBO[0] = A
EBO[4] = 2 → VBO[2] = C
EBO[5] = 3 → VBO[3] = D
```

결과적으로 화면에는 `A-B-C`와 `A-C-D` 두 삼각형이 그려집니다.

## 10. 정리 코드 변경

EBO도 OpenGL 객체이므로 프로그램 종료 시 삭제해야 합니다.

```cpp
glDeleteVertexArrays(1, &vertex_array);
glDeleteBuffers(1, &vertex_buffer);
glDeleteBuffers(1, &element_buffer);
glDeleteProgram(shader_program);
```

기존 VBO와 VAO 정리는 그대로 유지하고, EBO 삭제 코드가 추가되었습니다.

```text
생성: glGenBuffers
사용: glBindBuffer + glBufferData
삭제: glDeleteBuffers
```

## 11. 무엇이 바뀌고 무엇이 그대로인가요?

### 추가된 코드

```cpp
GLuint element_buffer = 0;
const unsigned int indices[] = {
    0, 1, 2,
    0, 2, 3
};
glGenBuffers(1, &element_buffer);
glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    sizeof(indices),
    indices,
    GL_STATIC_DRAW);
glDeleteBuffers(1, &element_buffer);
```

인덱스 배열, EBO 생성, 인덱스 업로드, EBO 삭제가 새로 추가되었습니다.

### 변경된 코드

```cpp
// 변경 전
glDrawArrays(GL_TRIANGLES, 0, 6);

// 변경 후
glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, nullptr);
```

사각형을 구성하는 정점의 저장 방식과 그리기 함수가 변경되었습니다.

### 삭제된 코드

EBO 버전으로 바꾸면서 별도로 삭제된 기능은 없지만, 다음과 같은 중복 정점 데이터는 제거되었습니다.

```text
A, B, C, A, C, D
```

대신 VBO에는 다음 네 정점만 남아 있습니다.

```text
A, B, C, D
```

`glVertexAttribPointer`, 셰이더, 시간에 따른 색상 변경, 렌더링 루프의 화면 지우기와 버퍼 교체는 그대로 유지됩니다.

## 12. 전체 EBO 설정 코드

현재 도형 데이터를 준비하고 버퍼를 설정하는 핵심 부분은 다음과 같습니다.

```cpp
const float vertices[] = {
    -0.6f,  0.6f,
     0.6f,  0.6f,
     0.6f, -0.6f,
    -0.6f, -0.6f
};

const unsigned int indices[] = {
    0, 1, 2,
    0, 2, 3
};

glGenVertexArrays(1, &vertex_array);
glGenBuffers(1, &vertex_buffer);
glGenBuffers(1, &element_buffer);

glBindVertexArray(vertex_array);

glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
glBufferData(
    GL_ARRAY_BUFFER,
    sizeof(vertices),
    vertices,
    GL_STATIC_DRAW);

glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
                      2 * sizeof(float), nullptr);
glEnableVertexAttribArray(0);

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer);
glBufferData(
    GL_ELEMENT_ARRAY_BUFFER,
    sizeof(indices),
    indices,
    GL_STATIC_DRAW);

glBindVertexArray(0);
```

렌더링 시에는 다음처럼 호출합니다.

```cpp
glUseProgram(shader_program);
glBindVertexArray(vertex_array);
glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, nullptr);
```

## 핵심 요약

```text
VBO:
정점 A, B, C, D를 저장

EBO:
0, 1, 2, 0, 2, 3을 저장

VAO:
정점 속성, VBO 연결, EBO 연결을 저장

glDrawElements:
EBO의 번호로 VBO의 정점을 선택해 삼각형 2개를 그림
```

EBO의 전체 흐름은 다음과 같습니다.

```text
indices 배열
    ↓ glBufferData(GL_ELEMENT_ARRAY_BUFFER, ...)
EBO
    ↓ glDrawElements(...)
VBO의 정점 선택
    ↓
버텍스 셰이더와 프래그먼트 셰이더 실행
    ↓
사각형 출력
```
