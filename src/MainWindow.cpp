#include "MainWindow.h"

#include "AudioOutputDevice.h"
#include "NesPalette.h"
#include "NesNametableItem.h"
#include "NesScene.h"
#include "NesTileItem.h"
#include "NesView.h"
#include "Nes/Mapper/Mapper001.h"
#include "PaletteEditorDialog.h"
#include "PatternTableDialog.h"

#include <QAction>
#include <QAudioSink>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QMediaDevices>
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>

namespace {
constexpr quint32 AudioPrebufferSamples = 22050;

NesPalette loadStoredPalette() {
    QSettings settings;
    QVector<QColor> colors;
    colors.reserve(NesPalette::ColorCount);

    if (!settings.contains("Palette/Color00")) {
        return NesPalette();
    }

    for (int index = 0; index < NesPalette::ColorCount; ++index) {
        const QString key = QString("Palette/Color%1").arg(index, 2, 10, QLatin1Char('0'));
        const QColor color = QColor::fromRgba(settings.value(key).toUInt());
        if (!color.isValid()) {
            return NesPalette();
        }
        colors.append(color);
    }

    return NesPalette(colors);
}
}

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent) {
    this->resize(800, 600);
    this->setWindowTitle("NesV3");

    this->view = new NesView(this);
    auto *view = this->view;
    connect(view, &NesView::buttonChanged, this, [this](int button, bool pressed) {
        if (button < 0 || button > 7) {
            return;
        }
        this->nes.controller().setButton(
            static_cast<Controller::Button>(button), pressed);
        qInfo().noquote() << "Controller button update: button=" << button
                          << "pressed=" << pressed;
    });
    this->scene = new NesScene(view);
    this->scene->setPalette(loadStoredPalette());
    this->nes.cartridge().setMappingChangedCallback(
        [this]() {
            this->nes.ppu().invalidateChrTileCache();
            this->scene->invalidateTileCache();
        });
    view->setScene(this->scene);
    view->centerOn(this->scene->nesViewportRect().center());
    this->setCentralWidget(view);
    this->statusBar()->showMessage(tr("Performance: waiting for ROM"));
    this->initializeAudioOutput();
    this->performanceTimer.start();

		this->simulationTimer.setSingleShot(true);
        this->simulationTimer.setTimerType(Qt::PreciseTimer);
        connect(
            &this->simulationTimer,
            &QTimer::timeout,
            this,
            &MainWindow::runSimulationFrame);
    connect(
        &this->automatedRunStopTimer,
        &QTimer::timeout,
        this,
        &MainWindow::finishAutomatedRun);
    connect(
        &this->screenshotTimer,
        &QTimer::timeout,
        this,
        &MainWindow::saveViewportScreenshot);

    auto *fileMenu = menuBar()->addMenu(tr("File"));
    auto *openRomAction = fileMenu->addAction(tr("Open ROM"));
    auto *closeRomAction = fileMenu->addAction(tr("Close ROM"));
    fileMenu->addSeparator();
    auto *quitAction = fileMenu->addAction(tr("Exit"));
    connect(openRomAction, &QAction::triggered, this, &MainWindow::openRom);
    connect(closeRomAction, &QAction::triggered, this, &MainWindow::closeRom);
    connect(quitAction, &QAction::triggered, this, &QWidget::close);

    auto *paletteMenu = menuBar()->addMenu(tr("Palette"));
    auto *editPaletteAction = paletteMenu->addAction(tr("Palette Settings"));
    connect(editPaletteAction, &QAction::triggered, this, &MainWindow::openPaletteEditor);

        auto *debugMenu = menuBar()->addMenu(tr("Debug"));
        auto *debugScrollAction = debugMenu->addAction(tr("Debug Nametable Input"));
        debugScrollAction->setCheckable(true);
        debugScrollAction->setChecked(false);
        connect(debugScrollAction, &QAction::toggled,
            this, &MainWindow::setDebugInputMode);
        debugMenu->addSeparator();
        auto *patternTableAction = debugMenu->addAction(tr("CHR Pattern Tables"));
        connect(patternTableAction, &QAction::triggered,
            this, &MainWindow::openPatternTableDialog);
        updateWindowTitle();
}

void MainWindow::setDebugInputMode(bool enabled) {
    if (this->view == nullptr) {
        return;
    }

    this->view->setInputMode(
        enabled ? NesView::InputMode::Debug : NesView::InputMode::Game);
    this->statusBar()->showMessage(
        enabled ? tr("Input mode: Debug nametable")
                : tr("Input mode: Game controller"));
}

