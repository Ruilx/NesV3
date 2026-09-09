#pragma once

#include <QColor>
#include <QVector>

class NesPalette {
public:
    static constexpr int ColorCount = 64;

    NesPalette();
    explicit NesPalette(const QVector<QColor> &colors);

    [[nodiscard]] const QVector<QColor> &colors() const;
    [[nodiscard]] QColor colorAt(int index) const;
    void setColor(int index, const QColor &color);

    [[nodiscard]] static QVector<QColor> standard2C02GUPalette();

private:
    QVector<QColor> paletteColors;
};
