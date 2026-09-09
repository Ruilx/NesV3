#include "MainWindow.h"

#include "NesPalette.h"
#include "NesScene.h"
#include "NesView.h"
#include "PaletteEditorDialog.h"

#include <QAction>
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

    auto *paletteMenu = menuBar()->addMenu(tr("Palette"));
    auto *editPaletteAction = paletteMenu->addAction(tr("Palette Settings"));
    connect(editPaletteAction, &QAction::triggered, this, &MainWindow::openPaletteEditor);
}

MainWindow::~MainWindow() {
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