MainWindow::~MainWindow() {
    this->simulationTimer.stop();
    this->automatedRunStopTimer.stop();
    this->screenshotTimer.stop();
    this->stopAudioOutput();
    closePerformanceLog();
}

void MainWindow::initializeAudioOutput() {
    this->audioOutputDevice = new AudioOutputDevice(
        this->audioRingBuffer, this);
    const QAudioDevice outputDevice = QMediaDevices::defaultAudioOutput();
    if (outputDevice.isNull()) {
        qWarning() << "No default audio output device is available";
        this->statusBar()->showMessage(tr("Audio: no output device"));
        return;
    }

    QAudioFormat format;
    format.setSampleRate(44100);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);
    if (!outputDevice.isFormatSupported(format)) {
        qWarning() << "Default audio output does not support 44100 Hz mono PCM16";
        this->statusBar()->showMessage(tr("Audio: format not supported"));
        return;
    }

    this->audioSink = new QAudioSink(outputDevice, format, this);
    connect(
        this->audioSink,
        &QAudioSink::stateChanged,
        this,
        [this](QAudio::State state) {
            if (state == QAudio::StoppedState
                && this->audioSink->error() != QAudio::NoError) {
                qWarning() << "Audio output stopped with error:"
                           << this->audioSink->error();
            }
        });
    this->statusBar()->showMessage(tr("Audio: ready"));
}

void MainWindow::startAudioOutput() {
    if (this->audioSink == nullptr || this->audioOutputDevice == nullptr) {
        return;
    }

    this->audioRingBuffer.reset();
    this->audioOutputDevice->resetReadStatistics();
    this->nes.resetAudioSampling();
    this->nes.setAudioSampleCallback(
        [this](float sample) {
            (void)this->audioRingBuffer.push(sample);
        });
    if (this->audioOutputDevice->isOpen()) {
        this->audioOutputDevice->close();
    }
    if (!this->audioOutputDevice->open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open audio output device"
                   << this->audioOutputDevice->errorString();
        this->nes.setAudioSampleCallback(AudioSampleScheduler::SampleCallback());
        return;
    }
    this->audioOutputStarted = false;
}

void MainWindow::startAudioSinkIfReady() {
    if (this->audioOutputStarted || this->audioSink == nullptr
        || this->audioOutputDevice == nullptr) {
        return;
    }
    if (this->audioRingBuffer.availableToRead() < AudioPrebufferSamples) {
        return;
    }

    this->audioSink->start(this->audioOutputDevice);
    this->audioOutputStarted = true;
}

void MainWindow::stopAudioOutput() {
    this->nes.setAudioSampleCallback(AudioSampleScheduler::SampleCallback());
    if (this->audioSink != nullptr) {
        this->audioSink->stop();
    }
    this->audioOutputStarted = false;
    if (this->audioOutputDevice != nullptr
        && this->audioOutputDevice->isOpen()) {
        this->audioOutputDevice->close();
    }
    this->audioRingBuffer.reset();
    this->nes.resetAudioSampling();
}

bool MainWindow::loadRomFromPath(const QString &path, QString &error) {
    if (path.isEmpty()) {
        error = QStringLiteral("ROM path is empty");
        return false;
    }

    this->simulationTimer.stop();
    this->stopAudioOutput();
    closePerformanceLog();
    if (!this->nes.cartridge().loadFromFile(path, error)) {
        return false;
    }

    qInfo() << QString("Loaded ROM: %1, mapper %2")
                   .arg(QFileInfo(path).fileName())
                   .arg(this->nes.cartridge().header().mapperNumber);
    this->nes.cartridge().connect(
            this->nes.cpu().bus(), this->nes.ppu().bus());
    this->nes.ppu().setNametableMirroring(
            this->nes.cartridge().header().fourScreenMirroring
                ? Ppu::NametableMirroring::FourScreen
                : (this->nes.cartridge().header().verticalMirroring
                    ? Ppu::NametableMirroring::Vertical
                    : Ppu::NametableMirroring::Horizontal));
    this->nes.cpu().reset();
    this->nes.ppu().reset();
    this->nes.apu().reset();
    this->nes.clock().reset();
    this->nes.resetAudioSampling();
    this->scene->invalidateTileCache();
    this->scene->updateFromPpu(this->nes.ppu());
    this->currentRomPath = path;
    this->simulationPacingTimer.restart();
    this->simulationFrameNumber = 0;
    openPerformanceLog(path);
    resetPerformanceWindow();
    this->startAudioOutput();
    this->scheduleSimulationWakeup();
    updateWindowTitle();
    this->view->setFocus(Qt::OtherFocusReason);
    qInfo() << "NesView focus restored after ROM load:" << this->view->hasFocus();
    return true;
}

