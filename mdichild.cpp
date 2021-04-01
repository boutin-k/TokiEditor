#include <QtWidgets>
#include <QDomDocument>
#include <QSharedPointer>
#include <QStackedLayout>
#include <QTimer>

#include "mdichild.h"
#include "tkgriditem.h"
#include "tkgridlayout.h"

uint32_t MdiChild::sMdiChildCounter{1U};

MdiChild::MdiChild() {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowIcon(QIcon(":/images/myappico.ico"));

  QStackedLayout* stackLayout = new QStackedLayout;
  stackLayout->setStackingMode(QStackedLayout::StackAll);
  {
    gridWidget = new QWidget(this);
    {
      gridWidget->setObjectName("Grid");
      gridLayout = new TkGridLayout();
      {
        gridLayout->setSizeConstraint(QLayout::SetFixedSize);
        gridLayout->setSpacing(0);
        gridLayout->setMdiId(mdiChildId);
      }
      gridWidget->setLayout(gridLayout);
    }
    stackLayout->addWidget(gridWidget);

    overlay = new Overlay(this);
    stackLayout->addWidget(overlay);
    overlay->raise();
  }
  stackLayout->setSizeConstraint(QLayout::SetFixedSize);
  setLayout(stackLayout);

  // Listen the shoebox file changed event
  connect(&mFileWatcher, &QFileSystemWatcher::fileChanged, this, [this]() {
    QTimer::singleShot(2000, this, &MdiChild::updateShoebox);
  });
  // Update the window size when the number of row involved
  connect(gridLayout, &TkGridLayout::rowCountChanged, this, [this](int rowCount) {
    gridWidget->setFixedHeight(rowCount << 5);
  });
  // Update the window size when the number of column involved
  connect(gridLayout, &TkGridLayout::colCountChanged, this, [this](int colCount) {
    gridWidget->setFixedWidth(colCount << 5);
  });
}

MdiChild::~MdiChild() { mFileWatcher.disconnect();
}

void MdiChild::paintEvent(QPaintEvent * event) {
  if (shoeboxDisplayed) {
    int w = _data.gridWidth << 4;
    int h = _data.gridHeight << 4;

    QPainter painter(this);
    for (const auto &item : shoeboxList) {
      int x = item.x + w - (item.i.width() >> 1);
      int y = item.y + h - (item.i.height() >> 1);
      painter.drawImage(x, y, item.i);
    }
  }
  QWidget::paintEvent(event);
}

void MdiChild::Overlay::paintEvent(QPaintEvent * event) {
  QPainter painter(this);
  for (const auto &item : overlay) {
    int x = item.x + size.width() - (item.i.width() >> 1);
    int y = item.y + size.height() - (item.i.height() >> 1);
    painter.drawImage(x, y, item.i);
  }
  QWidget::paintEvent(event);
}

void MdiChild::setData(const levelData &d) {
  bool update = std::strcmp(d.backFile, _data.backFile);
  if (_data != d) {
    // clang-format off
    int16_t  x = (d.gridWidth  >> 16) & 0xFFFF;
    int16_t  y = (d.gridHeight >> 16) & 0xFFFF;
    uint32_t w = (d.gridWidth  & 0xFFFF);
    uint32_t h = (d.gridHeight & 0xFFFF);
    // clang-format on

    using namespace std::placeholders;
    auto func = std::bind(&MdiChild::getNewGridItem, this, _1, _2, _3);

    if (w > _data.gridWidth) {
      gridLayout->addColumn(w - _data.gridWidth, func);
      if (x != 0) gridLayout->shiftColumn(x, func);
    } else if (w < _data.gridWidth) {
      if (x != 0) gridLayout->shiftColumn(x, func);
      gridLayout->removeColumn(_data.gridWidth-w);
    } else if (x != 0) {
      gridLayout->shiftColumn(x, func);
    }

    if (h > _data.gridHeight) {
      gridLayout->addRow(h - _data.gridHeight, func);
      if (y != 0) gridLayout->shiftRow(y, func);
    } else if (h < _data.gridHeight) {
      if (y != 0) gridLayout->shiftRow(y, func);
      gridLayout->removeRow(_data.gridHeight-h);
    } else if (y != 0) {
      gridLayout->shiftRow(y, func);
    }

    _data = d;
    _data.gridWidth &= 0xFFFF;
    _data.gridHeight &= 0xFFFF;
    setModified(true);
  }
  if (update) updateShoebox();
}

