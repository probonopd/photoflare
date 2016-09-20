#include "StampTool.h"
#include "../PaintWidget.h"

#include <QPainter>
#include <QtMath>

class StampToolPrivate
{
public:
    StampToolPrivate()
    {
        primaryPen = QPen(QBrush(), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        secondaryPen = QPen(QBrush(), 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);

        selectMode = false;
        selectPos = QPoint(0,0);
        offset = QPoint(0,0);
    }
    ~StampToolPrivate()
    {

    }

    QPoint lastPos;
    QPen primaryPen;
    QPen secondaryPen;
    Qt::MouseButton mouseButton;
    int radius;
    int pressure;
    int step;
    bool fixed;
    bool diffuse;
    bool selectMode;
    QPoint selectPos;
    QPoint offset;
    float opacity;
};

StampTool::StampTool(QObject *parent)
    : Tool(parent)
    , d(new StampToolPrivate)
{

}

StampTool::~StampTool()
{
    delete d;
}

void StampTool::setPrimaryColor(const QColor &color)
{
    d->primaryPen.setColor(color);
}

QColor StampTool::primaryColor() const
{
    return d->primaryPen.color();
}

void StampTool::setSecondaryColor(const QColor &color)
{
    d->secondaryPen.setColor(color);
}

QColor StampTool::secondaryColor() const
{
    return d->secondaryPen.color();
}

void StampTool::setRadius(int radius)
{
    d->radius = radius;
    emit cursorChanged(getCursor());
}

void StampTool::setPressure(int pressure)
{
    d->pressure = pressure;
}

void StampTool::setStep(int step)
{
    d->step = step;
}

void StampTool::setFixed(bool fixed)
{
    d->fixed = fixed;
}

void StampTool::setDiffuse(bool diffuse)
{
    d->diffuse = diffuse;
}

void StampTool::setCapStyle(Qt::PenCapStyle capStyle)
{
    d->primaryPen.setCapStyle(capStyle);
    d->secondaryPen.setCapStyle(capStyle);
}

QCursor StampTool::getCursor()
{
    int width = d->radius * m_scale;
    if(width < 5)
        width = 5;
    QPixmap pixmap(QSize(width,width));
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    QPen pen = QPen(QBrush(), 1, Qt::DashLine);
    pen.setColor(Qt::gray);
    painter.setPen(pen);

    painter.drawEllipse(pixmap.rect());
    return QCursor(pixmap);
}

void StampTool::onMousePress(const QPoint &pos, Qt::MouseButton button)
{
    Q_UNUSED(button);

    d->lastPos = pos;
    d->mouseButton = button;
    d->opacity = d->pressure / 10.0f;

    if(d->selectMode)
    {
        d->selectPos = pos;
    } else
    {
        if(d->selectPos.x() >= 0 || d->fixed)
        {
            d->offset = pos - d->selectPos;
        } else
        {
            d->selectPos = pos - d->offset;
        }

        if (m_paintDevice) {
            const QImage *image = dynamic_cast<QImage*>(m_paintDevice);
            QImage surface = QImage(image->size(), QImage::Format_ARGB32_Premultiplied);
            QPainter painter(&surface);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.fillRect(surface.rect(), Qt::transparent);

            QPen pen = QPen(QBrush(), 1, Qt::DashLine);
            pen.setColor(Qt::gray);
            painter.setPen(pen);

            painter.drawEllipse(d->selectPos, 5, 5);
            emit overlaid(m_paintDevice, surface, QPainter::CompositionMode_SourceOver);
        }
    }
}

void StampTool::onMouseMove(const QPoint &pos)
{
    if (m_paintDevice) {
        const QImage *image = dynamic_cast<QImage*>(m_paintDevice);
        QImage surface = QImage(image->size(), QImage::Format_ARGB32_Premultiplied);
        QPainter cursor(&surface);
        cursor.setCompositionMode(QPainter::CompositionMode_Source);
        cursor.fillRect(surface.rect(), Qt::transparent);

        QPen pen = QPen(QBrush(), 1, Qt::DashLine);
        pen.setColor(Qt::gray);
        cursor.setPen(pen);

        cursor.drawEllipse(pos - d->offset, 5, 5);

        if(d->fixed)
        {
            QPoint diff = d->lastPos-pos;
            if(qSqrt(diff.x()*diff.x() + diff.y()*diff.y()) >= d->step)
            {
                d->lastPos = pos;
            } else
            {
                return;
            }
        }

        QPainter painter(m_paintDevice);
        painter.setOpacity(d->opacity);
        QPoint base;
        if(d->fixed)
        {
            base = d->selectPos;
        } else
        {
            base = pos - d->offset;
        }

        for(int i=-d->radius/2; i < d->radius/2;i++)
        {
            for(int j=-d->radius/2; j < d->radius/2;j++)
            {
                if(d->diffuse && (float)rand()/(float)RAND_MAX > 0.5f)
                    continue;

                if(base.x() + i < 0 || base.y() + j < 0)
                    continue;

                if(base.x() + i >= image->width() || base.y() + j >= image->height())
                    continue;

                if(i*i + j*j > d->radius*d->radius/4)
                    continue;
                pen.setColor(image->pixel(base.x() + i,base.y() + j));
                painter.setPen(pen);
                painter.drawPoint(pos.x() + i, pos.y() + j);
            }
        }

        emit painted(m_paintDevice);
        emit overlaid(m_paintDevice, surface, QPainter::CompositionMode_SourceOver);
    }
}

void StampTool::onMouseRelease(const QPoint &pos)
{
    Q_UNUSED(pos);

    d->mouseButton = Qt::NoButton;
    if(!d->selectMode && !d->fixed)
    {
        d->selectPos.setX(-1);
    }
    emit painted(m_paintDevice);
}

void StampTool::onKeyPressed(QKeyEvent * keyEvent)
{
    if(keyEvent->key() == Qt::Key_Control)
    {
        d->selectMode = true;
        emit cursorChanged(Qt::ArrowCursor);
    }
}

void StampTool::onKeyReleased(QKeyEvent * keyEvent)
{
    if(keyEvent->key() == Qt::Key_Control)
    {
        d->selectMode = false;
        emit cursorChanged(getCursor());
    }
}
