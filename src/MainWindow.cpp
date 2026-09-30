#include "MainWindow.h"

#include "NesPalette.h"
#include "NesNametableItem.h"
#include "NesScene.h"
#include "NesTileItem.h"
#include "NesView.h"
#include "Nes/Mapper/Mapper001.h"
#include "PaletteEditorDialog.h"
#include "PatternTableDialog.h"

#include <QAction>
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
#include <QSettings>
#include <QStandardPaths>
#include <QStatusBar>

namespace {
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
    this->performanceTimer.start();

        this->simulationTimer.setInterval(16);
        this->simulationTimer.setTimerType(Qt::PreciseTimer);
        connect(
            &this->simulationTimer,
            &QTimer::timeout,
            this,
            &MainWindow::runSimulationFrame);

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
    closePerformanceLog();
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
    if (!loadRomFromPath(path, &error)) {
        QMessageBox::critical(this, tr("Open ROM failed"), error);
        return;
    }
}

bool MainWindow::loadRomFromPath(const QString &path, QString *errorMessage) {
    if (path.isEmpty()) {
        return false;
    }

    if (!this->currentRomPath.isEmpty()) {
        closeRom();
    }

    QString error;
    if (!this->nes.cartridge().loadFromFile(path, error)) {
        if (errorMessage != nullptr) {
            *errorMessage = error;
        }
        qWarning() << "Unable to load ROM:" << path << error;
        return false;
    }

    qInfo() << QString("Loaded ROM: %1, mapper %2").arg(QFileInfo(path).fileName()).arg(this->nes.cartridge().header().mapperNumber);

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
    this->nes.clock().reset();
    this->scene->invalidateTileCache();
    this->scene->updateFromPpu(this->nes.ppu());
    this->currentRomPath = path;
    openPerformanceLog(path);
    this->simulationTimer.start();
    updateWindowTitle();
    if (auto *view = qobject_cast<NesView *>(this->centralWidget())) {
        view->setFocus(Qt::OtherFocusReason);
        qInfo() << "NesView focus restored after ROM load:" << view->hasFocus();
    }
    return true;
}

bool MainWindow::startBenchmark(
    const QStringList &romPaths, int perRomMilliseconds) {
    if (romPaths.size() != 2 || perRomMilliseconds <= 0) {
        return false;
    }

    this->benchmarkRomPaths = romPaths;
    this->benchmarkRomIndex = 0;
    this->benchmarkPerRomMilliseconds = perRomMilliseconds;
    QTimer::singleShot(0, this, &MainWindow::runNextBenchmarkRom);
    return true;
}

void MainWindow::runNextBenchmarkRom() {
    if (this->benchmarkRomIndex >= this->benchmarkRomPaths.size()) {
        closeRom();
        qInfo() << "Benchmark completed";
        QCoreApplication::exit(0);
        return;
    }

    const QString path = this->benchmarkRomPaths.at(this->benchmarkRomIndex);
    if (!loadRomFromPath(path)) {
        closeRom();
        qWarning() << "Benchmark failed while loading ROM:" << path;
        QCoreApplication::exit(2);
        return;
    }

    ++this->benchmarkRomIndex;
    this->benchmarkRomTimer.restart();
    qInfo() << "Benchmark started:" << path
            << "durationMs=" << this->benchmarkPerRomMilliseconds;
    QTimer::singleShot(this->benchmarkPerRomMilliseconds, this, [this]() {
        const QString finishedPath = this->currentRomPath;
        const qint64 elapsedMilliseconds = this->benchmarkRomTimer.elapsed();
        closeRom();
        qInfo() << "Benchmark finished:" << finishedPath
                << "elapsedMs=" << elapsedMilliseconds;
        QTimer::singleShot(0, this, &MainWindow::runNextBenchmarkRom);
    });
}

