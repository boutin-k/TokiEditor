#ifndef MDICHILD_H
#define MDICHILD_H

#include <cstring>

#include <QMap>
#include <QWidget>
#include <QDomDocument>
#include <QFileSystemWatcher>

#include "tkdata.h"
#include "tkgriditem.h"

class TkGridLayout;

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
  static uint32_t sMdiChildCounter;

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

  void setData(const levelData &d);
  inline const levelData& getData() const { return _data; }

  inline void setModified(bool m) { _isModified = m; documentWasModified(); }
  inline bool isModified() const { return _isModified; }

  inline const QString tilePath() { return _data.tileFile; }
  inline const QString eggsPath() { return _data.eggsFile; }
  inline const QString backPath() { return _data.backFile; }
  inline const QString musicPath() { return _data.musicFile; }
  inline const QSize levelSize() { return QSize(_data.gridWidth, _data.gridHeight); }

  inline TkGridItem *getTokiTile() const { return tokiTile; }
  inline void setTokiTile(TkGridItem *tile) { tokiTile = tile; }

 signals:
  void itemClicked(TkGridItem*, Qt::MouseButton);

 protected:
  void closeEvent(QCloseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

 private slots:
  void documentWasModified();
  void mouseButtonEvent(QWidget *w, QMouseEvent *ev);

 private:
  void buildGrid();
  TkGridItem *getNewGridItem();

  bool maybeSave();
  void setCurrentFile(const QString &fileName);
  QString strippedName(const QString &fullFileName);

  QDomDocument getDomDocument(const QString &fileName);
  static void parseDom(const QDomElement &docElem, std::list<shoeboxData>& list);

  void updateShoebox();


  /**
   * @brief The Overlay class
   */
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

  uint32_t mdiChildId{sMdiChildCounter++};

  QString _curFile;

  TkGridLayout* gridLayout{nullptr};
  QWidget* gridWidget{nullptr};

  TkGridItem *tokiTile{nullptr};

  bool _isUntitled{true};
  bool _isModified{false};
  bool _isLeftMouseButtonPressed{false};
  bool _isRightMouseButtonPressed{false};

  levelData _data;

  Overlay* overlay{nullptr};
  bool shoeboxDisplayed{true};
  std::list<shoeboxData> shoeboxList;
  std::list<shoeboxData> overlayList;
  QFileSystemWatcher mFileWatcher;
};

#endif
