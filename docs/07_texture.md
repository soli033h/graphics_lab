# 07. 텍스처 매핑

이 문서는 현재 [src/main.cpp](../src/main.cpp)의 EBO 사각형에 이미지 텍스처를 입히는 과정을 설명합니다.
현재 예제는 단색을 출력하지만, 텍스처를 사용하면 사각형의 각 프래그먼트에 이미지의 픽셀을 대응시킬 수 있습니다.

## 1. 텍스처란?

텍스처는 GPU가 읽을 수 있도록 준비한 이미지 데이터입니다.
도형 표면에 이미지를 입히는 작업을 **텍스처 매핑(texture mapping)** 이라고 합니다.

```text
이미지 픽셀
    ↓
텍스처 객체에 업로드
    ↓
텍스처 좌표로 샘플 위치 지정
    ↓
프래그먼트 셰이더에서 색상 읽기
    ↓
화면에 이미지 출력
```

현재 프래그먼트 셰이더는 모든 프래그먼트에 같은 색상을 출력합니다.

```glsl
fragment_color = vec4(0.2, 0.7, 1.0, 1.0);
```

텍스처를 사용하면 이 고정된 색상 대신 이미지에서 읽은 색상을 출력합니다.

```glsl
fragment_color = texture(image, texture_coordinate);
```

## 2. 텍스처 좌표(UV 좌표)

정점 위치만으로는 이미지의 어느 부분을 도형에 붙일지 알 수 없습니다.
각 정점에 이미지 안의 위치를 나타내는 텍스처 좌표를 추가해야 합니다.

일반적으로 텍스처 좌표는 `u`, `v` 또는 `s`, `t`로 부르며 `0.0 ~ 1.0` 범위를 사용합니다.

```text
(0, 1) ───────── (1, 1)
  왼쪽 위          오른쪽 위
    │                  │
    │      이미지      │
    │                  │
(0, 0) ───────── (1, 0)
  왼쪽 아래        오른쪽 아래
```

사각형의 정점에 위치와 UV를 함께 저장하면 다음과 같은 구조가 됩니다.

```text
x, y, u, v
```

```cpp
const float vertices[] = {
    // position       // texture coordinate
    -0.6f,  0.6f,     0.0f, 1.0f, // 왼쪽 위
     0.6f,  0.6f,     1.0f, 1.0f, // 오른쪽 위
     0.6f, -0.6f,     1.0f, 0.0f, // 오른쪽 아래
    -0.6f, -0.6f,     0.0f, 0.0f  // 왼쪽 아래
};
```

UV 좌표는 정점 사이에서 자동으로 보간됩니다.
따라서 프래그먼트 셰이더는 각 프래그먼트에 해당하는 이미지 위치를 받게 됩니다.

## 3. 정점 레이아웃 변경

현재 위치만 저장하는 정점은 다음과 같습니다.

```text
x, y
```

텍스처를 추가하면 정점 하나가 `float` 4개가 됩니다.

```text
x, y, u, v
```

따라서 다음 값이 바뀝니다.

```cpp
constexpr GLsizei stride = 4 * sizeof(float);
```

### 위치 속성

위치는 정점의 처음 두 `float`에 있습니다.

```cpp
glVertexAttribPointer(
    0,
    2,
    GL_FLOAT,
    GL_FALSE,
    4 * sizeof(float),
    nullptr);
glEnableVertexAttribArray(0);
```

### 텍스처 좌표 속성

UV는 위치 뒤의 두 `float`에 있습니다.

```cpp
glVertexAttribPointer(
    1,
    2,
    GL_FLOAT,
    GL_FALSE,
    4 * sizeof(float),
    reinterpret_cast<void*>(2 * sizeof(float)));
glEnableVertexAttribArray(1);
```

메모리 구조는 다음과 같습니다.

```text
정점 시작
    ↓
[ x ][ y ][ u ][ v ]
  └ 위치 ┘  └ UV ┘
```

`location = 0`은 위치, `location = 1`은 텍스처 좌표로 사용합니다.

## 4. 버텍스 셰이더에서 UV 전달

파일: [`shaders/shader.vert`](../shaders/shader.vert)

