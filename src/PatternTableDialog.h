#pragma once

#include <QDialog>
#include <QVector>

#include "Bus.h"
#include "NesPalette.h"

class QGraphicsScene;
class QGraphicsView;
class NesTileItem;

class PatternTableDialog final : public QDialog {
public:
    explicit PatternTableDialog(
            Bus &ppuBus,
            const NesPalette &palette,
            QWidget *parent = nullptr);

private:
    void refreshPatternTables();

    Bus &ppuBus;
    NesPalette nesPalette;
    QGraphicsScene *patternScene = nullptr;
    QGraphicsView *patternView = nullptr;
    QVector<NesTileItem *> tileItems;
};