#ifndef TKGRIDITEM_H
#define TKGRIDITEM_H

#include <QObject>
#include <QMap>

#include "tklabel.h"
#include "tkdata.h"

class TkGridItem : public TkLabel
{
  Q_OBJECT
  Q_PROPERTY(uint tile MEMBER mTile READ getTile WRITE setTile)

 public:
  explicit TkGridItem(QWidget* parent = nullptr,
                       Qt::WindowFlags f = Qt::WindowFlags());
  explicit TkGridItem(const QString& text, QWidget* parent = nullptr,
                       Qt::WindowFlags f = Qt::WindowFlags());
  ~TkGridItem();

  bool clearPixmapLayer(uint32_t index);
  bool updatePixmapLayer(const QPixmap& pixmap, uint32_t index, uint32_t tile, bool forceUpdate = false);

  void updateLayerVisibility(const QMap<TkLayer, bool>& visibility) {
    bool update = false;

    for (uint32_t index = 0U; index < TkLayer::layerCount; ++index) {
      bool value = visibility.value((TkLayer)index, false);
      if (value != visibilityMask[index]) {
        visibilityMask[index] = value;
        // TODO - Check layer mask before update
        update = true;
      }
    }
    if (update) drawPixmap();
  }

  inline uint getTile() { return mTile; }
  inline void setTile(uint tile) { mTile = tile; }

 private:
  void drawPixmap();

  QPixmap layer[TkLayer::layerCount];
  bool visibilityMask[TkLayer::layerCount] = {true, true, true};

  static constexpr uint32_t layerMask[TkLayer::layerCount] = {0xFF, 0xFF00, 0xFF0000};

  static constexpr auto sTransHoverBackground{":hover {background-color: rgba(127, 127, 127, 0)}"};
  static constexpr auto sGreyHoverBackground{":hover {background-color: rgba(127, 127, 127, 127)}"};

  uint mTile{0xFF};
};

#endif // TKGRIDITEM_H
