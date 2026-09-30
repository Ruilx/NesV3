#pragma once

#include <QGraphicsView>
#include <QElapsedTimer>
#include <QTimer>

class NesView : public QGraphicsView {
    Q_OBJECT

public:
    enum class InputMode {
        Game,
        Debug,
    };

    explicit NesView(QWidget *parent = nullptr);

    [[nodiscard]] qreal zoomFactor() const;
    [[nodiscard]] InputMode inputMode() const;
    void setInputMode(InputMode mode);
    void resetZoom();

    struct PaintStats {
        quint64 calls = 0;
        quint64 nanoseconds = 0;
    };

    [[nodiscard]] PaintStats takePaintStats();

signals:
    void buttonChanged(int button, bool pressed);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void setZoomFactor(qreal factor, const QPointF &viewAnchor);
    void updateScrollTimer();
    void scrollNametable();

    qreal zoom = 1.0;
    bool panning = false;
    QPoint lastMousePosition;
    QTimer scrollTimer;
    InputMode currentInputMode = InputMode::Game;
    bool scrollLeft = false;
    bool scrollRight = false;
    bool scrollUp = false;
    bool scrollDown = false;
    PaintStats paintStats;
};
