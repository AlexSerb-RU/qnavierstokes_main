#pragma once

#include <QOpenGLWidget>

#include "../app/paint_aim.hpp"
#include "color_scale.hpp"

class OpenGLPainter : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit OpenGLPainter(QWidget *parent = nullptr);

    void processData();
    void setPaintAim(PaintAim aim);
    void setColorScheme(ColorScheme scheme);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void processDataFile(const QString &filepath);
    void paintGlFile(const QString &filepath);
    void drawCoordinateTicks(double xMax, double yMax, double viewSize);

private:
    PaintAim paintAim = NotDefine;
    ColorScheme scheme;
};
