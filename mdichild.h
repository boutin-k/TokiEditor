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
  inline QString currentFile() { return curFile; }

  void updateGrid(const QMap<TkLayer, bool>& visibility);
  inline void updateShoeboxState(bool enabled) {
    overlay->setVisible(shoeboxDisplayed = enabled);
  }

  void setData(const levelData &d);
  inline const levelData& getData() const { return data; }

  inline void setModified(bool m) { modified = m; documentWasModified(); }
  inline bool isModified() const { return modified; }

  inline const QString tilePath() { return data.tileFile; }
  inline const QString eggsPath() { return data.eggsFile; }
  inline const QString backPath() { return data.backFile; }
  inline const QString musicPath() { return data.musicFile; }
  inline const QSize levelSize() { return QSize(data.gridWidth, data.gridHeight); }

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
  TkGridItem *getNewGridItem(uint32_t row, uint32_t col, uint32_t mdiId);

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

  QString curFile;

  TkGridLayout* gridLayout{nullptr};
  QWidget* gridWidget{nullptr};

  TkGridItem *tokiTile{nullptr};

  bool untitled{true};
  bool modified{false};
  bool leftMouseButtonPressed{false};
  bool rightMouseButtonPressed{false};

  levelData data;

  Overlay* overlay{nullptr};
  bool shoeboxDisplayed{true};
  std::list<shoeboxData> shoeboxList;
  std::list<shoeboxData> overlayList;
  QFileSystemWatcher fileWatcher;
};

#endif
