#include "tklabel.h"

#include <QEvent>
#include <QMouseEvent>

TkLabel::TkLabel(QWidget *parent, Qt::WindowFlags f) : TkLabel("", parent, f) {}
TkLabel::TkLabel(const QString &text, QWidget *parent, Qt::WindowFlags f)
    : QLabel(text, parent, f) {
  installEventFilter(this);
}

TkLabel::~TkLabel() {}

bool TkLabel::eventFilter(QObject *obj, QEvent *event) {
  if (obj->isWidgetType()) {
    switch (event->type()) {
      case QEvent::MouseButtonPress:
      case QEvent::MouseButtonRelease:
      case QEvent::MouseButtonDblClick:
        emit mouseButtonEvent(static_cast<TkLabel *>(obj),
                              static_cast<QMouseEvent *>(event));
        event->accept();
        break;
      case QEvent::HoverEnter:
      case QEvent::HoverLeave:
        emit mouseHoverEvent(static_cast<TkLabel *>(obj),
                             static_cast<QHoverEvent *>(event));
        event->accept();
        break;
      default:
        break;
    }
  }
  return QObject::eventFilter(obj, event);
}
