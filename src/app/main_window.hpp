#pragma once

#include <QMainWindow>
#include <QList>
#include <QScopedPointer>
#include <QString>

#include "about_window.hpp"
#include "scientific_spinbox.hpp"
#include "ui_main_window.h"
#include "../bottom_heating/bottom_heating_solver.hpp"
#include "../help/help_window.hpp"
#include "../opengl_painter/opengl_painter.hpp"
#include "../settings/settings_window.hpp"
#include "../side_heating/side_heating_solver.hpp"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void changeEvent(QEvent *e) override;

private slots:
    void showSettingsWindow();
    void showHelpWindow();
    void on_saveImageButton_clicked();
    void on_startButton_clicked();
    void on_stopButton_clicked();
    void on_heightComboBox_currentIndexChanged(int index);
    void on_tBCComboBox_currentIndexChanged(int index);
    void onSolverFinished();
    void on_paintTButton_clicked();
    void on_paintVxButton_clicked();
    void on_paintVyButton_clicked();
    void on_paintPsiButton_clicked();
    void on_paintOmegaButton_clicked();
    void onIterationFinished(int currentIteration, int maxNumOfIterations, double residual);
    void onMaxIterNumAttained(double residual);

private:
    struct BatchRunSpec {
        QString heightText;
        double prandtl = 1.0;
        double grashof = 100.0;
    };

    void clearRootResultFiles();
    void savePaintAimImage(PaintAim aim, const QString &filename);
    void saveAllResultImages();
    void setControlsEnabled(bool enabled);
    void launchCurrentRun();
    void startNextBatchRun();
    QList<BatchRunSpec> buildBatchQueue(QString *errorMessage) const;
    QList<double> parseNumberList(const QString &text, bool *ok) const;
    QString sanitizeFolderComponent(const QString &value) const;
    QString uniqueFolderNameForCurrentRun() const;
    void copyResultFilesToUniqueFolder();
    QString batchSpecFilePath() const;

private:
    Ui::MainWindow *ui = nullptr;
    OpenGLPainter *glPainter = nullptr;
    AboutWindow *aboutWindow = nullptr;
    QScopedPointer<SettingsWindow> settingsWindow;
    QScopedPointer<HelpWindow> helpWindow;
    QScopedPointer<QScientificSpinbox> prandtlSpinBox;
    QScopedPointer<QScientificSpinbox> grashofSpinBox;
    SideHeatingSolver solverSideHeating;
    BottomHeatingSolver solverBottomHeating;

    QList<BatchRunSpec> batchQueue;
    int currentBatchIndex = -1;
    bool batchModeActive = false;
};