bool MainWindow::startAutomatedRun(
        int durationMilliseconds,
        const QString &screenshotDirectory,
        int screenshotIntervalMilliseconds,
        bool screenshotOnExit,
        QString &error) {
    if (durationMilliseconds <= 0) {
        error = QStringLiteral("Run duration must be positive");
        return false;
    }
    if (screenshotDirectory.isEmpty()
            && (screenshotIntervalMilliseconds > 0 || screenshotOnExit)) {
        error = QStringLiteral("Screenshot options require a directory");
        return false;
    }
    if (screenshotIntervalMilliseconds < 0) {
        error = QStringLiteral("Screenshot interval must not be negative");
        return false;
    }
    if (!screenshotDirectory.isEmpty()
            && !QDir().mkpath(screenshotDirectory)) {
        error = QStringLiteral("Unable to create screenshot directory: %1")
                    .arg(screenshotDirectory);
        return false;
    }

    this->screenshotDirectory = screenshotDirectory;
    this->screenshotSequence = 0;
    this->screenshotOnExit = screenshotOnExit;
    this->automatedRunActive = true;
    this->automatedRunTimer.restart();
    this->automatedRunStopTimer.start(durationMilliseconds);
    if (screenshotIntervalMilliseconds > 0) {
        this->screenshotTimer.start(screenshotIntervalMilliseconds);
    }
    qInfo() << "Automated run started: durationMs=" << durationMilliseconds;
    return true;
}

void MainWindow::openPerformanceLog(const QString &romPath) {
    closePerformanceLog();

    const QString logDirectory = QStandardPaths::writableLocation(
        QStandardPaths::AppLocalDataLocation);
    if (!QDir().mkpath(logDirectory)) {
        qWarning() << "Unable to create performance log directory:" << logDirectory;
        return;
    }

    const QString romName = QFileInfo(romPath).completeBaseName();
    const QString timestamp = QDateTime::currentDateTime().toString(
        QStringLiteral("yyyyMMdd-HHmmss"));
    this->performanceLogFile.setFileName(
        QDir(logDirectory).filePath(
            QStringLiteral("performance-%1-%2.log").arg(romName, timestamp)));
    if (!this->performanceLogFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Unable to open performance log:"
                   << this->performanceLogFile.fileName()
                   << this->performanceLogFile.errorString();
        return;
    }

    this->performanceLogStream.setDevice(&this->performanceLogFile);
    this->performanceLogStream
        << "ROM=" << romPath
        << " mapper=" << this->nes.cartridge().header().mapperNumber
        << "\n";
    this->performanceLogStream.flush();
    qInfo() << "Performance log:" << this->performanceLogFile.fileName();
}

void MainWindow::closePerformanceLog() {
    if (!this->performanceLogFile.isOpen()) {
        return;
    }

    this->performanceLogStream.flush();
    this->performanceLogStream.setDevice(nullptr);
    this->performanceLogFile.close();
}

void MainWindow::resetPerformanceWindow() {
    this->performanceTimer.restart();
    this->performanceFrameCount = 0;
    this->performanceCoreNanoseconds = 0;
    this->performanceSceneNanoseconds = 0;
    this->performanceDirtyTiles = 0;
    this->performanceDecodedTiles = 0;
    this->performanceUpdatedTiles = 0;
    this->performanceNametableSetPos = 0;
    NesTileItem::resetPaintStats();
    NesNametableItem::resetPositionStats();
    (void)this->nes.clock().takeTimingStats();
    (void)this->nes.cpu().bus().takeStats();
    (void)this->nes.apu().takeStats();
    (void)this->view->takePaintStats();
}

void MainWindow::savePalette() {
    QSettings settings;
    settings.beginGroup("Palette");
    const QVector<QColor> &colors = this->scene->palette().colors();
    for (int index = 0; index < colors.size(); ++index) {
        settings.setValue(QString("Color%1").arg(index, 2, 10, QLatin1Char('0')), colors[index].rgba());
    }
    settings.endGroup();
}

void MainWindow::openPaletteEditor() {
    PaletteEditorDialog dialog(this);
    dialog.setPalette(this->scene->palette().colors());
    connect(&dialog, &PaletteEditorDialog::paletteChanged, this, [this](const QVector<QColor> &colors) {
        this->scene->setPalette(NesPalette(colors));
        savePalette();
    });
    dialog.exec();
}

