#pragma once

#include <QGraphicsView>
#include <QTimer>

class NesView : public QGraphicsView {
    Q_OBJECT

public:
    explicit NesView(QWidget *parent = nullptr);

    [[nodiscard]] qreal zoomFactor() const;
    void resetZoom();

signals:
    void buttonChanged(int button, bool pressed);

protected:
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
    bool scrollLeft = false;
    bool scrollRight = false;
    bool scrollUp = false;
    bool scrollDown = false;
};
