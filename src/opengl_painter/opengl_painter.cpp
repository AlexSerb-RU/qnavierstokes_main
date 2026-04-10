#include <QApplication>
#include <QColor>
#include <QFile>
#include <QMessageBox>
#include <QPainter>
#include <QTextStream>
#include <QVector>
#include <QtMath>
#include <QLocale>

#include <QOpenGLFunctions>

#include "opengl_painter.hpp"
#include "../color_scale/color_scale.hpp"
#include "../settings/settings_manager.hpp"

namespace {
constexpr int kLeftMarginPx = 52;
constexpr int kRightMarginPx = 12;
constexpr int kBottomMarginPx = 34;
constexpr int kTopMarginPx = 12;
constexpr int kTickLengthPx = 6;
constexpr int kTickLabelGapPx = 3;

double axisTickStep(double axisMax)
{
    if (axisMax <= 0.0) {
        return 0.1;
    }
    return axisMax / 10.0;
}

QString tickLabel(double value)
{
    const double rounded = std::round(value * 10.0) / 10.0;
    const bool isInteger = std::abs(rounded - std::round(rounded)) < 1e-9;
    return QLocale::system().toString(rounded, 'f', isInteger ? 0 : 1);
}

int xToPixel(const QRectF &plotRect, double x, double xMax)
{
    if (xMax <= 0.0) {
        return qRound(plotRect.left());
    }
    return qRound(plotRect.left() + (x / xMax) * plotRect.width());
}

int yToPixel(const QRectF &plotRect, double y, double yMax)
{
    if (yMax <= 0.0) {
        return qRound(plotRect.bottom());
    }
    return qRound(plotRect.bottom() - (y / yMax) * plotRect.height());
}
}

OpenGLPainter::OpenGLPainter(QWidget *parent)
    : QOpenGLWidget(parent)
{
    const auto settings = SettingsManager::instance().settings();
    scheme = settings.colorScheme;
}

void OpenGLPainter::initializeGL()
{
    const QColor clearColor(Qt::white);
    glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());
}

void OpenGLPainter::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

QRectF OpenGLPainter::computePlotRectPixels(double xMax, double yMax) const
{
    const double safeXMax = qMax(xMax, 1e-6);
    const double safeYMax = qMax(yMax, 1e-6);

    const double availableWidth = qMax(width() - kLeftMarginPx - kRightMarginPx, 10);
    const double availableHeight = qMax(height() - kTopMarginPx - kBottomMarginPx, 10);

    const double domainAspect = safeXMax / safeYMax;
    const double availableAspect = availableWidth / availableHeight;

    double plotWidth = availableWidth;
    double plotHeight = availableHeight;

    if (domainAspect > availableAspect) {
        plotHeight = plotWidth / domainAspect;
    } else {
        plotWidth = plotHeight * domainAspect;
    }

    const double left = kLeftMarginPx + (availableWidth - plotWidth) * 0.5;
    const double top = kTopMarginPx + (availableHeight - plotHeight) * 0.5;
    return QRectF(left, top, plotWidth, plotHeight);
}

void OpenGLPainter::drawCoordinateTicks(double xMax, double yMax, const QRectF &plotRect)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setPen(QPen(Qt::black, 1.0));

    const int left = qRound(plotRect.left());
    const int right = qRound(plotRect.right());
    const int top = qRound(plotRect.top());
    const int bottom = qRound(plotRect.bottom());

    painter.drawRect(QRectF(plotRect.left(), plotRect.top(), plotRect.width(), plotRect.height()));

    const double xStep = axisTickStep(xMax);
    const double xEps = xStep * 0.5;
    for (double x = 0.0; x <= xMax + xEps; x += xStep) {
        const double clampedX = qMin(x, xMax);
        const int px = xToPixel(plotRect, clampedX, xMax);
        painter.drawLine(px, bottom + 1, px, bottom + kTickLengthPx);
        painter.drawLine(px, top - 1, px, top - kTickLengthPx);

        const QString label = tickLabel(clampedX);
        const QRect textRect(px - 24, bottom + kTickLengthPx + kTickLabelGapPx, 48, 16);
        painter.drawText(textRect, Qt::AlignHCenter | Qt::AlignTop, label);
    }

    const double yStep = axisTickStep(yMax);
    const double yEps = yStep * 0.5;
    for (double y = 0.0; y <= yMax + yEps; y += yStep) {
        const double clampedY = qMin(y, yMax);
        const int py = yToPixel(plotRect, clampedY, yMax);
        painter.drawLine(left - kTickLengthPx, py, left - 1, py);
        painter.drawLine(right + 1, py, right + kTickLengthPx, py);

        const QString label = tickLabel(clampedY);
        const QRect textRect(0, py - 8, left - kTickLengthPx - kTickLabelGapPx, 16);
        painter.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, label);
    }
}

