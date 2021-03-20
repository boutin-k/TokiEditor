#include <QtWidgets>
#include <QDomDocument>
#include <QSharedPointer>

#include "mdichild.h"
#include "tkgriditem.h"

MdiChild::MdiChild() {
  setAttribute(Qt::WA_DeleteOnClose);
  setWindowIcon(QIcon(":/images/myappico.ico"));

  setLayout(&_gridLayout);
  _gridLayout.setSizeConstraint(QLayout::SetMinAndMaxSize);
  _gridLayout.setSpacing(0);
}

void MdiChild::paintEvent(QPaintEvent * event) {
  QPainter painter(this);
  for (const auto& pair : wallpaperList)
    painter.drawPixmap(0, 0, *pair.second.get());
  QWidget::paintEvent(event);
}

void MdiChild::updateBackground() {
  if (*_data.backFile != 0) {
    wallpaperList.clear();

    parseDom(getDomDocument(_data.backFile).documentElement());
    std::sort(wallpaperList.begin(), wallpaperList.end(),
              [](QPair<uint32_t, QSharedPointer<QPixmap>> const &x,
                 QPair<uint32_t, QSharedPointer<QPixmap>> const &y) {
                return x.first < y.first;
              });
    repaint();
  }
}

void MdiChild::updateGrid(const QMap<TkLayer, bool> &visibility) {
  for (QObject *widget : this->children()) {
    TkGridItem *item = qobject_cast<TkGridItem *>(widget);
    if (item != nullptr) item->updateLayerVisibility(visibility);
  }
//  GridLayoutUtil::removeColumn(&_gridLayout, 0);
//  GridLayoutUtil::removeColumn(&_gridLayout, 1);
//  GridLayoutUtil::removeColumn(&_gridLayout, 2);
//  GridLayoutUtil::removeColumn(&_gridLayout, 3);
//  GridLayoutUtil::removeColumn(&_gridLayout, 4);
}

QDomDocument MdiChild::getDomDocument(const QString &fileName) {
  QDomDocument doc("wallpaper");
  QFile file(fileName);
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

void MdiChild::parseDom(const QDomElement &docElem) {
  if (docElem.tagName() == "plane") {
    QPixmap pixmap(docElem.attribute("texture"));
//    pixmap.scaledToWidth(docElem.attribute("width").toInt());
//    pixmap.scaledToHeight(docElem.attribute("height").toInt());

    int rotate = docElem.attribute("rotation").toInt();
    int x = docElem.attribute("x").toInt();
    int y = docElem.attribute("y").toInt();
    int z = docElem.attribute("z").toInt();
    qreal scaleX = docElem.attribute("width").toDouble()/pixmap.width();
    qreal scaleY = docElem.attribute("height").toDouble()/pixmap.height();

    QTransform transform;

    transform = transform.translate(x, y).scale(scaleX, scaleY).rotate(rotate);
    QPixmap *transPixmap = new QPixmap(pixmap.transformed(transform, Qt::SmoothTransformation));
    QPair<uint32_t, QSharedPointer<QPixmap>> pair(
        z, QSharedPointer<QPixmap>(transPixmap));
    wallpaperList.append(pair);
    return;
  }

  QDomNode n = docElem.firstChild();
  while (!n.isNull()) {
    QDomElement e = n.toElement();  // try to convert the node to an element.
    if (!e.isNull()) {
      parseDom(e);
    }
    n = n.nextSibling();
  }
}

void MdiChild::buildGrid() {
  ulong nbTiles = _data.getTileNumber();
  for (ulong i = 0U; i < nbTiles; ++i) {
    TkGridItem *label = new TkGridItem(this);
    uint32_t x = i % _data.gridWidth;
    uint32_t y = i / _data.gridWidth;
    label->setProperty("tile", 0xFF);
    label->setProperty("x", x);
    label->setProperty("y", y);
    _gridLayout.addWidget(label, y, x);
    connect(label, &TkLabel::mouseButtonEvent, this,
            &MdiChild::mouseButtonEvent);
    connect(label, &TkLabel::mouseHoverEvent, this,
            &MdiChild::mouseHoverEvent);
  }
  adjustSize();
  layout()->invalidate();
  repaint();
}

void MdiChild::mouseButtonEvent(QWidget *w, QMouseEvent *ev) {
  if (ev->type() == QEvent::MouseButtonPress) {
    emit itemClicked(static_cast<TkGridItem *>(w), ev);
  }
}

void MdiChild::mouseHoverEvent(QWidget *w, QHoverEvent *ev) {
  if (ev->type() == QEvent::HoverEnter ||
      ev->type() == QEvent::HoverLeave) {
    emit itemHovered(static_cast<TkGridItem *>(w), ev);
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
          _gridLayout.itemAtPosition(i / _data.gridWidth, i % _data.gridWidth);
      item->widget()->setProperty("tile", level[i]);
    }

    updateBackground();

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
        QLayoutItem *item = _gridLayout.itemAtPosition(i / _data.gridWidth,
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
