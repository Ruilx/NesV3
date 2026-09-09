#include "PaletteEditorDialog.h"

#include "NesPalette.h"

#include <QColorDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

PaletteEditorDialog::PaletteEditorDialog(QWidget *parent)
    : QDialog(parent), currentPalette(NesPalette::standard2C02GUPalette()) {
    setWindowTitle(tr("Palette Settings"));

    auto *layout = new QVBoxLayout(this);
    auto *colorGrid = new QGridLayout;

    for (int index = 0; index < NesPalette::ColorCount; ++index) {
        auto *button = new QPushButton(this);
        button->setFixedSize(72, 42);
        connect(button, &QPushButton::clicked, this, [this, index]() { chooseColor(index); });
        colorButtons.append(button);
        colorGrid->addWidget(button, index / 8, index % 8);
    }

    layout->addLayout(colorGrid);

    auto *tools = new QHBoxLayout;
    auto *openButton = new QPushButton(tr("Open .pal"), this);
    auto *resetButton = new QPushButton(tr("Reset Default"), this);
    connect(openButton, &QPushButton::clicked, this, &PaletteEditorDialog::openPaletteFile);
    connect(resetButton, &QPushButton::clicked, this, &PaletteEditorDialog::resetToDefault);
    tools->addWidget(openButton);
    tools->addWidget(resetButton);
    tools->addStretch();
    layout->addLayout(tools);

    auto *dialogButtons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(dialogButtons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(dialogButtons);

    refreshButtons();
}

void PaletteEditorDialog::setPalette(const QVector<QColor> &palette) {
    if (palette.size() != NesPalette::ColorCount) {
        return;
    }

    currentPalette = palette;
    refreshButtons();
}

QVector<QColor> PaletteEditorDialog::palette() const {
    return currentPalette;
}

void PaletteEditorDialog::chooseColor(int index) {
    if (index < 0 || index >= currentPalette.size()) {
        return;
    }

    const QColor selectedColor = QColorDialog::getColor(currentPalette[index], this,
                                                        tr("Choose palette color %1").arg(index, 2, 16, QLatin1Char('0')));
    if (!selectedColor.isValid()) {
        return;
    }

    currentPalette[index] = selectedColor;
    refreshButtons();
    emit paletteChanged(currentPalette);
}

void PaletteEditorDialog::resetToDefault() {
    currentPalette = NesPalette::standard2C02GUPalette();
    refreshButtons();
    emit paletteChanged(currentPalette);
}

void PaletteEditorDialog::openPaletteFile() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Open RGB palette"), QString(),
                                                      tr("Raw RGB palette (*.pal *.rgb);;All files (*)"));
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Palette Error"), tr("Could not open the palette file."));
        return;
    }

    const QByteArray data = file.readAll();
    if (data.size() != NesPalette::ColorCount * 3) {
        QMessageBox::warning(this, tr("Palette Error"),
                             tr("This version accepts only 192-byte raw RGB palettes."));
        return;
    }

    QVector<QColor> importedPalette;
    importedPalette.reserve(NesPalette::ColorCount);
    for (int index = 0; index < NesPalette::ColorCount; ++index) {
        const int offset = index * 3;
        importedPalette.append(QColor(
            static_cast<unsigned char>(data[offset]),
            static_cast<unsigned char>(data[offset + 1]),
            static_cast<unsigned char>(data[offset + 2])));
    }

    currentPalette = importedPalette;
    refreshButtons();
    emit paletteChanged(currentPalette);
}

void PaletteEditorDialog::refreshButtons() {
    for (int index = 0; index < colorButtons.size(); ++index) {
        const QColor color = currentPalette.value(index, Qt::black);
        colorButtons[index]->setText(QString("$%1").arg(index, 2, 16, QLatin1Char('0')).toUpper());
        colorButtons[index]->setStyleSheet(QString("color: %1; background-color: %2;")
                                                .arg(color.lightness() > 128 ? "black" : "white", color.name()));
        colorButtons[index]->setToolTip(tr("RGB(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue()));
    }
}