void OpenGLPainter::paintGlFile(const QString &filepath)
{
    if (!QFile(filepath).exists()) {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, 1, 0, 1, -1, 1);
        return;
    }

    int r = 0;
    int g = 0;
    int b = 0;
    double x1 = 0.0;
    double x2 = 0.0;
    double x3 = 0.0;
    double y1 = 0.0;
    double y2 = 0.0;
    double y3 = 0.0;
    double xMax = 0.0;
    double yMax = 0.0;
    double numOfTriangles = 0.0;

    QFile file(filepath);
    QTextStream iostream(&file);
    file.open(QIODevice::ReadOnly);

    iostream >> xMax >> yMax;

    const QRectF plotRect = computePlotRectPixels(xMax, yMax);
    const qreal dpr = devicePixelRatioF();
    glViewport(
        qRound(plotRect.left() * dpr),
        qRound((height() - plotRect.bottom()) * dpr),
        qRound(plotRect.width() * dpr),
        qRound(plotRect.height() * dpr));

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, qMax(xMax, 1e-6), 0, qMax(yMax, 1e-6), -1, 1);

    iostream >> numOfTriangles;
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < static_cast<int>(numOfTriangles); ++i) {
        iostream >> x1 >> y1 >> r >> g >> b;
        glColor3f(r / 255.0f, g / 255.0f, b / 255.0f);
        glVertex2f(x1, y1);

        iostream >> x2 >> y2 >> r >> g >> b;
        glColor3f(r / 255.0f, g / 255.0f, b / 255.0f);
        glVertex2f(x2, y2);

        iostream >> x3 >> y3 >> r >> g >> b;
        glColor3f(r / 255.0f, g / 255.0f, b / 255.0f);
        glVertex2f(x3, y3);
    }
    glEnd();

    const QColor lineColor(Qt::black);
    glBegin(GL_LINES);
    glColor3f(lineColor.redF(), lineColor.greenF(), lineColor.blueF());
    while (!iostream.atEnd()) {
        iostream >> x1 >> y1 >> x2 >> y2;
        if (iostream.status() != QTextStream::Ok && iostream.status() != QTextStream::ReadPastEnd) {
            break;
        }
        glVertex2f(x1, y1);
        glVertex2f(x2, y2);
    }
    glEnd();

    glColor3f(lineColor.redF(), lineColor.greenF(), lineColor.blueF());
    glLineWidth(2);
    glBegin(GL_LINE_LOOP);
    glVertex2f(0, 0);
    glVertex2f(xMax, 0);
    glVertex2f(xMax, yMax);
    glVertex2f(0, yMax);
    glEnd();
    glLineWidth(1);

    file.close();
    glFinish();

    drawCoordinateTicks(xMax, yMax, plotRect);
}

void OpenGLPainter::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT);

    switch (paintAim) {
    case T:
        paintGlFile(qApp->applicationDirPath() + "/result/T.gl");
        break;
    case Psi:
        paintGlFile(qApp->applicationDirPath() + "/result/Psi.gl");
        break;
    case Omega:
        paintGlFile(qApp->applicationDirPath() + "/result/Omega.gl");
        break;
    case Vx:
        paintGlFile(qApp->applicationDirPath() + "/result/Vx.gl");
        break;
    case Vy:
        paintGlFile(qApp->applicationDirPath() + "/result/Vy.gl");
        break;
    default:
        break;
    }

    glFinish();
}

void OpenGLPainter::setPaintAim(PaintAim aim)
{
    paintAim = aim;
    update();
}

void OpenGLPainter::setColorScheme(ColorScheme newScheme)
{
    if (scheme != newScheme) {
        scheme = newScheme;
        processData();
    }
}

void OpenGLPainter::processData()
{
    if (QFile(qApp->applicationDirPath() + "/result/T.dat").exists()) {
        processDataFile(qApp->applicationDirPath() + "/result/T.dat");
    }
    if (QFile(qApp->applicationDirPath() + "/result/Psi.dat").exists()) {
        processDataFile(qApp->applicationDirPath() + "/result/Psi.dat");
    }
    if (QFile(qApp->applicationDirPath() + "/result/Omega.dat").exists()) {
        processDataFile(qApp->applicationDirPath() + "/result/Omega.dat");
    }
    if (QFile(qApp->applicationDirPath() + "/result/Vx.dat").exists()) {
        processDataFile(qApp->applicationDirPath() + "/result/Vx.dat");
    }
    if (QFile(qApp->applicationDirPath() + "/result/Vy.dat").exists()) {
        processDataFile(qApp->applicationDirPath() + "/result/Vy.dat");
    }
}

