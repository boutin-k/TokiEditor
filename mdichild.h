#ifndef MDICHILD_H
#define MDICHILD_H

#include <cstring>

#include <QMap>
#include <QWidget>
#include <QGridLayout>
#include <QDomDocument>

#include "tkdata.h"
#include "tkgriditem.h"

class MdiChild : public QWidget
{
  Q_OBJECT

 public:
  MdiChild();

  void newFile();
  bool loadFile(const QString &fileName);
  bool save();
  bool saveAs();
  bool saveFile(const QString &fileName);
  void cut();
  void copy();
  void paste();
  bool hasSelection();
  QString userFriendlyCurrentFile();
  inline QString currentFile() { return _curFile; }

  void updateGrid(const QMap<TkLayer, bool>& visibility);

  inline void setData(const levelData &d) {
    bool update = std::strcmp(d.backFile, _data.backFile);
    if (_data != d) {
      _data = d;
      setModified(true);
    }
    if (update) updateBackground();
  }
  inline const levelData& getData() { return _data; }

  inline void setModified(bool m) { _isModified = m; documentWasModified(); }
  inline bool isModified() { return _isModified; }

  inline const QString tilePath() { return _data.tileFile; }
  inline const QString eggsPath() { return _data.eggsFile; }
  inline const QString backPath() { return _data.backFile; }
  inline const QString musicPath() { return _data.musicFile; }
  inline const QSize levelSize() { return QSize(_data.gridWidth, _data.gridHeight); }

  inline TkGridItem *getTokiTile() const { return tokiTile; }
  inline void setTokiTile(TkGridItem *tile) { tokiTile = tile; }


 signals:
  void itemClicked(TkGridItem*, QMouseEvent*);
  void itemHovered(TkGridItem*, QHoverEvent*);

 protected:
  void closeEvent(QCloseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

 private slots:
  void documentWasModified();
  void mouseButtonEvent(QWidget *w, QMouseEvent *ev);
  void mouseHoverEvent(QWidget *w, QHoverEvent *ev);

 private:
  void buildGrid();
  bool maybeSave();
  void setCurrentFile(const QString &fileName);
  QString strippedName(const QString &fullFileName);

  QDomDocument getDomDocument(const QString &fileName);
  void parseDom(const QDomElement &docElem);

  void updateBackground();

  QString _curFile;
  QGridLayout _gridLayout;
  TkGridItem *tokiTile{nullptr};

  bool _isUntitled{true};
  bool _isModified{false};

  levelData _data;

  QList<QPair<uint32_t, QSharedPointer<QPixmap>>> wallpaperList;
};

#endif