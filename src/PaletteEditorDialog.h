#pragma once

#include <QColor>
#include <QDialog>
#include <QVector>

class QPushButton;

class PaletteEditorDialog : public QDialog {
    Q_OBJECT

public:
    explicit PaletteEditorDialog(QWidget *parent = nullptr);

    void setPalette(const QVector<QColor> &palette);
    [[nodiscard]] QVector<QColor> palette() const;

signals:
    void paletteChanged(const QVector<QColor> &palette);

private slots:
    void chooseColor(int index);
    void resetToDefault();
    void openPaletteFile();

private:
    void refreshButtons();

    QVector<QColor> currentPalette;
    QVector<QPushButton *> colorButtons;
};