void OpenGLPainter::processDataFile(const QString &filepath)
{
    QFile file(filepath);
    QTextStream iostream(&file);
    QList<double> f;
    QList<QVector<double>> nvtr;
    QList<QVector<double>> xy;

    file.open(QIODevice::ReadOnly);
    while (!iostream.atEnd()) {
        double function = 0.0;
        QVector<double> point(2);
        iostream >> point[0] >> point[1] >> function;
        f.append(function);
        xy.append(point);
    }
    file.close();

    if (!f.isEmpty()) {
        f.removeLast();
    }
    if (!xy.isEmpty()) {
        xy.removeLast();
    }

    if (QFile(qApp->applicationDirPath() + "/result/nvtr.dat").exists()) {
        file.setFileName(qApp->applicationDirPath() + "/result/nvtr.dat");
        file.open(QIODevice::ReadOnly);
        while (!iostream.atEnd()) {
            QVector<double> triangle(3);
            iostream >> triangle[0] >> triangle[1] >> triangle[2];
            nvtr.append(triangle);
        }
        file.close();
        if (!nvtr.isEmpty()) {
            nvtr.removeLast();
        }
    } else if (QFile(qApp->applicationDirPath() + "/result/net.dat").exists()) {
        int nx = 0;
        int ny = 0;
        file.setFileName(qApp->applicationDirPath() + "/result/net.dat");
        file.open(QIODevice::ReadOnly);
        iostream >> nx >> ny;
        file.close();

        for (int i = 0; i < ny - 1; ++i) {
            for (int j = 0; j < nx - 1; ++j) {
                QVector<double> triangle1(3);
                triangle1[0] = nx * i + j;
                triangle1[1] = nx * i + j + 1;
                triangle1[2] = nx * (i + 1) + j;

                QVector<double> triangle2(3);
                triangle2[0] = nx * (i + 1) + j;
                triangle2[1] = nx * (i + 1) + j + 1;
                triangle2[2] = nx * i + j + 1;

                nvtr.append(triangle1);
                nvtr.append(triangle2);
            }
        }
    } else {
        QMessageBox::critical(
            this,
            tr("Ошибка обработки данных"),
            tr("Невозможно найти файл\n\"") + qApp->applicationDirPath() + "/result/nvtr.dat" +
                tr("\"\nили\n\"") + qApp->applicationDirPath() + "/result/net.dat" +
                tr("\"\nДальнейшая обработка данных невозможна!"));

        return;
    }

    double minF = f[0];
    double maxF = f[0];
    for (double value : f) {
        minF = qMin(minF, value);
        maxF = qMax(maxF, value);
    }

    double maxX = 0.0;
    double maxY = 0.0;
    for (const QVector<double> &point : xy) {
        maxX = qMax(maxX, point[0]);
        maxY = qMax(maxY, point[1]);
    }

    ColorScale colorScale;
    colorScale.initScale(minF, maxF, scheme);

    QFile ofile(filepath.left(filepath.size() - 4) + ".gl");
    QTextStream ostream(&ofile);
    ofile.open(QIODevice::WriteOnly);

    ostream << maxX << " " << maxY << Qt::endl;
    ostream << nvtr.size() << Qt::endl;
    for (int i = 0; i < nvtr.size(); ++i) {
        for (int localNode = 0; localNode < 3; ++localNode) {
            const int nodeIndex = static_cast<int>(nvtr[i][localNode]);
            const QColor color = colorScale.getColor(f[nodeIndex]);
            ostream << xy[nodeIndex][0] << " " << xy[nodeIndex][1] << " " << color.red() << " "
                    << color.green() << " " << color.blue() << Qt::endl;
        }
    }

    double x[2] = {0.0, 0.0};
    double y[2] = {0.0, 0.0};
    double isolines[15];
    for (int i = 0; i < 15; ++i) {
        isolines[i] = minF + (i + 1) * (maxF - minF) / 16.0;
    }

    for (int i = 0; i < nvtr.size(); ++i) {
        for (double isoline : isolines) {
            int l = 0;
            for (int k = 0; k < 3 && l < 2; ++k) {
                const int i0 = static_cast<int>(nvtr[i][k % 3]);
                const int i1 = static_cast<int>(nvtr[i][(k + 1) % 3]);
                if ((f[i0] >= isoline && f[i1] < isoline) || (f[i0] < isoline && f[i1] >= isoline)) {
                    const double a = (isoline - f[i0]) / (f[i1] - f[i0]);
                    x[l] = xy[i0][0] + a * (xy[i1][0] - xy[i0][0]);
                    y[l] = xy[i0][1] + a * (xy[i1][1] - xy[i0][1]);
                    ++l;
                }
                if (f[i0] == isoline && f[i1] == isoline) {
                    x[0] = xy[i0][0];
                    y[0] = xy[i0][1];
                    x[1] = xy[i1][0];
                    y[1] = xy[i1][1];
                    l = 2;
                }
            }

            if (l == 2) {
                ostream << x[0] << " " << y[0] << " " << x[1] << " " << y[1] << Qt::endl;
            }
        }
    }

    ofile.close();
}
