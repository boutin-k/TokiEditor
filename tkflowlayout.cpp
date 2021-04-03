#include <QtWidgets>

#include "tkflowlayout.h"

/**
 * @brief Constructor with parent widget
 * @param[in,out] parent Parent widget
 * @param[in] margin Layout content margin
 * @param[in] hSpacing Horizontal layout content spacing
 * @param[in] vSpacing Vertical layout content spacing
 */
TkFlowLayout::TkFlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), hSpace(hSpacing), vSpace(vSpacing)
{
  setContentsMargins(margin, margin, margin, margin);
}

/**
 * @brief Constructor
 * @param[in] margin Layout content margin
 * @param[in] hSpacing Horizontal layout content spacing
 * @param[in] vSpacing Vertical layout content spacing
 */
TkFlowLayout::TkFlowLayout(int margin, int hSpacing, int vSpacing)
    : hSpace(hSpacing), vSpace(vSpacing)
{
  setContentsMargins(margin, margin, margin, margin);
}

/**
 * @brief Destructor
 */
TkFlowLayout::~TkFlowLayout()
{
  while (itemList.size())
    delete itemList.takeAt(0);
}

/**
 * @brief Add item in the layout
 * @param[in] item Item to add
 */
void TkFlowLayout::addItem(QLayoutItem *item)
{
  itemList.append(item);
}

/**
 * @brief Get the horizontal spacing
 * @return The horizontal spacing
 */
int TkFlowLayout::horizontalSpacing() const
{
  if (hSpace >= 0) {
    return hSpace;
  } else {
    return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
  }
}

/**
 * @brief Get the vertical spacing
 * @return The vertical spacing
 */
int TkFlowLayout::verticalSpacing() const
{
  if (vSpace >= 0) {
    return vSpace;
  } else {
    return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
  }
}

/**
 * @brief Get the number of item in the layout
 * @return The number of child item
 */
int TkFlowLayout::count() const
{
  return itemList.size();
}

/**
 * @brief Get an item related to its position
 * @param[in] index The position of the item in the item list
 * @return The item pointer or \c nullptr if not found
 */
QLayoutItem *TkFlowLayout::itemAt(int index) const
{
  return itemList.value(index);
}

/**
 * @brief Take an item related to its position
 * @param index The position of the item in the item list
 * @return The item pointer or \c nullptr if not found
 */
QLayoutItem *TkFlowLayout::takeAt(int index)
{
  if (index >= 0 && index < itemList.size())
    return itemList.takeAt(index);
  return nullptr;
}

/**
 * @brief Unused
 * @return Empty object
 */
Qt::Orientations TkFlowLayout::expandingDirections() const
{
  return { };
}

/**
 * @brief hasHeightForWidth
 * @return Always \c true
 */
bool TkFlowLayout::hasHeightForWidth() const
{
  return true;
}

/**
 * @brief height for width
 * @param[in] width Width value
 * @return The height related to the width
 */
int TkFlowLayout::heightForWidth(int width) const
{
  int height = doLayout(QRect(0, 0, width, 0), true);
  return height;
}

/**
 * @brief Set the layout geometry
 * @param[in] rect The new geometry
 */
void TkFlowLayout::setGeometry(const QRect &rect)
{
  QLayout::setGeometry(rect);
  doLayout(rect, false);
}

/**
 * @brief Get the size hint
 * @return The size hint
 */
QSize TkFlowLayout::sizeHint() const
{
  return minimumSize();
}

/**
 * @brief Get the minimum size
 * @return The minimum size
 */
QSize TkFlowLayout::minimumSize() const
{
  QSize size;
  for (const QLayoutItem *item : qAsConst(itemList))
    size = size.expandedTo(item->minimumSize());

  const QMargins margins = contentsMargins();
  size += QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
  return size;
}

/**
 * @brief Build the layout
 * @param[in] rect The new geometry
 * @param[in] testOnly Apply or not the geometry
 * @return The height of the layout content
 */
int TkFlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
  int left, top, right, bottom;
  getContentsMargins(&left, &top, &right, &bottom);
  QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
  int x = effectiveRect.x();
  int y = effectiveRect.y();
  int lineHeight = 0;

  for (QLayoutItem *item : qAsConst(itemList)) {
    const QWidget *wid = item->widget();
    int spaceX = horizontalSpacing();
    if (spaceX == -1)
      spaceX = wid->style()->layoutSpacing(
          QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
    int spaceY = verticalSpacing();
    if (spaceY == -1)
      spaceY = wid->style()->layoutSpacing(
          QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);

    int nextX = x + item->sizeHint().width() + spaceX;
    if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
      x = effectiveRect.x();
      y = y + lineHeight + spaceY;
      nextX = x + item->sizeHint().width() + spaceX;
      lineHeight = 0;
    }

    if (!testOnly)
      item->setGeometry(QRect(QPoint(x, y), item->sizeHint()));

    x = nextX;
    lineHeight = qMax(lineHeight, item->sizeHint().height());
  }
  return y + lineHeight - rect.y() + bottom;
}

/**
 * @brief smart spacing
 * @param[in] pm The pixel metric
 * @return The spacing between the widgets
 */
int TkFlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
  QObject *parent = this->parent();
  if (!parent) {
    return -1;
  } else if (parent->isWidgetType()) {
    QWidget *pw = static_cast<QWidget *>(parent);
    return pw->style()->pixelMetric(pm, nullptr, pw);
  } else {
    return static_cast<QLayout *>(parent)->spacing();
  }
}
