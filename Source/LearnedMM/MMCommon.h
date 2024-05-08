// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include <math.h>

#define PIf 3.14159265358979323846f
#define LN2f 0.69314718056f
/**
 * 
 */
class LEARNEDMM_API MMCommon
{
public:
	MMCommon();
	~MMCommon();
};

static inline float clampf(float x, float min, float max)
{
    return x > max ? max : x < min ? min : x;
}

static inline float minf(float x, float y)
{
    return x < y ? x : y;
}

static inline float maxf(float x, float y)
{
    return x > y ? x : y;
}

static inline float squaref(float x)
{
    return x * x;
}

//프레임이 달라지면 속도가 달라지는 문제가 있음. 
// -> DeltaTime을 곱하여 문제해결가능. 하지만 댐핑이나 dt를 너무 높게 설정하면 (즉, damping * dt > 1이 되는 경우) 전체 시스템이 불안정해지고, 최악의 경우 폭발할 수 있습니다
// -> damper_exponential 사용으로 해결가능.
static inline float lerpf(float x, float y, float a)
{
    return (1.0f - a) * x + a * y;
}

static inline float signf(float x)
{
    return x > 0.0f ? 1.0f : x < 0.0f ? -1.0f : 0.0f;
}

//음수 지수 함수를 간단한 다항식의 역수로 빠르게 근사하는 것입니다
//불안정한 감쇠기를 빠르고 안정적이며 직관적인 매개변수를 갖는 감쇠기로 변환
static inline float fast_negexpf(float x)
{
    return 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
}

static inline float fast_atanf(float x)
{
    float z = fabs(x);
    float w = z > 1.0f ? 1.0f / z : z;
    float y = (PIf / 4.0f) * w - w * (w - 1.0f) * (0.2447f + 0.0663f * w);
    return copysign(z > 1.0f ? PIf / 2.0f - y : y, x);
}

static inline int clamp(int x, int min, int max)
{
    return x < min ? min : x > max ? max : x;
}
