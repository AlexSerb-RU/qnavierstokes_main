#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QImage>
#include <QPainter>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QTextStream>
#include <QVBoxLayout>

#include "main_window.hpp"
#include "../settings/settings.hpp"
#include "../settings/settings_manager.hpp"

namespace {
QStringList splitListValues(const QString &text)
{
    QString normalized = text;
    normalized.replace(QRegularExpression("\\s+"), "");
    return normalized.split(';', Qt::SkipEmptyParts);
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , settingsWindow(new SettingsWindow(this))
    , helpWindow(new HelpWindow(this))
    , prandtlSpinBox(new QScientificSpinbox(this))
    , grashofSpinBox(new QScientificSpinbox(this))
{
    ui->setupUi(this);

    aboutWindow = new AboutWindow(this);

    if (!QDir(qApp->applicationDirPath()).exists("result")) {
        QDir(qApp->applicationDirPath()).mkdir("result");
    }
    if (!QDir(qApp->applicationDirPath()).exists("tmp")) {
        QDir(qApp->applicationDirPath()).mkdir("tmp");
    }

    const auto settings = SettingsManager::instance().settings();
    on_heightComboBox_currentIndexChanged(0);
    on_tBCComboBox_currentIndexChanged(0);

    prandtlSpinBox->setValue(1.0);
    grashofSpinBox->setValue(100.0);
    ui->formLayout->insertRow(2, prandtlSpinBox.get());
    ui->formLayout->insertRow(4, grashofSpinBox.get());

    glPainter = new OpenGLPainter();
    auto *solutionPainterLayout = new QVBoxLayout();
    solutionPainterLayout->addWidget(glPainter);
    solutionPainterLayout->setContentsMargins(0, 0, 0, 0);
    ui->solutionPainterFrame->setLayout(solutionPainterLayout);

    auto adaptPrGr = [this](const Settings &currentSettings) {
        if (currentSettings.limitPrGr) {
            prandtlSpinBox->setMinimum(0.001);
            prandtlSpinBox->setMaximum(1000);
            grashofSpinBox->setMinimum(0.001);
            grashofSpinBox->setMaximum(100000);
            prandtlSpinBox->setToolTip(tr("Введите число от 0,001 до 1000"));
            grashofSpinBox->setToolTip(tr("Введите число от 0,001 до 100000"));
        } else {
            prandtlSpinBox->setMinimum(1e-6);
            prandtlSpinBox->setMaximum(1e20);
            grashofSpinBox->setMinimum(1e-6);
            grashofSpinBox->setMaximum(1e20);
            prandtlSpinBox->setToolTip(tr("Введите число от 1e-6 до 1e20"));
            grashofSpinBox->setToolTip(tr("Введите число от 1e-6 до 1e20"));
        }
        prandtlSpinBox->setUseScientificNotation(currentSettings.scientificPrGr);
        grashofSpinBox->setUseScientificNotation(currentSettings.scientificPrGr);
    };

    adaptPrGr(settings);

    connect(&SettingsManager::instance(), &SettingsManager::settingsChanged, this, adaptPrGr);
    connect(ui->aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui->helpAction, SIGNAL(triggered()), this, SLOT(showHelpWindow()));
    connect(ui->aboutAction, SIGNAL(triggered()), aboutWindow, SLOT(show()));
    connect(ui->settingsAction, SIGNAL(triggered()), this, SLOT(showSettingsWindow()));

    connect(&solverSideHeating, SIGNAL(solutionFinished()), this, SLOT(onSolverFinished()));
    connect(&solverBottomHeating, SIGNAL(solutionFinished()), this, SLOT(onSolverFinished()));
    connect(&solverSideHeating, SIGNAL(iterationFinished(int,int,double)), this, SLOT(onIterationFinished(int,int,double)));
    connect(&solverBottomHeating, SIGNAL(iterationFinished(int,int,double)), this, SLOT(onIterationFinished(int,int,double)));
    connect(&solverSideHeating, SIGNAL(maxIterNumberAttained(double)), this, SLOT(onMaxIterNumAttained(double)));
    connect(&solverBottomHeating, SIGNAL(maxIterNumberAttained(double)), this, SLOT(onMaxIterNumAttained(double)));
}

MainWindow::~MainWindow()
{
    delete ui;
    delete aboutWindow;
}

void MainWindow::clearRootResultFiles()
{
    const QDir resultDir(qApp->applicationDirPath() + "/result");
    const QFileInfoList fileInfoList = resultDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &fileInfo : fileInfoList) {
        QFile::remove(fileInfo.absoluteFilePath());
    }
}

void MainWindow::setControlsEnabled(bool enabled)
{
    ui->startButton->setEnabled(enabled);
    ui->settingsAction->setEnabled(enabled);
}

void MainWindow::savePaintAimImage(PaintAim aim, const QString &filename)
{
    glPainter->setPaintAim(aim);
    glPainter->repaint();
    qApp->processEvents();

    const qreal dpr = glPainter->devicePixelRatioF();
    QImage image(qRound(glPainter->width() * dpr), qRound(glPainter->height() * dpr), QImage::Format_ARGB32);
    image.setDevicePixelRatio(dpr);
    image.fill(Qt::white);

    QPainter painter(&image);
    glPainter->render(&painter);
    painter.end();

    image.save(filename, "PNG");
}

void MainWindow::saveAllResultImages()
{
    const QString resultDir = qApp->applicationDirPath() + "/result/";
    savePaintAimImage(T, resultDir + "T.png");
    savePaintAimImage(Psi, resultDir + "Psi.png");
    savePaintAimImage(Omega, resultDir + "Omega.png");
    savePaintAimImage(Vx, resultDir + "Vx.png");
    savePaintAimImage(Vy, resultDir + "Vy.png");
}

QString MainWindow::batchSpecFilePath() const
{
    return qApp->applicationDirPath() + "/batch_values.txt";
}

QList<double> MainWindow::parseNumberList(const QString &text, bool *ok) const
{
    QList<double> values;
    bool localOk = true;
    for (const QString &token : splitListValues(text)) {
        bool tokenOk = false;
        QString normalizedToken = token;
        normalizedToken.replace(',', '.');
        const double value = normalizedToken.toDouble(&tokenOk);
        if (!tokenOk) {
            localOk = false;
            break;
        }
        values.append(value);
    }

    if (ok != nullptr) {
        *ok = localOk && !values.isEmpty();
    }
    return values;
}

QList<MainWindow::BatchRunSpec> MainWindow::buildBatchQueue(QString *errorMessage) const
{
    QList<BatchRunSpec> result;

    QStringList heightValues;
    heightValues.append(ui->heightComboBox->currentText());

    QList<double> prandtlValues{prandtlSpinBox->value()};
    QList<double> grashofValues{grashofSpinBox->value()};

    QFile batchFile(batchSpecFilePath());
    if (batchFile.exists()) {
        if (!batchFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            if (errorMessage != nullptr) {
                *errorMessage = tr("Не удалось открыть файл пакетного запуска:\n%1").arg(batchSpecFilePath());
            }
            return {};
        }

        QTextStream input(&batchFile);
        while (!input.atEnd()) {
            const QString rawLine = input.readLine().trimmed();
            if (rawLine.isEmpty() || rawLine.startsWith('#')) {
                continue;
            }

            const int separatorIndex = rawLine.indexOf('=');
            if (separatorIndex < 0) {
                continue;
            }

            const QString key = rawLine.left(separatorIndex).trimmed().toLower();
            const QString value = rawLine.mid(separatorIndex + 1).trimmed();

            if (key == "height" || key == "h" || key == "ratio") {
                const QStringList parsed = splitListValues(value);
                if (parsed.isEmpty()) {
                    if (errorMessage != nullptr) {
                        *errorMessage = tr("Файл пакетного запуска содержит пустой список высот.");
                    }
                    return {};
                }
                heightValues = parsed;
            } else if (key == "pr" || key == "prandtl") {
                bool ok = false;
                prandtlValues = parseNumberList(value, &ok);
                if (!ok) {
                    if (errorMessage != nullptr) {
                        *errorMessage = tr("Не удалось разобрать список чисел Прандтля в файле %1.").arg(batchSpecFilePath());
                    }
                    return {};
                }
            } else if (key == "gr" || key == "grashof") {
                bool ok = false;
                grashofValues = parseNumberList(value, &ok);
                if (!ok) {
                    if (errorMessage != nullptr) {
                        *errorMessage = tr("Не удалось разобрать список чисел Грасгофа в файле %1.").arg(batchSpecFilePath());
                    }
                    return {};
                }
            }
        }
    }

    for (const QString &heightTextRaw : heightValues) {
        const QString heightText = heightTextRaw.trimmed();
        if (ui->heightComboBox->findText(heightText) < 0) {
            if (errorMessage != nullptr) {
                *errorMessage = tr("Значение высоты/отношения сторон \"%1\" отсутствует в списке программы.").arg(heightText);
            }
            return {};
        }
        for (double prValue : prandtlValues) {
            for (double grValue : grashofValues) {
                BatchRunSpec spec;
                spec.heightText = heightText;
                spec.prandtl = prValue;
                spec.grashof = grValue;
                result.append(spec);
            }
        }
    }

    return result;
}

void MainWindow::launchCurrentRun()
{
    if (ui->tBCComboBox->currentIndex() == 0) {
        solverSideHeating.setProblemParameters(
            ui->heightComboBox->currentText().replace(',', '.').toDouble(),
            prandtlSpinBox->value(),
            grashofSpinBox->value(),
            ui->leftWallCheckBox->isChecked(),
            ui->rightWallCheckBox->isChecked(),
            ui->topWallCheckBox->isChecked(),
            ui->bottomWallCheckBox->isChecked());

        solverSideHeating.setSolverParameters(
            ui->nxSpinBox->value(),
            ui->nySpinBox->value(),
            ui->maxIterSpinBox->value(),
            ui->wTDoubleSpinBox->value(),
            ui->wPsiDoubleSpinBox->value(),
            ui->wOmegaDoubleSpinBox->value());

        solverSideHeating.start();
    } else {
        solverBottomHeating.setProblemParameters(
            1.0 / ui->heightComboBox->currentText().replace(',', '.').toDouble(),
            prandtlSpinBox->value(),
            grashofSpinBox->value(),
            ui->leftWallCheckBox->isChecked(),
            ui->rightWallCheckBox->isChecked(),
            ui->topWallCheckBox->isChecked(),
            ui->bottomWallCheckBox->isChecked());

        solverBottomHeating.setSolverParameters(
            ui->nxSpinBox->value(),
            ui->nySpinBox->value(),
            ui->maxIterSpinBox->value(),
            ui->wTDoubleSpinBox->value(),
            ui->wPsiDoubleSpinBox->value(),
            ui->wOmegaDoubleSpinBox->value());

        solverBottomHeating.start();
    }
}

void MainWindow::startNextBatchRun()
{
    if (currentBatchIndex < 0 || currentBatchIndex >= batchQueue.size()) {
        return;
    }

    const BatchRunSpec &spec = batchQueue[currentBatchIndex];

    {
        const QSignalBlocker blocker(ui->heightComboBox);
        ui->heightComboBox->setCurrentIndex(ui->heightComboBox->findText(spec.heightText));
    }
    on_heightComboBox_currentIndexChanged(ui->heightComboBox->currentIndex());

    prandtlSpinBox->setValue(spec.prandtl);
    grashofSpinBox->setValue(spec.grashof);

    ui->solutionProgressBar->setValue(0);
    ui->solutionProgressLabel->setText(
        tr("Прогресс решения: запуск %1 из %2 ; H/L=%3 ; Pr=%4 ; Gr=%5")
            .arg(currentBatchIndex + 1)
            .arg(batchQueue.size())
            .arg(spec.heightText)
            .arg(spec.prandtl)
            .arg(spec.grashof));

    launchCurrentRun();
}

void MainWindow::on_startButton_clicked()
{
    QString errorMessage;
    batchQueue = buildBatchQueue(&errorMessage);
    if (batchQueue.isEmpty()) {
        if (!errorMessage.isEmpty()) {
            QMessageBox::critical(this, tr("Ошибка пакетного запуска"), errorMessage);
        }
        return;
    }

    clearRootResultFiles();
    setControlsEnabled(false);

    batchModeActive = batchQueue.size() > 1;
    currentBatchIndex = 0;
    startNextBatchRun();
}

QString MainWindow::sanitizeFolderComponent(const QString &value) const
{
    QString sanitized = value;
    sanitized.replace(',', '.');
    sanitized.replace(' ', '_');
    sanitized.replace(';', '_');
    sanitized.replace(':', '_');
    sanitized.replace('/', '_');
    sanitized.replace('\\', '_');
    sanitized.replace('[', '(');
    sanitized.replace(']', ')');
    return sanitized;
}

QString MainWindow::uniqueFolderNameForCurrentRun() const
{
    QString folderName;
    if (batchModeActive) {
        folderName += QString("%1_").arg(currentBatchIndex + 1, 3, 10, QLatin1Char('0'));
    }

    folderName += (ui->tBCComboBox->currentIndex() == 0) ? "s" : "b";
    folderName += sanitizeFolderComponent(ui->heightComboBox->currentText());
    folderName += "_[Pr" + sanitizeFolderComponent(QString::number(prandtlSpinBox->value(), 'g', 16));
    folderName += ";Gr" + sanitizeFolderComponent(QString::number(grashofSpinBox->value(), 'g', 16)) + "]";
    folderName += "[t" + QString::number(ui->topWallCheckBox->isChecked());
    folderName += ";l" + QString::number(ui->leftWallCheckBox->isChecked());
    folderName += ";r" + QString::number(ui->rightWallCheckBox->isChecked());
    folderName += ";b" + QString::number(ui->bottomWallCheckBox->isChecked()) + "]";
    folderName += "[" + QString::number(ui->nxSpinBox->value()) + "x" + QString::number(ui->nySpinBox->value()) + "]";
    folderName += "[" + sanitizeFolderComponent(QString::number(ui->wTDoubleSpinBox->value(), 'g', 16));
    folderName += ";" + sanitizeFolderComponent(QString::number(ui->wPsiDoubleSpinBox->value(), 'g', 16));
    folderName += ";" + sanitizeFolderComponent(QString::number(ui->wOmegaDoubleSpinBox->value(), 'g', 16)) + "]_";
    folderName += QString::number(ui->maxIterSpinBox->value());

    return folderName;
}

void MainWindow::copyResultFilesToUniqueFolder()
{
    const auto settings = SettingsManager::instance().settings();
    if (!settings.useUniqueFolders && !batchModeActive) {
        return;
    }

    const QString uniqueFolderName = uniqueFolderNameForCurrentRun();
    const QString resultRootPath = qApp->applicationDirPath() + "/result";
    const QString targetPath = resultRootPath + "/" + uniqueFolderName;

    QDir resultRoot(resultRootPath);
    if (!resultRoot.exists(uniqueFolderName)) {
        resultRoot.mkdir(uniqueFolderName);
    }

    QDir targetDir(targetPath);
    const QFileInfoList existingFiles = targetDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &fileInfo : existingFiles) {
        QFile::remove(fileInfo.absoluteFilePath());
    }