버텍스 셰이더는 텍스처 좌표를 입력으로 받고 프래그먼트 셰이더로 전달합니다.

```glsl
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 input_texture_coordinate;

out vec2 texture_coordinate;

void main()
{
    gl_Position = vec4(position, 0.0, 1.0);
    texture_coordinate = input_texture_coordinate;
}
```

위치와 UV는 모두 정점마다 입력됩니다.
`texture_coordinate`는 정점 사이에서 기본적으로 `smooth` 보간됩니다.

```text
정점 UV
    ↓
래스터라이제이션 단계에서 보간
    ↓
프래그먼트별 UV
```

## 5. 프래그먼트 셰이더에서 텍스처 샘플링

파일: [`shaders/shader.frag`](../shaders/shader.frag)

프래그먼트 셰이더에는 텍스처 좌표 입력과 `sampler2D` uniform을 선언합니다.

```glsl
in vec2 texture_coordinate;

uniform sampler2D image;

out vec4 fragment_color;

void main()
{
    fragment_color = texture(image, texture_coordinate);
}
```

`sampler2D`는 2차원 텍스처를 읽기 위한 셰이더 변수입니다.
`texture` 함수는 UV 좌표에 해당하는 텍스처 색상을 찾아 반환합니다.

```text
texture(image, texture_coordinate)
    → 해당 UV 위치의 RGBA 색상
```

## 6. 텍스처 객체 생성

텍스처 이미지를 GPU로 업로드하려면 텍스처 객체가 필요합니다.

```cpp
GLuint texture = 0;
glGenTextures(1, &texture);
glBindTexture(GL_TEXTURE_2D, texture);
```

* `glGenTextures`: 텍스처 객체 이름 생성
* `glBindTexture`: 사용할 2D 텍스처 선택

VBO와 마찬가지로 `texture` 변수는 이미지 데이터 그 자체가 아니라 OpenGL 텍스처 객체의 이름을 저장합니다.

```text
texture
    → OpenGL 텍스처 객체 식별자

이미지 픽셀 데이터
    → 텍스처 객체에 업로드할 CPU 측 데이터
```

## 7. 이미지 파일 읽기

OpenGL은 PNG나 JPG 파일을 직접 읽지 않습니다.
이미지 파일을 디코딩해 픽셀 배열로 바꾸는 별도 라이브러리가 필요합니다.

학습용 프로젝트에서는 보통 `stb_image`를 사용합니다.
`stb_image`는 헤더 하나로 사용할 수 있는 이미지 로딩 라이브러리입니다.

개념적인 사용 흐름은 다음과 같습니다.

```cpp
int width = 0;
int height = 0;
int channels = 0;

unsigned char* pixels = stbi_load(
    "image.png",
    &width,
    &height,
    &channels,
    0);
```

반환되는 `pixels`는 이미지의 각 픽셀 채널이 연속으로 저장된 CPU 메모리입니다.

```text
pixels:
R, G, B, A, R, G, B, A, ...
```

사용이 끝나면 반드시 해제합니다.

```cpp
stbi_image_free(pixels);
```

이미지 로더는 파일을 읽고 픽셀 배열로 변환하는 역할만 합니다.
OpenGL 텍스처 객체 생성과 GPU 업로드는 별도로 수행합니다.

## 8. 픽셀 데이터를 GPU에 업로드

이미지를 읽은 뒤 `glTexImage2D`로 텍스처 객체에 업로드합니다.

```cpp
glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA,
    width,
    height,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    pixels);
```

주요 인자는 다음과 같습니다.

| 인자 | 의미 |
|---|---|
| `GL_TEXTURE_2D` | 2차원 텍스처 대상 |
| `0` | 기본 mipmap 레벨 |
| `GL_RGBA` | GPU에 저장할 내부 형식 |
| `width`, `height` | 이미지 크기 |
| `0` | 예약된 border 값 |
| `GL_RGBA` | 입력 픽셀의 채널 형식 |
| `GL_UNSIGNED_BYTE` | 채널 하나의 자료형 |
| `pixels` | CPU 픽셀 데이터 시작 주소 |