void MainWindow::openPatternTableDialog() {
    PatternTableDialog dialog(this->nes.ppu().bus(), this->scene->palette(), this);
    dialog.exec();
}

void MainWindow::openRom() {
    const QString path = QFileDialog::getOpenFileName(
            this,
            tr("Open NES ROM"),
            QString(),
            tr("NES ROMs (*.nes);;All files (*)"));
    if (path.isEmpty()) {
        return;
    }

    QString error;
    if (!this->loadRomFromPath(path, error)) {
        QMessageBox::critical(this, tr("Open ROM failed"), error);
        return;
    }
}

bool MainWindow::saveViewportScreenshot() {
    if (!this->automatedRunActive || this->screenshotDirectory.isEmpty()
            || this->view == nullptr) {
        return false;
    }

    const QString romName = QFileInfo(this->currentRomPath).completeBaseName();
    const QString fileName = QStringLiteral("%1-%2ms-%3.png")
        .arg(romName)
        .arg(this->automatedRunTimer.elapsed(), 8, 10, QLatin1Char('0'))
        .arg(this->screenshotSequence++, 4, 10, QLatin1Char('0'));
    const QString path = QDir(this->screenshotDirectory).filePath(fileName);
    const bool saved = this->view->viewport()->grab().save(path, "PNG");
    if (!saved) {
        qWarning() << "Unable to save screenshot:" << path;
    } else {
        qInfo() << "Screenshot:" << path;
    }
    return saved;
}

void MainWindow::finishAutomatedRun() {
    if (!this->automatedRunActive) {
        return;
    }

    this->screenshotTimer.stop();
    if (this->screenshotOnExit) {
        (void)this->saveViewportScreenshot();
    }
    this->simulationTimer.stop();
    this->stopAudioOutput();
    this->automatedRunStopTimer.stop();
    this->automatedRunActive = false;
    closePerformanceLog();
    qInfo() << "Automated run finished after ms="
            << this->automatedRunTimer.elapsed();
    this->close();
    QCoreApplication::exit(0);
}

void MainWindow::closeRom() {
    this->simulationTimer.stop();
    this->stopAudioOutput();
    closePerformanceLog();
    this->nes.cartridge().unload();
    this->nes.cartridge().connect(
            this->nes.cpu().bus(), this->nes.ppu().bus());
    this->nes.cpu().reset();
    this->nes.ppu().reset();
    this->nes.apu().reset();
    this->nes.clock().reset();
    this->scene->invalidateTileCache();
    this->currentRomPath.clear();
    updateWindowTitle();
}