void MdiChild::updateShoebox() {
  if (*_data.backFile != 0) {
    mFileWatcher.addPath(_data.backFile);
    QDomElement domElement = getDomDocument(_data.backFile).documentElement();

    shoeboxList.clear();
    parseDom(domElement.firstChildElement("background"), shoeboxList);
    shoeboxList.sort([](const auto& a, const auto& b) {return a.z < b.z;});

    overlayList.clear();
    parseDom(domElement.firstChildElement("foreground"), overlayList);
    overlayList.sort([](const auto &a, const auto &b) { return a.z < b.z; });

    QSize gridSize{(int)(_data.gridWidth << 4), (int)(_data.gridHeight << 4)};
    overlay->updateShoebox(overlayList, gridSize);

    repaint();
  }
}

void MdiChild::updateGrid(const QMap<TkLayer, bool> &visibility) {
  for (QObject *widget : gridWidget->children()) {
    TkGridItem *item = qobject_cast<TkGridItem *>(widget);
    if (item != nullptr) item->updateLayerVisibility(visibility);
  }
}

QDomDocument MdiChild::getDomDocument(const QString &fileName) {
  QDomDocument doc("shoebox");

  static const QString applicationPath = QCoreApplication::applicationDirPath()+'/';
  QFile file(applicationPath%fileName);
  if (!file.open(QIODevice::ReadOnly)) {
    QMessageBox::warning(
        this, tr("Toki Level Editor"),
        tr("Cannot open file %1:\n%2.").arg(fileName, file.errorString()));
  } else if (!doc.setContent(&file)) {
    QMessageBox::warning(this, tr("Toki Level Editor"),
                         tr("Cannot read file Dom %1.").arg(fileName));
  }
  file.close();
  return doc;
}

void MdiChild::parseDom(const QDomElement &docElem, std::list<shoeboxData>& list) {
  if (docElem.tagName() == "plane") {
    QImage image(docElem.attribute("texture"));
    image = image.scaled(docElem.attribute("width").toInt(),
                         docElem.attribute("height").toInt(),
                         Qt::IgnoreAspectRatio, Qt::SmoothTransformation)
                 .transformed(QTransform().rotate(-docElem.attribute("rotation").toDouble()),
                             Qt::SmoothTransformation);

    list.push_back(shoeboxData{
        docElem.attribute("x").toInt(), docElem.attribute("y").toInt(),
        docElem.attribute("z").toInt(), std::move(image)});
    return;
  }

  QDomNode n = docElem.firstChild();
  while (!n.isNull()) {
    QDomElement e = n.toElement();  // try to convert the node to an element.
    if (!e.isNull()) {
      parseDom(e, list);
    }
    n = n.nextSibling();
  }
}

void MdiChild::buildGrid() {
  ulong nbTiles = _data.getTileNumber();
  for (ulong i = 0U; i < nbTiles; ++i) {
    uint32_t col = i % _data.gridWidth;
    uint32_t row = i / _data.gridWidth;
    TkGridItem *item = getNewGridItem(row, col, mdiChildId);
    gridLayout->addWidget(item, row, col);
  }
  gridLayout->refreshItemCount();
  parentWidget()->layout()->setSizeConstraint(QLayout::SetMinimumSize);
}

TkGridItem *MdiChild::getNewGridItem(uint32_t row, uint32_t col, uint32_t mdiId) {
  TkGridItem *item = new TkGridItem(this);
  item->setProperty("row", row);
  item->setProperty("col", col);
  item->setProperty("mdiId", mdiId);

  connect(item, &TkLabel::mouseButtonEvent, this, &MdiChild::mouseButtonEvent);
  return item;
}