    const QFileInfoList rootFiles = resultRoot.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &fileInfo : rootFiles) {
        QFile::copy(fileInfo.absoluteFilePath(), targetPath + "/" + fileInfo.fileName());
    }
}

void MainWindow::onSolverFinished()
{
    glPainter->processData();
    glPainter->setPaintAim(T);
    if (QFile::exists(batchSpecFilePath())) {
        saveAllResultImages();
        glPainter->setPaintAim(T);
    }
    copyResultFilesToUniqueFolder();

    if (batchModeActive && currentBatchIndex + 1 < batchQueue.size()) {
        ++currentBatchIndex;
        clearRootResultFiles();
        startNextBatchRun();
        return;
    }

    const bool completedBatch = batchModeActive;
    batchModeActive = false;
    batchQueue.clear();
    currentBatchIndex = -1;

    QMessageBox::information(
        this,
        tr("Решение завершено"),
        completedBatch ? tr("Пакетный запуск завершён") : tr("Решение завершено"));

    setControlsEnabled(true);
    ui->paintTButton->setChecked(true);
}

void MainWindow::on_tBCComboBox_currentIndexChanged(int index)
{
    switch (index) {
    case 0:
        ui->wTDoubleSpinBox->setValue(solverSideHeating.getDefaultWT());
        ui->wPsiDoubleSpinBox->setValue(solverSideHeating.getDefaultWPsi());
        ui->wOmegaDoubleSpinBox->setValue(solverSideHeating.getDefaultWOmega());
        ui->maxIterSpinBox->setValue(solverSideHeating.getDefaultMaxNumOfIter());
        break;
    case 1:
        ui->wTDoubleSpinBox->setValue(solverBottomHeating.getDefaultWT());
        ui->wPsiDoubleSpinBox->setValue(solverBottomHeating.getDefaultWPsi());
        ui->wOmegaDoubleSpinBox->setValue(solverBottomHeating.getDefaultWOmega());
        ui->maxIterSpinBox->setValue(solverBottomHeating.getDefaultMaxNumOfIter());
        break;
    default:
        ui->wTDoubleSpinBox->setValue(0.3);
        ui->wPsiDoubleSpinBox->setValue(0.3);
        ui->wOmegaDoubleSpinBox->setValue(0.3);
        ui->maxIterSpinBox->setValue(1000);
        break;
    }
}

