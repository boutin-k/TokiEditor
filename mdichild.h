#ifndef MDICHILD_H
#define MDICHILD_H

#include <cstring>

#include <QMap>
#include <QWidget>
#include <QGridLayout>
#include <QDomDocument>
#include <QStackedLayout>
#include <QFileSystemWatcher>

#include "tkdata.h"
#include "tkgriditem.h"

struct shoeboxData {
  int x{0};
  int y{0};
  int z{0};
  QImage i;

  shoeboxData() {}
  shoeboxData(int x, int y, int z, QImage i)
      : x(x), y(y), z(z), i(std::move(i)) {}
};

class MdiChild : public QWidget
{
  Q_OBJECT

 public:
  MdiChild();
  virtual ~MdiChild();

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
  inline void updateShoeboxState(bool enabled) {
    overlay->setVisible(shoeboxDisplayed = enabled);
  }

  inline void setData(const levelData &d) {
    bool update = std::strcmp(d.backFile, _data.backFile);
    if (_data != d) {
      _data = d;
      setModified(true);
    }
    if (update) updateShoebox();
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
  static void parseDom(const QDomElement &docElem, std::list<shoeboxData>& list);

  void updateShoebox();

  class Overlay : public QWidget {
   public:
    Overlay(QWidget *parent = nullptr) : QWidget(parent) {
      setAttribute(Qt::WA_TransparentForMouseEvents);
    };

    inline void updateShoebox(const std::list<shoeboxData> &shoebox,
                              const QSize &s) {
      overlay = std::move(shoebox);
      size = s;
      repaint();
    }

   protected:
    void paintEvent(QPaintEvent *event) override;

   private:
    std::list<shoeboxData> overlay;
    QSize size{0,0};
  };


  QString _curFile;
  QGridLayout _gridLayout;
  QStackedLayout mStackLayout;


  TkGridItem *tokiTile{nullptr};

  bool _isUntitled{true};
  bool _isModified{false};

  levelData _data;

  Overlay* overlay{nullptr};
  bool shoeboxDisplayed{true};
  std::list<shoeboxData> shoeboxList;
  std::list<shoeboxData> overlayList;
  QFileSystemWatcher mFileWatcher;
};

#endif