void MdiChild::mouseButtonEvent(QWidget *w, QMouseEvent *ev) {
  switch (ev->type()) {
    // Press
    case QEvent::MouseButtonPress: {
      emit itemClicked(static_cast<TkGridItem *>(w), ev->button());
      if (ev->button() == Qt::LeftButton) _isLeftMouseButtonPressed = true;
      if (ev->button() == Qt::RightButton) _isRightMouseButtonPressed = true;
      break;
    }
    // Release
    case QEvent::MouseButtonRelease: {
      if (ev->button() == Qt::LeftButton) _isLeftMouseButtonPressed = false;
      if (ev->button() == Qt::RightButton) _isRightMouseButtonPressed = false;
      break;
    }
    // Move
    case QEvent::MouseMove: {
      static QPoint lastCoord{-1, -1};

      if (_isLeftMouseButtonPressed || _isRightMouseButtonPressed) {
        QWidget *widget = qApp->widgetAt(QCursor::pos());
        if (widget != nullptr && widget->property("mdiId").toUInt() == mdiChildId) {
          QVariant colVar(widget->property("col"));
          QVariant rowVar(widget->property("row"));
          if (colVar.isValid() && rowVar.isValid()) {
            int curCol = colVar.toInt();
            int curRow = rowVar.toInt();
            if (lastCoord != QPoint{curCol, curRow}) {
              lastCoord = {curCol, curRow};
              // clang-format off
              emit itemClicked(static_cast<TkGridItem *>(widget),
                               (_isLeftMouseButtonPressed)  ? Qt::LeftButton  :
                               (_isRightMouseButtonPressed) ? Qt::RightButton :
                                                              Qt::NoButton);
              // clang-format on
            }
          }
        }
      } else {
        // Reset the last coordinates
        lastCoord = {-1, -1};
      }
      break;
    }
    default:
      break;
  }
}

void MdiChild::newFile()
{
  buildGrid();

  static int sequenceNumber = 1;

  _isUntitled = true;
  _curFile = tr("level%1.tokilevel").arg(sequenceNumber++);
  setWindowTitle(_curFile + "[*]");

  _data.magicNumber = tkMagicNumber;
  _data.version = tkVersion;
//    connect(document(), &QTextDocument::contentsChanged,
//            this, &MdiChild::documentWasModified);
}

bool MdiChild::loadFile(const QString &fileName)
{
  bool succeed = false;

  do {
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly)) {
      QMessageBox::warning(
          this, tr("Toki Level Editor"),
          tr("Cannot read file %1:\n%2.").arg(fileName, file.errorString()));
      break;
    }

    QGuiApplication::setOverrideCursor(Qt::WaitCursor);

    // Read file header
    if (file.read((char *)&_data, 8) != 8) {
      QMessageBox::warning(this, tr("Toki Level Editor"),
                           tr("Cannot read file %1.").arg(fileName));
      break;
    }

    // clang-format off
    if (_data.magicNumber != tkMagicNumber || _data.version != tkVersion) {
      QMessageBox::warning(this, tr("Toki Level Editor"), tr("File content is incompatible."));
      break;
    }
    if (file.read((char *)&_data.gridWidth, 524) != 524) {
      QMessageBox::warning(this, tr("Toki Level Editor"),
                           tr("Cannot read file %1 content.").arg(fileName));
      break;
    }

    // clang-format off
    if (_data.gridWidth   < tkGridMinWidth  || _data.gridWidth  > tkGridMaxWidth  ||
        _data.gridHeight  < tkGridMinHeight || _data.gridHeight > tkGridMaxHeight) {
      QMessageBox::warning(this, tr("Toki Level Editor"), tr("File content is incompatible."));
      break;
    }
    // clang-format on

    buildGrid();
    ulong nbTiles = _data.getTileNumber();
    uint32_t level[nbTiles];
    if (file.read((char *)&level, nbTiles * sizeof(uint32_t)) != qint64(nbTiles * sizeof(uint32_t))) {
      QMessageBox::warning(this, tr("Toki Level Editor"), tr("File content is corrupted."));
      break;
    }

    for (ulong i = 0ULL; i < nbTiles; ++i) {
      QLayoutItem *item =
          gridLayout->itemAtPosition(i / _data.gridWidth, i % _data.gridWidth);
      item->widget()->setProperty("tile", level[i]);
    }

    updateShoebox();

    setCurrentFile(fileName);
    succeed = true;

    //    connect(document(), &QTextDocument::contentsChanged,
    //            this, &MdiChild::documentWasModified);
  } while (false);
  QGuiApplication::restoreOverrideCursor();
  return succeed;
}

