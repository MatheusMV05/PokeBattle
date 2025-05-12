#ifndef SCALING_H
#define SCALING_H

#include "raylib.h"

// Resolução de referência
#define REF_WIDTH 690
#define REF_HEIGHT 560

// Funções para escala
float GetScaleX(void);
float GetScaleY(void);
Vector2 ScalePosition(float x, float y);
Rectangle ScaleRectangle(float x, float y, float width, float height);
float ScaleFontSize(float fontSize);

#endif // SCALING_H