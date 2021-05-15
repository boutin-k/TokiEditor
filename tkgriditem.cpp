#include "tkgriditem.h"

#include <QPainter>
#include <QVariant>
#include <QMouseEvent>

constexpr uint32_t TkGridItem::layerMask[];

/**
 * @brief TkGridItem constructor
 * @param[in,out] parent The parent widget
 * @param[in] flags The window flags
 */
TkGridItem::TkGridItem(QWidget* parent, Qt::WindowFlags flags)
    : TkGridItem("", parent, flags) {}

/**
 * @brief TkGridItem constructor
 * @param[in] text The text to display in the label
 * @param[in,out] parent The parent widget
 * @param[in] flags The window flags
 */
TkGridItem::TkGridItem(const QString& text, QWidget* parent,
                         Qt::WindowFlags flags)
    : TkLabel(text, parent, flags) {
  installEventFilter(this);
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

/**
 * @brief Destructor
 */
TkGridItem::~TkGridItem() {}

/**
 * @brief Clear the pixmap layer on the gridItem
 * @param[in] index The index of the layer to clear
 * @return \c true if the layer has been clear, \c false otherwise
 */
bool TkGridItem::clearPixmapLayer(uint32_t index) {
  bool updated = false;

  uint32_t newValue = (mTile & ~TkGridItem::layerMask[index]) +
                      (0xFF & TkGridItem::layerMask[index]);
  if (newValue != mTile) {
    setTile(newValue);
    layer[index] = QPixmap();
    drawPixmap();

    updated = true;
  }

  return updated;
}

/**
 * @brief TkGridItem::updatePixmapLayer
 * @param pixmap The pixmap to use on the layer related to \a index
 * @param index The layer to update
 * @param tile The id of the pixmap
 * @param forceUpdate Force the update of the layer pixmap
 * @return \c true if the pixmap layer is updated , otherwise \c false
 */
bool TkGridItem::updatePixmapLayer(const QPixmap& pixmap, uint32_t index,
                                   uint32_t tile, bool forceUpdate) {
  bool updated = false;

  if (index < TkLayer::layerCount) {
    uint32_t newValue = ((mTile & ~TkGridItem::layerMask[index]) | (tile << index * 8));
    if (newValue != mTile || forceUpdate) {
      layer[index] = pixmap;
      drawPixmap();
      setTile(newValue);

      updated = true;
    }
  }

  return updated;
}

/**
 * @brief Draw the widget pixmap with the layers
 */
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