bool MdiChild::save()
{
  if (_isUntitled) {
    return saveAs();
  } else {
    return saveFile(_curFile);
  }
}

bool MdiChild::saveAs()
{
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                  _curFile);
  if (fileName.isEmpty())
    return false;

  return saveFile(fileName);
}

bool MdiChild::saveFile(const QString &fileName)
{
  QString errorMessage;

  QGuiApplication::setOverrideCursor(Qt::WaitCursor);

  do {
    QSaveFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
      // Write file header
      if (file.write((const char *)&_data, sizeof(_data)) != sizeof(_data)) {
        errorMessage = "Cannot write file header";
        break;
      }
      // Write map content
      std::vector<uint32_t> levelMap(_data.getTileNumber(), 0xFF);
      for (ulong i = 0ULL; i < levelMap.size(); ++i) {
        QLayoutItem *item = gridLayout->itemAtPosition(i / _data.gridWidth,
                                                       i % _data.gridWidth);
        levelMap[i] = item->widget()->property("tile").toUInt();
      }
      if (file.write((const char *)levelMap.data(), levelMap.size() << 2) != qint64(levelMap.size() << 2)) {
        errorMessage = "Cannot write map content";
        break;
      }

      if (!file.commit()) {
        errorMessage =
            tr("Cannot write file %1:\n%2.")
                .arg(QDir::toNativeSeparators(fileName), file.errorString());
      }
    } else {
      errorMessage =
          tr("Cannot open file %1 for writing:\n%2.")
              .arg(QDir::toNativeSeparators(fileName), file.errorString());
    }
  } while (false);

  QGuiApplication::restoreOverrideCursor();

  if (!errorMessage.isEmpty()) {
    QMessageBox::warning(this, tr("Toki Level Editor"), errorMessage);
    return false;
  }

  setCurrentFile(fileName);
  return true;
}

void MdiChild::cut() {}

void MdiChild::copy() {}

void MdiChild::paste() {}

bool MdiChild::hasSelection() { return false; }

QString MdiChild::userFriendlyCurrentFile()
{
  return strippedName(_curFile);
}

void MdiChild::closeEvent(QCloseEvent *event)
{
  if (maybeSave()) {
    event->accept();
  } else {
    event->ignore();
  }
}

void MdiChild::documentWasModified()
{
  setWindowModified(isModified());
}

bool MdiChild::maybeSave()
{
  if (!isModified())
    return true;
  const QMessageBox::StandardButton ret
          = QMessageBox::warning(this, tr("Toki Level Editor"),
                                 tr("'%1' has been modified.\n"
                                    "Do you want to save your changes?")
                                 .arg(userFriendlyCurrentFile()),
                                 QMessageBox::Save | QMessageBox::Discard
                                 | QMessageBox::Cancel);
  switch (ret) {
  case QMessageBox::Save:
    return save();
  case QMessageBox::Cancel:
    return false;
  default:
    break;
  }
  return true;
}

void MdiChild::setCurrentFile(const QString &fileName)
{
  _curFile = QFileInfo(fileName).canonicalFilePath();
  _isUntitled = false;
  setModified(false);
  setWindowModified(false);
  setWindowTitle(userFriendlyCurrentFile() + "[*]");
}

QString MdiChild::strippedName(const QString &fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}