void MainWindow::runSimulationFrame() {
    if (this->currentRomPath.isEmpty() || !this->nes.cartridge().isLoaded()) {
        return;
    }

    const quint64 nextFrameNumber = this->simulationFrameNumber + 1;
    const qint64 nextFrameDeadline = static_cast<qint64>(
        nextFrameNumber * 1000000000ULL / 60);
    if (this->simulationPacingTimer.nsecsElapsed() < nextFrameDeadline) {
        this->scheduleSimulationWakeup();
        return;
    }

    this->frameTimer.start();
    this->nes.runFrame();
    this->startAudioSinkIfReady();
    const quint64 coreNanoseconds = static_cast<quint64>(this->frameTimer.nsecsElapsed());
    this->simulationFrameNumber = nextFrameNumber;
    this->scheduleSimulationWakeup();

    this->frameTimer.start();
    this->scene->updateFromPpu(this->nes.ppu());
    const quint64 sceneNanoseconds = static_cast<quint64>(this->frameTimer.nsecsElapsed());
    const NesScene::UpdateStats sceneStats = this->scene->takeUpdateStats();

    ++this->performanceFrameCount;
    this->performanceCoreNanoseconds += coreNanoseconds;
    this->performanceSceneNanoseconds += sceneNanoseconds;
    this->performanceDirtyTiles += sceneStats.dirtyTiles;
    this->performanceDecodedTiles += sceneStats.decodedTiles;
    this->performanceUpdatedTiles += sceneStats.updatedTiles;
    this->performanceNametableSetPos += sceneStats.nametableSetPosCalls;

    const qint64 elapsedMilliseconds = this->performanceTimer.elapsed();
    if (elapsedMilliseconds < 1000) {
        return;
    }

    const double seconds = elapsedMilliseconds / 1000.0;
    const double simulationFps = this->performanceFrameCount / seconds;
    const double averageCoreMs = this->performanceCoreNanoseconds
        / static_cast<double>(this->performanceFrameCount) / 1000000.0;
    const double averageSceneMs = this->performanceSceneNanoseconds
        / static_cast<double>(this->performanceFrameCount) / 1000000.0;
    const NesTileItem::PaintStats tileStats = NesTileItem::paintStats();
    const double paintMs = tileStats.paintNanoseconds / 1000000.0;
    const NesClock::TimingStats clockStats = this->nes.clock().takeTimingStats();
    const double clockCpuMs = clockStats.cpuNanoseconds / 1000000.0;
    const double clockPpuMs = clockStats.ppuNanoseconds / 1000000.0;
    const Bus::Stats busStats = this->nes.cpu().bus().takeStats();
    const Apu::Stats apuStats = this->nes.apu().takeStats();
    const NesView::PaintStats viewPaintStats = this->view->takePaintStats();
    const double viewPaintMs = viewPaintStats.nanoseconds / 1000000.0;
    const NesNametableItem::PositionStats positionStats =
        NesNametableItem::positionStats();
    const quint32 audioBufferedSamples = this->audioRingBuffer.availableToRead();
    const quint64 audioUnderrunSamples =
        this->audioRingBuffer.underrunSampleCount();
    const quint64 audioDroppedSamples =
        this->audioRingBuffer.droppedSampleCount();
    const quint64 audioReadCalls = this->audioOutputDevice == nullptr
        ? 0
        : this->audioOutputDevice->readCallCount();
    const qint64 audioMaximumReadBytes = this->audioOutputDevice == nullptr
        ? 0
        : this->audioOutputDevice->maximumReadRequestBytes();
    const QString message = QStringLiteral(
        "fps=%1 core=%2ms scene=%3ms view=%4/%5ms tile=%6/%7ms "
        "rebuild=%8 dirty=%9 decoded=%10 updated=%11 "
        "pos=%12/%13 checks=%14 clock=%15/%16ms samples=%17/%18 "
        "bus=%19/%20 miss=%21 apu=%22 audio=%23/%24/%25/%26/%27")
        .arg(simulationFps, 0, 'f', 1)
        .arg(averageCoreMs, 0, 'f', 3)
        .arg(averageSceneMs, 0, 'f', 3)
        .arg(viewPaintStats.calls)
        .arg(viewPaintMs, 0, 'f', 3)
        .arg(tileStats.paintCalls)
        .arg(paintMs, 0, 'f', 3)
        .arg(tileStats.imageRebuilds)
        .arg(this->performanceDirtyTiles)
        .arg(this->performanceDecodedTiles)
        .arg(this->performanceUpdatedTiles)
        .arg(this->performanceNametableSetPos)
        .arg(positionStats.tileSetPosCalls)
        .arg(positionStats.rasterPositionChecks)
        .arg(clockCpuMs, 0, 'f', 3)
        .arg(clockPpuMs, 0, 'f', 3)
        .arg(clockStats.cpuSamples)
        .arg(clockStats.ppuSamples)
        .arg(busStats.reads)
        .arg(busStats.writes)
        .arg(busStats.readCacheMisses + busStats.writeCacheMisses)
        .arg(apuStats.clockCalls)
        .arg(audioBufferedSamples)
        .arg(audioUnderrunSamples)
        .arg(audioDroppedSamples)
        .arg(audioReadCalls)
        .arg(audioMaximumReadBytes);
    this->statusBar()->showMessage(message);
    qInfo().noquote() << message;
    if (this->performanceLogFile.isOpen()) {
        this->performanceLogStream << message << '\n';
        this->performanceLogStream.flush();
    }
    this->resetPerformanceWindow();
}

void MainWindow::scheduleSimulationWakeup() {
    if (this->currentRomPath.isEmpty() || !this->nes.cartridge().isLoaded()) {
        return;
    }

    const quint64 nextFrameNumber = this->simulationFrameNumber + 1;
    const qint64 nextFrameDeadline = static_cast<qint64>(
        nextFrameNumber * 1000000000ULL / 60);
    const qint64 remainingNanoseconds = nextFrameDeadline
        - this->simulationPacingTimer.nsecsElapsed();
    const int delayMilliseconds = remainingNanoseconds <= 0
        ? 1
        : static_cast<int>((remainingNanoseconds + 999999) / 1000000);
    this->simulationTimer.start(delayMilliseconds);
}

void MainWindow::updateWindowTitle() {
    if (this->currentRomPath.isEmpty()) {
        this->setWindowTitle(tr("NesV3"));
        return;
    }
    this->setWindowTitle(
            QStringLiteral("NesV3 - %1 (Mapper %2)")
                .arg(QFileInfo(this->currentRomPath).fileName())
                .arg(this->nes.cartridge().header().mapperNumber));
}

