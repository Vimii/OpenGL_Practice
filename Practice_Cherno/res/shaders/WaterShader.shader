#shader vertex
#version 400 core
layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texCoord;

out vec4 worldPosition;

uniform mat4 u_Projection;
uniform mat4 u_View;
uniform mat4 u_Model;

void main()
{

    worldPosition = u_Model * position;
    gl_Position = u_Projection * u_View * worldPosition;

}



#shader fragment
#version 400 core

in vec4 worldPosition;

layout(location = 0) out vec4 FragColor;

uniform vec4 u_Color;
uniform vec2 u_resolution;
uniform float u_time;
uniform vec3 cPos, cDir, cUp;

//ランダム関数 
float random(in vec2 p) {
    return fract(sin(dot(p.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

//ノイズ関数 
float noise(in vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

// FBMを定義
#define OCTAVES 4
float fbm(in vec2 p) {
    float value = 0.0;
    float amplitud = 0.2;
    float frequency = .9;
    for (int i = 0; i < OCTAVES; i++) {
        value += amplitud * noise(p);
        p *= 2.0;
        amplitud *= 0.8;
    }
    return value;
}

float sWave(vec3 p) {
    vec3 n = normalize(vec3(0.0, 1.0, 0.0));

    // 複数の非整数ブラウン運動
    float s = fbm(vec2(p.x, length(p * 1.5) - u_time * 1.25) * .075);
    float t = fbm(vec2(p.x, length(p * 2.5) - u_time * 2.75) * .125);
    float u = fbm(vec2(p.x, length(p * 3.5) - u_time * 0.75) * .075);
    float v = s + t + u;
    return dot(p, n) + v;
}

vec3 genNormal(vec3 p) {
    float d = 0.001;
    return normalize(vec3(
        sWave(p + vec3(d, 0.0, 0.0)) - sWave(p + vec3(-d, 0.0, 0.0)),
        sWave(p + vec3(0.0, d, 0.0)) - sWave(p + vec3(0.0, -d, 0.0)),
        sWave(p + vec3(0.0, 0.0, d)) - sWave(p + vec3(0.0, 0.0, -d))
    ));
}

void main()
{
    vec3 sea = vec3(0.6, 0.7, 0.9);

    // colorの初期化
    vec3 color = vec3(0.0);

    // 座標の正規化(range: -1.0 ~ 1.0)
    vec2 p = (gl_FragCoord.xy * 2.0 - u_resolution) / min(u_resolution.x, u_resolution.y);

    // カメラのセッティング
    vec3 cSide = cross(cDir, cUp);          // 横方向を算出
    float targetDepth = 1.0f;                // 深度

    // レイの生成
    vec3 ray = normalize(cSide * p.x + cUp * p.y + cDir * targetDepth);

    // レイマーチングループ
    float dist = 0.0;   // レイと穴の距離
    float rLen = 0.0;   // レイの長さ
    vec3  rPos = cPos;  // レイの先端
    for (int i = 0; i < 32; i++) {
        dist = sWave(rPos);
        rLen += dist;
        rPos = cPos + ray * rLen;
    }

    // 衝突判定で色の出しわけ
    if (abs(dist) < 0.001) {
        vec3 normal = genNormal(rPos);  // 物体の法線情報を取得
        vec3 light = normalize(vec3(sin(u_time), 1.2, 0.0));    // ライトの位置
        float diff = max(dot(normal, light), 0.1);  // 拡散反射光を内積で算出

        vec3 eye = reflect(normalize(rPos - cPos), normal);
        float speculer = pow(clamp(dot(eye, light), 0.0, 1.0) * 1.025, 30.0);   // 反射光を算出

        color = (sea + speculer) * diff;
    }
    else {
        color = vec3(0.0);
    }

    FragColor = vec4(color, 0.3);
}