void MainWindow::on_heightComboBox_currentIndexChanged(int index)
{
    switch (index) {
    case 0:
        ui->nxSpinBox->setValue(100);
        ui->nySpinBox->setValue(25);
        break;
    case 1:
        ui->nxSpinBox->setValue(100);
        ui->nySpinBox->setValue(50);
        break;
    case 2:
        ui->nxSpinBox->setValue(100);
        ui->nySpinBox->setValue(100);
        break;
    case 3:
        ui->nxSpinBox->setValue(50);
        ui->nySpinBox->setValue(100);
        break;
    case 4:
        ui->nxSpinBox->setValue(25);
        ui->nySpinBox->setValue(100);
        break;
    default:
        ui->nxSpinBox->setValue(100);
        ui->nySpinBox->setValue(100);
        break;
    }
}

void MainWindow::on_paintTButton_clicked()
{
    glPainter->setPaintAim(T);
}

void MainWindow::on_paintPsiButton_clicked()
{
    glPainter->setPaintAim(Psi);
}

void MainWindow::on_paintOmegaButton_clicked()
{
    glPainter->setPaintAim(Omega);
}

void MainWindow::on_paintVxButton_clicked()
{
    glPainter->setPaintAim(Vx);
}

void MainWindow::on_paintVyButton_clicked()
{
    glPainter->setPaintAim(Vy);
}

