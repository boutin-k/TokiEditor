#include <QtWidgets>
#include <QDomDocument>
#include <QSharedPointer>
#include <QStackedLayout>
#include <QTimer>

#include "mdichild.h"
#include "tkgriditem.h"
#include "tkgridlayout.h"
#include "tkutils.h"

uint32_t MdiChild::sMdiChildCounter{1U};

/**
 * @brief Default constructor
 */
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
  connect(&fileWatcher, &QFileSystemWatcher::fileChanged, this, [this]() {
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

/**
 * @brief Destructor
 */
MdiChild::~MdiChild() { fileWatcher.disconnect();
}

/**
 * @brief Draw the widget
 * @param[in] event Paint event
 */
void MdiChild::paintEvent(QPaintEvent * event) {
  if (shoeboxDisplayed) {
    int w = data.gridWidth << 4;
    int h = data.gridHeight << 4;

    QPainter painter(this);
    for (const auto &item : shoeboxList) {
      int x = item.x + w - (item.i.width() >> 1);
      int y = item.y + h - (item.i.height() >> 1);
      painter.drawImage(x, y, item.i);
    }
  }
  QWidget::paintEvent(event);
}

/**
 * @brief Draw the Overlay widget
 * @param event Paint event
 */
void MdiChild::Overlay::paintEvent(QPaintEvent * event) {
  QPainter painter(this);
  for (const auto &item : overlay) {
    int x = item.x + size.width() - (item.i.width() >> 1);
    int y = item.y + size.height() - (item.i.height() >> 1);
    painter.drawImage(x, y, item.i);
  }
  QWidget::paintEvent(event);
}

/**
 * @brief Update the mdi data
 * @param[in] d The new data
 */
void MdiChild::setData(const levelData &d) {
  bool update = std::strcmp(d.backFile, data.backFile);
  if (data != d) {
    // clang-format off
    int16_t  x = (d.gridWidth  >> 16) & 0xFFFF;
    int16_t  y = (d.gridHeight >> 16) & 0xFFFF;
    uint32_t w = (d.gridWidth  & 0xFFFF);
    uint32_t h = (d.gridHeight & 0xFFFF);
    // clang-format on

    using namespace std::placeholders;
    auto func = std::bind(&MdiChild::getNewGridItem, this, _1, _2, _3);

    if (w > data.gridWidth) {
      gridLayout->addColumn(w - data.gridWidth, func);
      if (x != 0) gridLayout->shiftColumn(x, func);
    } else if (w < data.gridWidth) {
      if (x != 0) gridLayout->shiftColumn(x, func);
      gridLayout->removeColumn(data.gridWidth-w);
    } else if (x != 0) {
      gridLayout->shiftColumn(x, func);
    }

    if (h > data.gridHeight) {
      gridLayout->addRow(h - data.gridHeight, func);
      if (y != 0) gridLayout->shiftRow(y, func);
    } else if (h < data.gridHeight) {
      if (y != 0) gridLayout->shiftRow(y, func);
      gridLayout->removeRow(data.gridHeight-h);
    } else if (y != 0) {
      gridLayout->shiftRow(y, func);
    }

    data = d;
    data.gridWidth &= 0xFFFF;
    data.gridHeight &= 0xFFFF;
    setModified(true);
  }
  if (update) updateShoebox();
}

/**
 * @brief Update the mdi shoebox
 */
void MdiChild::updateShoebox() {
  if (*data.backFile != 0) {
    fileWatcher.addPath(data.backFile);
    QDomElement domElement = getDomDocument(data.backFile).documentElement();

    shoeboxList.clear();
    parseDom(domElement.firstChildElement("background"), shoeboxList);
    shoeboxList.sort([](const auto& a, const auto& b) {return a.z < b.z;});

    overlayList.clear();
    parseDom(domElement.firstChildElement("foreground"), overlayList);
    overlayList.sort([](const auto &a, const auto &b) { return a.z < b.z; });

    QSize gridSize{(int)(data.gridWidth << 4), (int)(data.gridHeight << 4)};
    overlay->updateShoebox(overlayList, gridSize);

    repaint();
  }
}

/**
 * @brief Update the mdi grid
 * @param visibility The layers visibility
 */
void MdiChild::updateGrid(const QMap<TkLayer, bool> &visibility) {
  for (QObject *widget : gridWidget->children()) {
    TkGridItem *item = qobject_cast<TkGridItem *>(widget);
    if (item != nullptr) item->updateLayerVisibility(visibility);
  }
}

/**
 * @brief Open file and extract DOM
 * @param[in] fileName The file containing the DOM
 * @return The DOM document
 */
QDomDocument MdiChild::getDomDocument(const QString &fileName) {
  QDomDocument doc("shoebox");

  QFile file(absolutePath(fileName));
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

/**
 * @brief Parse the DOM content
 * @param[in] docElem The dom content
 * @param[in,out] list The list to populate
 */
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

/**
 * @brief Build the grid
 */
void MdiChild::buildGrid() {
  ulong nbTiles = data.getTileNumber();
  for (ulong i = 0U; i < nbTiles; ++i) {
    uint32_t col = i % data.gridWidth;
    uint32_t row = i / data.gridWidth;
    TkGridItem *item = getNewGridItem(row, col, mdiChildId);
    gridLayout->addWidget(item, row, col);
  }
  gridLayout->refreshItemCount();
  parentWidget()->layout()->setSizeConstraint(QLayout::SetMinimumSize);
}

/**
 * @brief MdiChild::getNewGridItem
 * @param[in] row The row position of the item
 * @param[in] col The column position of the item
 * @param[in] mdiId The Id of the parent widget
 * @return The grid item pointer
 */
TkGridItem *MdiChild::getNewGridItem(uint32_t row, uint32_t col, uint32_t mdiId) {
  TkGridItem *item = new TkGridItem(this);
  item->setProperty("row", row);
  item->setProperty("col", col);
  item->setProperty("mdiId", mdiId);

  connect(item, &TkLabel::mouseButtonEvent, this, &MdiChild::mouseButtonEvent);
  return item;
}

/**
 * @brief Listener called when mouse button event is trigged
 * @param[in,out] w The widget related to the event
 * @param[in,out] ev The mouse event detail
 */
void MdiChild::mouseButtonEvent(QWidget *w, QMouseEvent *ev) {
  switch (ev->type()) {
    // Press
    case QEvent::MouseButtonPress: {
      emit itemClicked(static_cast<TkGridItem *>(w), ev->button());
      if (ev->button() == Qt::LeftButton) leftMouseButtonPressed = true;
      if (ev->button() == Qt::RightButton) rightMouseButtonPressed = true;
      break;
    }
    // Release
    case QEvent::MouseButtonRelease: {
      if (ev->button() == Qt::LeftButton) leftMouseButtonPressed = false;
      if (ev->button() == Qt::RightButton) rightMouseButtonPressed = false;
      break;
    }
    // Move
    case QEvent::MouseMove: {
      static QPoint lastCoord{-1, -1};

      if (leftMouseButtonPressed || rightMouseButtonPressed) {
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
                               (leftMouseButtonPressed)  ? Qt::LeftButton  :
                               (rightMouseButtonPressed) ? Qt::RightButton :
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

/**
 * @brief Create a new mdi window
 */
void MdiChild::newFile()
{
  buildGrid();

  static int sequenceNumber = 1;

  untitled = true;
  curFile = tr("level%1.tokilevel").arg(sequenceNumber++);
  setWindowTitle(curFile + "[*]");

  data.magicNumber = tkMagicNumber;
  data.version = tkVersion;
//    connect(document(), &QTextDocument::contentsChanged,
//            this, &MdiChild::documentWasModified);
}

/**
 * @brief Load a tokilevel file in mdi window
 * @param[in] fileName The name of the file to load
 * @return \c true if the file is correctly loaded, otherwise \c false
 */
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
    if (file.read((char *)&data, 8) != 8) {
      QMessageBox::warning(this, tr("Toki Level Editor"),
                           tr("Cannot read file %1.").arg(fileName));
      break;
    }

    // clang-format off
    if (data.magicNumber != tkMagicNumber || data.version != tkVersion) {
      QMessageBox::warning(this, tr("Toki Level Editor"), tr("File content is incompatible."));
      break;
    }
    if (file.read((char *)&data.gridWidth, 524) != 524) {
      QMessageBox::warning(this, tr("Toki Level Editor"),
                           tr("Cannot read file %1 content.").arg(fileName));
      break;
    }

    // clang-format off
    if (data.gridWidth   < tkGridMinWidth  || data.gridWidth  > tkGridMaxWidth  ||
        data.gridHeight  < tkGridMinHeight || data.gridHeight > tkGridMaxHeight) {
      QMessageBox::warning(this, tr("Toki Level Editor"), tr("File content is incompatible."));
      break;
    }
    // clang-format on

    buildGrid();
    ulong nbTiles = data.getTileNumber();
    uint32_t level[nbTiles];
    if (file.read((char *)&level, nbTiles * sizeof(uint32_t)) != qint64(nbTiles * sizeof(uint32_t))) {
      QMessageBox::warning(this, tr("Toki Level Editor"), tr("File content is corrupted."));
      break;
    }

    for (ulong i = 0ULL; i < nbTiles; ++i) {
      QLayoutItem *item =
          gridLayout->itemAtPosition(i / data.gridWidth, i % data.gridWidth);
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

/**
 * @brief Save grid
 * @return \c true if the grid is correctly saved, otherwise \c false
 */
bool MdiChild::save()
{
  if (untitled) {
    return saveAs();
  } else {
    return saveFile(curFile);
  }
}

/**
 * @brief Save the grid through a new name
 * @return \c true if the grid is correctly saved, otherwise \c false
 */
bool MdiChild::saveAs()
{
  QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                  curFile);
  if (fileName.isEmpty())
    return false;

  return saveFile(fileName);
}

/**
 * @brief Save the grid into the file name
 * @param[in] fileName The name of the file to load
 * @return \c true if the grid is correctly saved, otherwise \c false
 */
bool MdiChild::saveFile(const QString &fileName)
{
  QString errorMessage;

  QGuiApplication::setOverrideCursor(Qt::WaitCursor);

  do {
    QSaveFile file(fileName);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
      // Write file header
      if (file.write((const char *)&data, sizeof(data)) != sizeof(data)) {
        errorMessage = "Cannot write file header";
        break;
      }
      // Write map content
      std::vector<uint32_t> levelMap(data.getTileNumber(), 0xFF);
      for (ulong i = 0ULL; i < levelMap.size(); ++i) {
        QLayoutItem *item = gridLayout->itemAtPosition(i / data.gridWidth,
                                                       i % data.gridWidth);
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

/**
 * @brief Listener called when user try to close the mdi window
 * @param[in] event The close event
 */
void MdiChild::closeEvent(QCloseEvent *event)
{
  if (maybeSave()) {
    event->accept();
  } else {
    event->ignore();
  }
}

/**
 * @brief Update the modified state
 */
void MdiChild::documentWasModified()
{
  setWindowModified(isModified());
}

/**
 * @brief Before closing the window ask to save
 * @return \c true if saved, otherwise \c false
 */
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

/**
 * @brief Update the mdi window title
 * @param[in] fileName The path of the opened file
 */
void MdiChild::setCurrentFile(const QString &fileName)
{
  curFile = QFileInfo(fileName).canonicalFilePath();
  untitled = false;
  setModified(false);
  setWindowModified(false);
  setWindowTitle(userFriendlyCurrentFile() + "[*]");
}

/**
 * @brief Display the name of the file without the path
 * @return The name of the file
 */
QString MdiChild::userFriendlyCurrentFile()
{
  return strippedName(curFile);
}

/**
 * @brief stripped the name of the file
 * @param[in] fullFileName The file path
 * @return The name of the file
 */
QString MdiChild::strippedName(const QString &fullFileName)
{
  return QFileInfo(fullFileName).fileName();
}
