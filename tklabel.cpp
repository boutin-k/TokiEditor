#include "tklabel.h"

#include <QEvent>
#include <QMouseEvent>

/**
 * @brief TkLabel constructor
 * @param[in, out] parent The parent widget
 * @param[in] flags The window flags
 */
TkLabel::TkLabel(QWidget *parent, Qt::WindowFlags flags) : TkLabel("", parent, flags) {}

/**
 * @brief TkLabel constructor
 * @param[in] text The text to display in the label
 * @param[in,out] parent The parent widget
 * @param[in] flags The window flags
 */
TkLabel::TkLabel(const QString &text, QWidget *parent, Qt::WindowFlags flags)
    : QLabel(text, parent, flags) {
  installEventFilter(this);
}

/**
 * @brief Destructor
 */
TkLabel::~TkLabel() {}

/**
 * @brief Overrided method used to catch mouse events
 * @param{in, out] obj The QObject related to the event
 * @param{in, out] event The event type
 * @return \c true if the event is handled, otherwise \c false
 */
bool TkLabel::eventFilter(QObject *obj, QEvent *event) {
  if (obj->isWidgetType()) {
    switch (event->type()) {
      case QEvent::MouseMove:
      case QEvent::MouseButtonPress:
      case QEvent::MouseButtonRelease:
      case QEvent::MouseButtonDblClick:
        emit mouseButtonEvent(static_cast<TkLabel *>(obj),
                              static_cast<QMouseEvent *>(event));
        event->accept();
        break;
      default:
        break;
    }
  }
  return QObject::eventFilter(obj, event);
}