이미지가 RGB만 사용하는 경우에는 `GL_RGB`를 사용해야 할 수 있습니다.
내부 형식과 입력 형식은 실제 이미지 채널 수와 맞춰야 합니다.

## 9. Mipmap 생성

텍스처가 멀리 보이거나 작게 표시될 때 사용할 축소 이미지를 만들 수 있습니다.

```cpp
glGenerateMipmap(GL_TEXTURE_2D);
```

Mipmap은 원본 텍스처에서 더 작은 해상도의 텍스처들을 미리 생성합니다.

```text
원본: 1024 × 1024
    ↓
512 × 512
    ↓
256 × 256
    ↓
...
```

Mipmap은 축소된 텍스처의 품질과 성능을 개선하는 데 도움이 됩니다.
Mipmap을 사용할 경우 축소 필터도 mipmap 레벨을 사용하도록 설정합니다.

## 10. 텍스처 필터링

텍스처 크기와 화면에 표시되는 크기가 다를 때 필터링 방식이 적용됩니다.

```cpp
glTexParameteri(
    GL_TEXTURE_2D,
    GL_TEXTURE_MIN_FILTER,
    GL_LINEAR_MIPMAP_LINEAR);

glTexParameteri(
    GL_TEXTURE_2D,
    GL_TEXTURE_MAG_FILTER,
    GL_LINEAR);
```

### `GL_NEAREST`

가장 가까운 한 픽셀을 선택합니다.

```text
선명하고 픽셀화된 결과
```

픽셀 아트에 적합합니다.

### `GL_LINEAR`

주변 픽셀을 보간해 부드러운 결과를 만듭니다.

```text
부드러운 확대·축소
```

### 확대와 축소 필터

* `GL_TEXTURE_MIN_FILTER`: 텍스처가 화면보다 작게 보일 때
* `GL_TEXTURE_MAG_FILTER`: 텍스처가 화면보다 크게 보일 때

Mipmap을 사용한다면 `GL_TEXTURE_MIN_FILTER`에는 mipmap을 포함한 필터를 사용합니다.

## 11. 텍스처 래핑

UV 좌표가 `0.0 ~ 1.0` 범위를 벗어날 때 텍스처를 어떻게 반복할지 지정합니다.

```cpp
glTexParameteri(
    GL_TEXTURE_2D,
    GL_TEXTURE_WRAP_S,
    GL_REPEAT);

glTexParameteri(
    GL_TEXTURE_2D,
    GL_TEXTURE_WRAP_T,
    GL_REPEAT);
```

* `S`: 가로 방향, `U`와 대응
* `T`: 세로 방향, `V`와 대응

주요 래핑 방식은 다음과 같습니다.

| 방식 | 동작 |
|---|---|
| `GL_REPEAT` | 텍스처 반복 |
| `GL_MIRRORED_REPEAT` | 반복할 때 좌우 반전 |
| `GL_CLAMP_TO_EDGE` | 가장자리 픽셀을 늘림 |
| `GL_CLAMP_TO_BORDER` | 지정한 테두리 색 사용 |

## 12. 텍스처 유닛과 sampler uniform

OpenGL은 여러 텍스처를 동시에 사용하기 위해 텍스처 유닛을 제공합니다.

```cpp
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, texture);
```

프래그먼트 셰이더의 sampler uniform에는 텍스처 자체가 아니라 텍스처 유닛 번호를 전달합니다.

```cpp
GLint image_location =
    glGetUniformLocation(shader_program, "image");

glUseProgram(shader_program);
glUniform1i(image_location, 0);
```

`0`은 `GL_TEXTURE0`에 대응합니다.

```text
GL_TEXTURE0
    ↔ texture unit 번호 0
    ↔ sampler2D image에 전달할 값 0
```

둘의 관계는 다음과 같습니다.

```text
glActiveTexture(GL_TEXTURE0)
    ↓
glBindTexture(GL_TEXTURE_2D, texture)
    ↓
glUniform1i(image_location, 0)
    ↓
sampler2D image가 texture unit 0에서 텍스처 읽기
```

## 13. 바인딩 순서

텍스처를 그리기 전에 다음 상태가 준비되어 있어야 합니다.

