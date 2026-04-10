#pragma once

#include <QImage>
#include <QOpenGLWidget>
#include <QRectF>

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
    QImage exportCurrentImage() const;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void processDataFile(const QString &filepath);
    void paintGlFile(const QString &filepath);
    QRectF computePlotRectPixels(int targetWidth, int targetHeight, double xMax, double yMax) const;
    QRectF computePlotRectPixels(double xMax, double yMax) const;
    void paintCoordinateTicks(QPainter &painter, double xMax, double yMax, const QRectF &plotRect) const;
    void drawCoordinateTicks(double xMax, double yMax, const QRectF &plotRect);

private:
    PaintAim paintAim = NotDefine;
    ColorScheme scheme;
    double currentXMax = 1.0;
    double currentYMax = 1.0;
    bool hasCurrentDomain = false;
};
