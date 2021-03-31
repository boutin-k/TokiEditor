#include "tkgriditem.h"

#include <QPainter>
#include <QVariant>
#include <QMouseEvent>

constexpr uint32_t TkGridItem::layerMask[];

TkGridItem::TkGridItem(QWidget* parent, Qt::WindowFlags f)
    : TkGridItem("", parent, f) {}

TkGridItem::TkGridItem(const QString& text, QWidget* parent,
                         Qt::WindowFlags f)
    : TkLabel(text, parent, f) {
  installEventFilter(this);
  setProperty("tile", 0xFF);
  setAttribute(Qt::WA_Hover);
  setStyleSheet(sGreyHoverBackground);
  drawPixmap();

  connect(this, &TkGridItem::mouseButtonEvent, [this] (TkLabel*, QMouseEvent* ev){
    if (ev->type() == QEvent::MouseButtonPress)
      return setStyleSheet(sTransHoverBackground);

    if (ev->type() == QEvent::MouseButtonRelease)
      return setStyleSheet(sGreyHoverBackground);
  });
}

TkGridItem::~TkGridItem() {}

bool TkGridItem::clearPixmapLayer(uint32_t index) {
  bool updated = false;

  uint32_t tile = property("tile").toUInt();
  uint32_t newValue = (tile & ~TkGridItem::layerMask[index]) +
                      (0xFF & TkGridItem::layerMask[index]);
  if (newValue != tile) {
    setProperty("tile", newValue);
    layer[index] = QPixmap();
    drawPixmap();

    updated = true;
  }

  return updated;
}

bool TkGridItem::updatePixmapLayer(const QPixmap& pixmap, uint32_t index,
                                   uint32_t tile, bool forceUpdate) {
  bool updated = false;

  uint32_t currentValue = property("tile").toUInt();
  if (index < TkLayer::layerCount) {
    uint32_t newValue = ((currentValue & ~TkGridItem::layerMask[index]) | (tile << index * 8));
    if (newValue != currentValue || forceUpdate) {
      layer[index] = pixmap;
      drawPixmap();
      setProperty("tile", newValue);

      updated = true;
    }
  }

  return updated;
}

void TkGridItem::drawPixmap() {
  static const QPixmap defaultPixmap(":/images/grid.png");
  QPixmap tempPixmap = defaultPixmap;

  QPainter painter;
  painter.begin(&tempPixmap);
  if (visibilityMask[TkLayer::background])
    painter.drawPixmap(layer[TkLayer::background].rect(), layer[TkLayer::background]);
  if (visibilityMask[TkLayer::ground])
    painter.drawPixmap(layer[TkLayer::ground].rect(), layer[TkLayer::ground]);
  if (visibilityMask[TkLayer::foreground])
    painter.drawPixmap(layer[TkLayer::foreground].rect(), layer[TkLayer::foreground]);
  painter.end();
  setPixmap(tempPixmap);
}
