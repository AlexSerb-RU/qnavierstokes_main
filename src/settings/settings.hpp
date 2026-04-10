#pragma once
#include <QtCore/QString>

// Цветовая схема. Определяет какая цветовая гамма будет использоваться для отрисовки результата.
enum class ColorScheme
{
    BlueRed,
    Rainbow
};

// Движок отрисовки результатов.
enum class PaintEngine
{
    OpenGL,
};

struct Settings
{
    bool useUniqueFolders;  // Запись каждого решения в отдельную папку
    PaintEngine paintEngine;  // Используемый движок отрисовки результата
    ColorScheme colorScheme;  // Цветовая гамма, используемая для отрисовки результата
    bool limitPrGr;  // Ограничивать значения чисел Pr и Gr
    bool scientificPrGr;  // Отображать Pr и Gr в экспоненциальной нотации
};
