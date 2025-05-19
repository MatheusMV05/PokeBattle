#include "scaling.h"

// Calcular a escala horizontal com base na resolução atual
float GetScaleX(void) {
    return (float)GetScreenWidth() / REF_WIDTH;
}

// Calcular a escala vertical com base na resolução atual
float GetScaleY(void) {
    return (float)GetScreenHeight() / REF_HEIGHT;
}

// Escalar uma posição XY
Vector2 ScalePosition(float x, float y) {
    return (Vector2) {
        x * GetScaleX(),
        y * GetScaleY()
    };
}

// Escalar um retângulo inteiro (posição e tamanho)
Rectangle ScaleRectangle(float x, float y, float width, float height) {
    float scaleX = GetScaleX();
    float scaleY = GetScaleY();

    return (Rectangle) {
        x * scaleX,
        y * scaleY,
        width * scaleX,
        height * scaleY
    };
}

// Escalar um tamanho de fonte
float ScaleFontSize(float fontSize) {
    return fontSize * ((GetScaleX() + GetScaleY()) / 2.0f);
}