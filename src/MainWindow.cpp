#include "MainWindow.h"

#include "NesPalette.h"
#include "NesScene.h"
#include "NesTileItem.h"
#include "NesView.h"
#include "PaletteEditorDialog.h"
#include "PatternTableDialog.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>
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
    if (!this->nes.cartridge().loadFromFile(path, error)) {
        QMessageBox::critical(this, tr("Open ROM failed"), error);
        return;
    }

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
    this->simulationTimer.start();
    this->currentRomPath = path;
    updateWindowTitle();
    if (auto *view = qobject_cast<NesView *>(this->centralWidget())) {
        view->setFocus(Qt::OtherFocusReason);
        qInfo() << "NesView focus restored after ROM load:" << view->hasFocus();
    }
}

void MainWindow::closeRom() {
    this->simulationTimer.stop();
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
    const QString message = QStringLiteral(
        "FPS %1 | core %2 ms | PC %3:%4 now %5 SP %6 int %7 NMI %8 RTI %9 | maskWrites %10 lastMask $%11 DMA %12 spriteOut %13 spriteFail %14 | PPU $2000 %15 $2001 %16 $2002 %17 | buttons $%18 seen $%19")
        .arg(simulationFps, 0, 'f', 1)
        .arg(averageCoreMs, 0, 'f', 3)
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

    this->performanceTimer.restart();
    this->performanceFrameCount = 0;
    this->performanceCoreNanoseconds = 0;
    this->performanceSceneNanoseconds = 0;
    this->performanceDirtyTiles = 0;
    this->performanceDecodedTiles = 0;
    this->performanceUpdatedTiles = 0;
    NesTileItem::resetPaintStats();
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