```text
셰이더 프로그램 선택
    ↓
sampler uniform에 텍스처 유닛 번호 전달
    ↓
사용할 텍스처 유닛 활성화
    ↓
텍스처 객체 바인딩
    ↓
VAO 바인딩
    ↓
glDrawElements
```

개념적인 렌더링 코드는 다음과 같습니다.

```cpp
glUseProgram(shader_program);
glUniform1i(image_location, 0);

glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, texture);

glBindVertexArray(vertex_array);
glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
```

## 14. 텍스처와 EBO의 관계

텍스처를 추가해도 EBO의 역할은 바뀌지 않습니다.

```text
VBO:
위치와 UV 저장

EBO:
정점 사용 순서 저장

텍스처:
이미지 픽셀 저장

VAO:
위치와 UV 속성 설정, VBO/EBO 연결 저장
```

인덱스가 같은 정점을 재사용하면 그 정점의 위치와 UV가 함께 재사용됩니다.
따라서 한 꼭짓점에서 서로 다른 UV를 사용해야 하는 경우에는 정점을 분리해야 합니다.

## 15. 이미지가 거꾸로 보이는 경우

이미지 로더와 OpenGL의 세로축 기준이 다르면 텍스처가 위아래로 뒤집혀 보일 수 있습니다.

해결 방법은 보통 두 가지입니다.

### UV 좌표를 뒤집기

```text
v = 0.0 ↔ v = 1.0
```

### 이미지 로더에서 수직 뒤집기

`stb_image`를 사용하는 경우 로드 전에 수직 뒤집기 옵션을 설정할 수 있습니다.

어떤 방식이든 프로젝트 전체에서 좌표 기준을 일관되게 유지하는 것이 중요합니다.

## 16. 리소스 정리

텍스처 객체도 사용이 끝나면 삭제합니다.

```cpp
glDeleteTextures(1, &texture);
```

이미지 로더가 할당한 CPU 메모리도 별도로 해제해야 합니다.

```cpp
stbi_image_free(pixels);
```

두 메모리는 서로 다른 자원입니다.

```text
stbi_image_free
    → CPU 이미지 픽셀 배열 해제

glDeleteTextures
    → GPU 텍스처 객체 삭제
```

## 17. 텍스처 매핑 전체 흐름

```text
1. 이미지 파일을 CPU 픽셀 배열로 읽기
2. 텍스처 객체 생성
3. 텍스처 파라미터 설정
4. 픽셀 데이터를 GPU에 업로드
5. mipmap 생성
6. 위치와 UV가 포함된 정점 데이터 준비
7. VAO에 위치와 UV 속성 연결
8. 버텍스 셰이더에서 UV 전달
9. sampler uniform에 텍스처 유닛 번호 전달
10. 프래그먼트 셰이더에서 texture() 호출
11. EBO로 사각형 그리기
12. CPU와 GPU 텍스처 리소스 정리
```

## 18. 현재 단색 사각형과 비교

### 현재 방식

```text
VBO:
x, y

프래그먼트 셰이더:
고정된 vec4 색상 출력
```

### 텍스처 방식

```text
VBO:
x, y, u, v

버텍스 셰이더:
UV 전달

프래그먼트 셰이더:
texture(image, texture_coordinate) 출력
```

EBO와 `glDrawElements`는 두 방식에서 그대로 사용할 수 있습니다.
달라지는 부분은 정점 속성에 UV를 추가하고, 이미지 텍스처를 생성해 샘플링하는 것입니다.

## 핵심 요약

```text
이미지 파일
    ↓
CPU 픽셀 배열
    ↓ glTexImage2D
GPU 텍스처 객체
    ↓ sampler2D + texture()
프래그먼트 색상
```

```text
정점 위치 + UV
    ↓
VBO
    ↓
VAO에서 location 0, 1로 해석
    ↓
버텍스 셰이더가 UV 전달
    ↓
프래그먼트 셰이더가 텍스처 샘플링
    ↓
EBO 사각형에 이미지 출력
```

텍스처의 핵심은 **이미지 데이터 자체**, **텍스처 좌표**, **sampler uniform**, **텍스처 객체 바인딩**을 연결하는 것입니다.
