#include "color_scale.hpp"

//============================================================================================================
//Конструктор создает цветовую шкалу аналогичную шкале, создаваемой вызовом функции initScale(0,1,BlueRed).
//============================================================================================================
ColorScale::ColorScale()
{
    initScale(0, 1, ColorScheme::BlueRed);
}

//============================================================================================================
//Функция позволяет получить цвет, соответствующий запрашиваему значению функции.
//============================================================================================================
QColor ColorScale::getColor(double value)
{
    if(value <= values.first())
        return colors.first();

    if(value >= values.last())
        return colors.last();

    int i = 0;
    while(!(value > values[i] && value < values[i + 1]))
        i++;

    QColor final;
    final.setRed((int)(colors[i].red()*(values[i + 1] - value)/(values[i + 1] - values[i])
                       + colors[i + 1].red()*(values[i] - value)/(values[i] - values[i + 1])));
    final.setGreen((int)(colors[i].green()*(values[i + 1] - value)/(values[i + 1] - values[i])
                         + colors[i + 1].green()*(values[i] - value)/(values[i] - values[i + 1])));
    final.setBlue((int)(colors[i].blue()*(values[i + 1] - value)/(values[i + 1] - values[i])
                        + colors[i + 1].blue()*(values[i] - value)/(values[i] - values[i + 1])));

    return final;
}

//============================================================================================================
//Создание новой цветовой шкалы, которая каждому числу из диапазона [min,max] ставит какой либо цвет.
//============================================================================================================
void ColorScale::initScale(double min, double max, ColorScheme scheme)
{
    values.clear();
    colors.clear();

    switch(scheme)
    {
    case ColorScheme::BlueRed:
        colors.append(Qt::blue);
        colors.append(Qt::red);
        values.append(min);
        values.append(max);
        break;
    case ColorScheme::Rainbow:
        values.append(min);
        values.append(min + 1.*(max - min)/5.);
        values.append(min + 2.*(max - min)/5.);
        values.append(min + 3.*(max - min)/5.);
        values.append(min + 4.*(max - min)/5.);
        values.append(max);
        colors.append(QColor(153,102,255)); //Blue Violet
        colors.append(Qt::blue);
        colors.append(Qt::green);
        colors.append(Qt::yellow);
        colors.append(QColor(255,102,0));   //Orange
        colors.append(Qt::red);
        break;
    default:
        colors.append(Qt::blue);
        colors.append(Qt::red);
        values.append(min);
        values.append(max);
        break;
    }
}
