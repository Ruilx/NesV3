#include "NesView.h"

#include "NesScene.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QWheelEvent>
#include <QDebug>

#include <algorithm>

NesView::NesView(QWidget *parent) : QGraphicsView(parent) {
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::NoAnchor);
    setResizeAnchor(QGraphicsView::NoAnchor);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setRenderHint(QPainter::Antialiasing, false);
    setRenderHint(QPainter::SmoothPixmapTransform, false);
    setBackgroundBrush(QColor(24, 24, 28));
    setDragMode(QGraphicsView::NoDrag);
    setInteractive(true);
    setFocusPolicy(Qt::StrongFocus);
    this->scrollTimer.setInterval(16);
    connect(&this->scrollTimer, &QTimer::timeout, this, &NesView::scrollNametable);
    scale(this->zoom, this->zoom);
}

qreal NesView::zoomFactor() const {
    return this->zoom;
}

NesView::InputMode NesView::inputMode() const {
    return this->currentInputMode;
}

void NesView::setInputMode(InputMode mode) {
    if (this->currentInputMode == mode) {
        return;
    }

    this->scrollLeft = false;
    this->scrollRight = false;
    this->scrollUp = false;
    this->scrollDown = false;
    this->scrollTimer.stop();
    this->currentInputMode = mode;
}

void NesView::resetZoom() {
    const QPointF viewCenter = viewport()->rect().center();
    setZoomFactor(1.0, viewCenter);
}

void NesView::setZoomFactor(qreal factor, const QPointF &viewAnchor) {
    const qreal clampedFactor = std::clamp(factor, 0.5, 8.0);
    if (qFuzzyCompare(clampedFactor, this->zoom)) {
        return;
    }

    const QPointF sceneAnchor = mapToScene(viewAnchor.toPoint());
    const qreal scaleRatio = clampedFactor / this->zoom;
    this->zoom = clampedFactor;
    scale(scaleRatio, scaleRatio);

    const QPointF newSceneAnchor = mapToScene(viewAnchor.toPoint());
    const QPointF delta = newSceneAnchor - sceneAnchor;
    translate(delta.x(), delta.y());
}

void NesView::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() == 0) {
        event->ignore();
        return;
    }

    const qreal direction = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    setZoomFactor(this->zoom * direction, event->position());
    event->accept();
}

void NesView::mousePressEvent(QMouseEvent *event) {
    setFocus();
    if (event->button() == Qt::RightButton) {
        this->panning = true;
        this->lastMousePosition = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    QGraphicsView::mousePressEvent(event);
}

void NesView::mouseMoveEvent(QMouseEvent *event) {
    if (!this->panning) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    const QPoint delta = event->pos() - this->lastMousePosition;
    horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
    verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
    this->lastMousePosition = event->pos();
    event->accept();
}

void NesView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton && this->panning) {
        this->panning = false;
        unsetCursor();
        event->accept();
        return;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

void NesView::keyPressEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }

    int button = -1;
    bool handled = true;
    switch (event->key()) {
        case Qt::Key_Left:
            button = 6;
            break;
        case Qt::Key_Right:
            button = 7;
            break;
        case Qt::Key_Up:
            button = 4;
            break;
        case Qt::Key_Down:
            button = 5;
            break;
        case Qt::Key_A:
            button = 6;
            break;
        case Qt::Key_D:
            button = 7;
            break;
        case Qt::Key_W:
            button = 4;
            break;
        case Qt::Key_S:
            button = 5;
            break;
        case Qt::Key_H:
            button = 0;
            break;
        case Qt::Key_J:
            button = 1;
            break;
        case Qt::Key_V:
            button = 2;
            break;
        case Qt::Key_B:
            button = 3;
            break;
        default:
            handled = false;
            break;
    }

    if (!handled) {
        qInfo().noquote() << "NesView key press ignored: key=" << event->key()
                          << "text=" << event->text()
                          << "autoRepeat=" << event->isAutoRepeat();
        QGraphicsView::keyPressEvent(event);
        return;
    }

    qInfo().noquote() << "NesView key press accepted: key=" << event->key()
                      << "button=" << button
                      << "autoRepeat=" << event->isAutoRepeat();
    if (this->currentInputMode == InputMode::Debug) {
        switch (event->key()) {
            case Qt::Key_Left:
                this->scrollLeft = true;
                break;
            case Qt::Key_Right:
                this->scrollRight = true;
                break;
            case Qt::Key_Up:
                this->scrollUp = true;
                break;
            case Qt::Key_Down:
                this->scrollDown = true;
                break;
            default:
                break;
        }
        updateScrollTimer();
    } else {
        emit this->buttonChanged(button, true);
    }
    event->accept();
}

void NesView::keyReleaseEvent(QKeyEvent *event) {
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }

    int button = -1;
    bool handled = true;
    switch (event->key()) {
        case Qt::Key_Left:
            button = 6;
            break;
        case Qt::Key_Right:
            button = 7;
            break;
        case Qt::Key_Up:
            button = 4;
            break;
        case Qt::Key_Down:
            button = 5;
            break;
        case Qt::Key_A:
            button = 6;
            break;
        case Qt::Key_D:
            button = 7;
            break;
        case Qt::Key_W:
            button = 4;
            break;
        case Qt::Key_S:
            button = 5;
            break;
        case Qt::Key_H:
            button = 0;
            break;
        case Qt::Key_J:
            button = 1;
            break;
        case Qt::Key_V:
            button = 2;
            break;
        case Qt::Key_B:
            button = 3;
            break;
        default:
            handled = false;
            break;
    }

    if (!handled) {
        qInfo().noquote() << "NesView key release ignored: key=" << event->key()
                          << "text=" << event->text()
                          << "autoRepeat=" << event->isAutoRepeat();
        QGraphicsView::keyReleaseEvent(event);
        return;
    }

    qInfo().noquote() << "NesView key release accepted: key=" << event->key()
                      << "button=" << button
                      << "autoRepeat=" << event->isAutoRepeat();
    if (this->currentInputMode == InputMode::Debug) {
        switch (event->key()) {
            case Qt::Key_Left:
                this->scrollLeft = false;
                break;
            case Qt::Key_Right:
                this->scrollRight = false;
                break;
            case Qt::Key_Up:
                this->scrollUp = false;
                break;
            case Qt::Key_Down:
                this->scrollDown = false;
                break;
            default:
                break;
        }
        updateScrollTimer();
    } else {
        emit this->buttonChanged(button, false);
    }
    event->accept();
}

void NesView::updateScrollTimer() {
    const bool scrolling = this->scrollLeft || this->scrollRight
                           || this->scrollUp || this->scrollDown;
    if (scrolling) {
        if (!this->scrollTimer.isActive()) {
            this->scrollTimer.start();
        }
    } else {
        this->scrollTimer.stop();
    }
}

void NesView::scrollNametable() {
    auto *nesScene = qobject_cast<NesScene *>(scene());
    if (nesScene == nullptr) {
        return;
    }

    const qreal deltaX = (this->scrollRight ? 1.0 : 0.0)
                         - (this->scrollLeft ? 1.0 : 0.0);
    const qreal deltaY = (this->scrollDown ? 1.0 : 0.0)
                         - (this->scrollUp ? 1.0 : 0.0);
    nesScene->scrollBy(deltaX, deltaY);
}
