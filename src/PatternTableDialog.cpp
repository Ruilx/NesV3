#include "PatternTableDialog.h"

#include "ChrTileDecoder.h"
#include "NesTileItem.h"

#include <QDialogButtonBox>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
constexpr int TilesPerRow = 16;
constexpr int TilesPerPatternTable = 256;
constexpr int PatternTablePixelWidth = TilesPerRow * NesTileItem::TileSize;
constexpr int PatternTablePixelHeight = PatternTablePixelWidth;
constexpr int PatternTableCount = 2;
}

PatternTableDialog::PatternTableDialog(
        Bus &ppuBus,
        const NesPalette &palette,
        QWidget *parent)
    : QDialog(parent),
      ppuBus(ppuBus),
      nesPalette(palette),
      patternScene(new QGraphicsScene(this)),
      patternView(new QGraphicsView(patternScene, this)) {
    setWindowTitle(tr("CHR Pattern Tables"));
    resize(600, 420);

    patternScene->setSceneRect(
            0,
            0,
            PatternTablePixelWidth * PatternTableCount,
            PatternTablePixelHeight);
    patternView->setRenderHint(QPainter::Antialiasing, false);
    patternView->setBackgroundBrush(Qt::black);
    patternView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    patternView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    patternView->setInteractive(false);

    auto *refreshButton = new QPushButton(tr("Refresh"), this);
    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(refreshButton, &QPushButton::clicked,
            this, &PatternTableDialog::refreshPatternTables);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(buttonBox);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(patternView);
    layout->addLayout(buttonLayout);

    for (int patternTable = 0; patternTable < PatternTableCount; ++patternTable) {
        for (int tileIndex = 0; tileIndex < TilesPerPatternTable; ++tileIndex) {
            auto *tileItem = new NesTileItem;
            tileItem->setPalette(this->nesPalette.colors());
            tileItem->setGridVisible(true);
            tileItem->setPos(
                    patternTable * PatternTablePixelWidth
                        + (tileIndex % TilesPerRow) * NesTileItem::TileSize,
                    (tileIndex / TilesPerRow) * NesTileItem::TileSize);
            this->patternScene->addItem(tileItem);
            this->tileItems.append(tileItem);
        }
    }

    refreshPatternTables();
}

void PatternTableDialog::refreshPatternTables() {
    int itemIndex = 0;
    for (int patternTable = 0; patternTable < PatternTableCount; ++patternTable) {
        const auto table = patternTable == 0
                ? ChrTileDecoder::PatternTable::Lower
                : ChrTileDecoder::PatternTable::Upper;
        for (int tileIndex = 0; tileIndex < TilesPerPatternTable; ++tileIndex) {
            QVector<quint8> pixels;
            const bool decoded = ChrTileDecoder::decodeTile(
                    this->ppuBus,
                    static_cast<quint16>(tileIndex),
                    table,
                    pixels);
            if (!decoded) {
                pixels.fill(0, NesTileItem::TileSize * NesTileItem::TileSize);
            }
            this->tileItems[itemIndex]->setPixels(pixels);
            ++itemIndex;
        }
    }
    this->patternScene->update();
}