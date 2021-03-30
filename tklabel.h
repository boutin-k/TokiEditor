#ifndef TKLABEL_H
#define TKLABEL_H

#include <QLabel>

QT_BEGIN_NAMESPACE
class QMouseEvent;
class QEnterEvent;
QT_END_NAMESPACE

class TkLabel : public QLabel
{
  Q_OBJECT

 public:
  explicit TkLabel(QWidget *parent=nullptr, Qt::WindowFlags f=Qt::WindowFlags());
  explicit TkLabel(const QString &text, QWidget *parent=nullptr, Qt::WindowFlags f=Qt::WindowFlags());
  ~TkLabel();

  Q_SIGNAL void mouseButtonEvent(TkLabel *, QMouseEvent *);

 private:
  bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;
};

#endif // TKLABEL_H