void MainWindow::onIterationFinished(int currentIteration, int maxNumOfIterations, double residual)
{
    QString prefix;
    if (batchModeActive && currentBatchIndex >= 0 && currentBatchIndex < batchQueue.size()) {
        prefix = tr("Запуск %1/%2 ; ").arg(currentBatchIndex + 1).arg(batchQueue.size());
    }

    ui->solutionProgressLabel->setText(
        tr("Прогресс решения: ") + prefix +
        tr("Итерация ") + QString::number(currentIteration) +
        tr(" из ") + QString::number(maxNumOfIterations) +
        tr(" ; Текущая невязка ") + QString::number(residual));

    ui->solutionProgressBar->setValue(
        static_cast<int>(static_cast<double>(currentIteration) * 100.0 / static_cast<double>(maxNumOfIterations)));
}

void MainWindow::onMaxIterNumAttained(double residual)
{
    QMessageBox::warning(
        this,
        tr("Решение не найдено"),
        tr("Решатель не смог найти решение за отведенное число итераций.\n") +
        tr("Текущая невязка: ") + QString::number(residual) +
        tr(".\nРешение может быть неточным или вообще неправильным"));
}

void MainWindow::on_stopButton_clicked()
{
    batchQueue.clear();
    batchModeActive = false;
    currentBatchIndex = -1;

    if (solverBottomHeating.isRunning()) {
        solverBottomHeating.cutOffSolution();
    }
    if (solverSideHeating.isRunning()) {
        solverSideHeating.cutOffSolution();
    }

    setControlsEnabled(true);
}

void MainWindow::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::showSettingsWindow()
{
    settingsWindow->show();
}

void MainWindow::showHelpWindow()
{
    helpWindow->show();
}

void MainWindow::on_saveImageButton_clicked()
{
    const auto settings = SettingsManager::instance().settings();
    if (settings.paintEngine == PaintEngine::OpenGL) {
        QString filename = qApp->applicationDirPath() + "/result/";

        if (ui->paintTButton->isChecked()) {
            filename += "T.png";
            savePaintAimImage(T, filename);
        } else if (ui->paintPsiButton->isChecked()) {
            filename += "Psi.png";
            savePaintAimImage(Psi, filename);
        } else if (ui->paintOmegaButton->isChecked()) {
            filename += "Omega.png";
            savePaintAimImage(Omega, filename);
        } else if (ui->paintVxButton->isChecked()) {
            filename += "Vx.png";
            savePaintAimImage(Vx, filename);
        } else if (ui->paintVyButton->isChecked()) {
            filename += "Vy.png";
            savePaintAimImage(Vy, filename);
        } else {
            filename += "unknown.png";
            savePaintAimImage(NotDefine, filename);
        }
    }
}
