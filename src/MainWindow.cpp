#include "MainWindow.h"

#include "NesPalette.h"
#include "NesScene.h"
#include "NesView.h"
#include "PaletteEditorDialog.h"
#include "PatternTableDialog.h"

#include <QAction>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QSettings>

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

    auto *view = new NesView(this);
    this->scene = new NesScene(view);
    this->scene->setPalette(loadStoredPalette());
    view->setScene(this->scene);
    view->centerOn(this->scene->nesViewportRect().center());
    this->setCentralWidget(view);

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
        auto *patternTableAction = debugMenu->addAction(tr("CHR Pattern Tables"));
        connect(patternTableAction, &QAction::triggered,
            this, &MainWindow::openPatternTableDialog);
        updateWindowTitle();
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
    this->scene->updateFromPpu(this->nes.ppu());
    this->simulationTimer.start();
    this->currentRomPath = path;
    updateWindowTitle();
}

void MainWindow::closeRom() {
    this->simulationTimer.stop();
    this->nes.cartridge().unload();
    this->nes.cartridge().connect(
            this->nes.cpu().bus(), this->nes.ppu().bus());
    this->nes.cpu().reset();
    this->nes.ppu().reset();
    this->nes.clock().reset();
    this->currentRomPath.clear();
    updateWindowTitle();
}

void MainWindow::runSimulationFrame() {
    if (this->currentRomPath.isEmpty() || !this->nes.cartridge().isLoaded()) {
        return;
    }

    this->nes.runFrame();
    this->scene->updateFromPpu(this->nes.ppu());
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