void MainWindow::closeRom() {
    this->simulationTimer.stop();
    closePerformanceLog();
    this->nes.cartridge().unload();
    this->nes.cartridge().connect(
            this->nes.cpu().bus(), this->nes.ppu().bus());
    this->nes.cpu().reset();
    this->nes.ppu().reset();
    this->nes.clock().reset();
    this->scene->invalidateTileCache();
    this->currentRomPath.clear();
    updateWindowTitle();
}

void MainWindow::runSimulationFrame() {
    if (this->currentRomPath.isEmpty() || !this->nes.cartridge().isLoaded()) {
        return;
    }

    QElapsedTimer coreTimer;
    coreTimer.start();
    this->nes.runFrame();
    const quint64 coreNanoseconds = static_cast<quint64>(coreTimer.nsecsElapsed());

    QElapsedTimer sceneTimer;
    sceneTimer.start();
    this->scene->updateFromPpu(this->nes.ppu());
    const quint64 sceneNanoseconds = static_cast<quint64>(sceneTimer.nsecsElapsed());
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
    const Ppu::WriteStats ppuWriteStats = this->nes.ppu().takeWriteStats();
    const Controller::ReadStats controllerStats =
        this->nes.controller().takeReadStats();
    const Cpu::ExecutionStats cpuStats = this->nes.cpu().takeExecutionStats();
    Cpu::CpuReg cpuRegisters;
    this->nes.cpu().getContent(cpuRegisters);
    const double simulationFps = this->performanceFrameCount / seconds;
    const double averageCoreMs = this->performanceCoreNanoseconds
        / static_cast<double>(this->performanceFrameCount) / 1000000.0;
    const double averageSceneMs = this->performanceSceneNanoseconds
        / static_cast<double>(this->performanceFrameCount) / 1000000.0;
    const NesTileItem::PaintStats tileStats = NesTileItem::paintStats();
    const double paintMs = tileStats.paintNanoseconds / 1000000.0;
    const double imageRebuildMs = tileStats.imageRebuildNanoseconds / 1000000.0;
    const NesClock::TimingStats clockStats = this->nes.clock().takeTimingStats();
    const double clockCpuMs = clockStats.cpuNanoseconds / 1000000.0;
    const double clockPpuMs = clockStats.ppuNanoseconds / 1000000.0;
    const Bus::Stats busStats = this->nes.cpu().bus().takeStats();
    const Apu::Stats apuStats = this->nes.apu().takeStats();
    const NesView::PaintStats viewPaintStats = this->view->takePaintStats();
    const double viewPaintMs = viewPaintStats.nanoseconds / 1000000.0;
    const NesNametableItem::PositionStats positionStats =
        NesNametableItem::positionStats();
    const QString message = QStringLiteral(
        "FPS %1 | core %2 ms clockCpu %3 ms clockPpu %4 ms samples %5/%6 | scene %7 ms | dirty %8 decoded %9 updated %10 | parentSetPos %11 tileSetPos %12 zeroDelta %13 smallDelta %14 checks %15 | paint %16 calls %17 ms rebuild %18 calls %19 ms | PC %20:%21 now %22 SP %23 int %24 NMI %25 RTI %26 | maskWrites %27 lastMask $%28 DMA %29 spriteOut %30 spriteFail %31 | PPU $2000 %32 $2001 %33 $2002 %34 | buttons $%35 seen $%36")
        .arg(simulationFps, 0, 'f', 1)
        .arg(averageCoreMs, 0, 'f', 3)
        .arg(clockCpuMs, 0, 'f', 3)
        .arg(clockPpuMs, 0, 'f', 3)
        .arg(clockStats.cpuSamples)
        .arg(clockStats.ppuSamples)
        .arg(averageSceneMs, 0, 'f', 3)
        .arg(this->performanceDirtyTiles)
        .arg(this->performanceDecodedTiles)
        .arg(this->performanceUpdatedTiles)
        .arg(this->performanceNametableSetPos)
        .arg(positionStats.tileSetPosCalls)
        .arg(positionStats.zeroDeltaPositionChecks)
        .arg(positionStats.smallDeltaPositionChecks)
        .arg(positionStats.rasterPositionChecks)
        .arg(tileStats.paintCalls)
        .arg(paintMs, 0, 'f', 3)
        .arg(tileStats.imageRebuilds)
        .arg(imageRebuildMs, 0, 'f', 3)
        .arg(QString::number(cpuStats.lastPc, 16).rightJustified(4, QLatin1Char('0')))
        .arg(QString::number(cpuStats.lastOpcode, 16).rightJustified(2, QLatin1Char('0')))
        .arg(QString::number(cpuRegisters.pc, 16).rightJustified(4, QLatin1Char('0')))
        .arg(QString::number(cpuRegisters.s, 16).rightJustified(2, QLatin1Char('0')))
        .arg(static_cast<int>(cpuRegisters.intPending))
        .arg(cpuStats.nmiEntries)
        .arg(cpuStats.rtiEntries)
        .arg(ppuWriteStats.maskWrites)
        .arg(ppuWriteStats.lastMaskValue, 2, 16, QLatin1Char('0'))
        .arg(ppuWriteStats.oamDmaTransfers)
        .arg(ppuWriteStats.spriteOutputs)
        .arg(ppuWriteStats.spriteRenderFailures)
        .arg(this->nes.ppu().controlRegister(), 2, 16, QLatin1Char('0'))
        .arg(this->nes.ppu().maskRegister(), 2, 16, QLatin1Char('0'))
        .arg(this->nes.ppu().statusRegister(), 2, 16, QLatin1Char('0'))
        .arg(QString::number(controllerStats.currentButtons, 16)
             .rightJustified(2, QLatin1Char('0')))
        .arg(QString::number(controllerStats.lastNonZeroLatchedButtons, 16)
             .rightJustified(2, QLatin1Char('0')));
    this->statusBar()->showMessage(message);
    qInfo().noquote() << message;
    if (this->performanceLogFile.isOpen()) {
        this->performanceLogStream << message << '\n';
        this->performanceLogStream.flush();
    }
    const QString diagnostics = QStringLiteral(
        "Diagnostics clockCpuMs %1 clockPpuMs %2 clockSamples %3/%4 "
        "busReadWrite %5/%6 busCacheHit %7/%8 busCacheMiss %9/%10 "
        "busScanEntries %11 apuClock %12 apuReadWrite %13/%14 apuIrqChanges %15 "
        "viewPaint %16 calls %17 ms")
        .arg(clockCpuMs, 0, 'f', 3)
        .arg(clockPpuMs, 0, 'f', 3)
        .arg(clockStats.cpuSamples)
        .arg(clockStats.ppuSamples)
        .arg(busStats.reads)
        .arg(busStats.writes)
        .arg(busStats.readCacheHits)
        .arg(busStats.writeCacheHits)
        .arg(busStats.readCacheMisses)
        .arg(busStats.writeCacheMisses)
        .arg(busStats.mappingScanEntries)
        .arg(apuStats.clockCalls)
        .arg(apuStats.reads)
        .arg(apuStats.writes)
        .arg(apuStats.irqLineChanges)
        .arg(viewPaintStats.calls)
        .arg(viewPaintMs, 0, 'f', 3);
    qInfo().noquote() << diagnostics;
    if (this->performanceLogFile.isOpen()) {
        this->performanceLogStream << diagnostics << '\n';
        this->performanceLogStream.flush();
    }
    if (Mapper001 *mapper = this->nes.cartridge().mapper001()) {
        const Mapper001::Stats mapperStats = mapper->takeStats();
        qInfo().noquote()
            << QStringLiteral("MMC1 serialWrites=%1 chrBankCommits=%2 "
                              "effectiveChrChanges=%3 resetWrites=%4")
                .arg(mapperStats.serialWrites)
                .arg(mapperStats.chrBankCommits)
                .arg(mapperStats.effectiveChrChanges)
                .arg(mapperStats.resetWrites);
    }

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

