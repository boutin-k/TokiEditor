#include <QtWidgets>
#include <QMap>

#include "flowlayout.h"
#include "mainwindow.h"
#include "mdichild.h"
#include "tklabel.h"

#include "settingsdialog.h"

#define DockItemDefaultStylesheet  \
    "color:white;"                 \
    "border-color:white;"          \
    "border-width:2px;"            \
    "border-style:solid"

#define DockItemSelectedStylesheet \
    "color:white;"                 \
    "border-color:white;"          \
    "border-width:2px;"            \
    "border-style:dashed"

MainWindow::MainWindow()
    : mdiArea(new QMdiArea)
{
  setWindowIcon(QIcon(":/images/myappico.ico"));

  mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setCentralWidget(mdiArea);
  connect(mdiArea, &QMdiArea::subWindowActivated,
          this, &MainWindow::updateMenus);

  createActions();
  createDockWindows();
  createStatusBar();
  updateMenus();

  readSettings();

  setWindowTitle(tr("Toki Level Editor"));
  setUnifiedTitleAndToolBarOnMac(true);
}

void MainWindow::closeEvent(QCloseEvent *event)
{

  mdiArea->closeAllSubWindows();
  if (mdiArea->currentSubWindow()) {
    event->ignore();
  } else {
    writeSettings();
    event->accept();
  }
}

void MainWindow::newFile()
{
  SettingsDialog *settingsDialog = new SettingsDialog(this);
  connect(settingsDialog, &SettingsDialog::finished,
          [this, settingsDialog](int result) {
            if (result == QDialog::Accepted) {
              MdiChild *child = createMdiChild();
              child->setData(settingsDialog->getData());
              child->newFile();
              child->show();

              settingsDialog->disconnect();
              settingsDialog->deleteLater();
            }
  });
  dockItemUnselect();
  settingsDialog->show();
}

void MainWindow::open()
{
  const QString fileName = QFileDialog::getOpenFileName(this, tr("Open Level"), "~", "*.tokilevel");
  if (!fileName.isEmpty())
    openFile(fileName);
}

bool MainWindow::openFile(const QString &fileName)
{
  if (QMdiSubWindow *existing = findMdiChild(fileName)) {
    mdiArea->setActiveSubWindow(existing);
    return true;
  }
  const bool succeeded = loadFile(fileName);
  if (succeeded)
    statusBar()->showMessage(tr("File loaded"), 2000);
  return succeeded;
}

bool MainWindow::loadFile(const QString &fileName)
{
  dockItemUnselect();
  MdiChild *child = createMdiChild();
  const bool succeeded = child->loadFile(fileName);
  if (succeeded) {
    QLayout *childLayout = child->findChild<QWidget *>("Grid")->layout();

    for (int idx = 0; idx < childLayout->count(); ++idx) {
      TkGridItem *item =
          static_cast<TkGridItem *>(childLayout->itemAt(idx)->widget());
      if (item != nullptr) {
        uint32_t tile = item->property("tile").toUInt();
        if (tile != 0xFF) {
          uint32_t groundTile = tile & 0xFF;
          if (groundTile < 0xFD) {
            item->updatePixmapLayer(
                dockItems[groundTile]->pixmap(Qt::ReturnByValue), 0, groundTile,
                true);
          } else if (groundTile == 0xFD) {
            item->updatePixmapLayer(QPixmap(":/images/startIcon.png"), 0,
                                    groundTile, true);
            child->setTokiTile(item);
          } else if (groundTile == 0xFE) {
            item->updatePixmapLayer(QPixmap(":/images/egg.png"), 0, groundTile, true);
          }

          uint32_t backTile = (tile & 0xFF00) >> 8;
          if (backTile)
            item->updatePixmapLayer(
                dockItems[backTile]->pixmap(Qt::ReturnByValue), 1, backTile, true);

          uint32_t frontTile = (tile & 0xFF0000) >> 16;
          if (frontTile)
            item->updatePixmapLayer(
                dockItems[frontTile]->pixmap(Qt::ReturnByValue), 2, frontTile, true);
        }
      }
    }
    child->show();
  }
  else {
    child->close();
  }
  MainWindow::prependToRecentFiles(fileName);
  return succeeded;
}

static inline QString recentFilesKey() { return QStringLiteral("recentFileList"); }
static inline QString fileKey() { return QStringLiteral("file"); }

static QStringList readRecentFiles(QSettings &settings)
{
  QStringList result;
  const int count = settings.beginReadArray(recentFilesKey());
  for (int i = 0; i < count; ++i) {
    settings.setArrayIndex(i);
    result.append(settings.value(fileKey()).toString());
  }
  settings.endArray();
  return result;
}

static void writeRecentFiles(const QStringList &files, QSettings &settings)
{
  const int count = files.size();
  settings.beginWriteArray(recentFilesKey());
  for (int i = 0; i < count; ++i) {
    settings.setArrayIndex(i);
    settings.setValue(fileKey(), files.at(i));
  }
  settings.endArray();
}

bool MainWindow::hasRecentFiles()
{
  QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
  const int count = settings.beginReadArray(recentFilesKey());
  settings.endArray();
  return count > 0;
}

void MainWindow::prependToRecentFiles(const QString &fileName)
{
  QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

  const QStringList oldRecentFiles = readRecentFiles(settings);
  QStringList recentFiles = oldRecentFiles;
  recentFiles.removeAll(fileName);
  recentFiles.prepend(fileName);
  if (oldRecentFiles != recentFiles)
    writeRecentFiles(recentFiles, settings);

  setRecentFilesVisible(!recentFiles.isEmpty());
}

void MainWindow::setRecentFilesVisible(bool visible)
{
  recentFileSubMenuAct->setVisible(visible);
  recentFileSeparator->setVisible(visible);
}

void MainWindow::updateRecentFileActions()
{
  QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());

  const QStringList recentFiles = readRecentFiles(settings);
  const int count = qMin(int(MaxRecentFiles), recentFiles.size());
  int i = 0;
  for ( ; i < count; ++i) {
    const QString fileName = QFileInfo(recentFiles.at(i)).fileName();
    recentFileActs[i]->setText(tr("&%1 %2").arg(i + 1).arg(fileName));
    recentFileActs[i]->setData(recentFiles.at(i));
    recentFileActs[i]->setVisible(true);
  }
  for ( ; i < MaxRecentFiles; ++i)
    recentFileActs[i]->setVisible(false);
}

void MainWindow::openRecentFile()
{
  if (const QAction *action = qobject_cast<const QAction *>(sender()))
    openFile(action->data().toString());
}

void MainWindow::save()
{
  if (activeMdiChild() && activeMdiChild()->save())
    statusBar()->showMessage(tr("File saved"), 2000);
}

void MainWindow::saveAs()
{
  MdiChild *child = activeMdiChild();
  if (child && child->saveAs()) {
    statusBar()->showMessage(tr("File saved"), 2000);
    MainWindow::prependToRecentFiles(child->currentFile());
  }
}

#ifndef QT_NO_CLIPBOARD
void MainWindow::cut()
{
  if (activeMdiChild())
    activeMdiChild()->cut();
}

void MainWindow::copy()
{
  if (activeMdiChild())
    activeMdiChild()->copy();
}

void MainWindow::paste()
{
  if (activeMdiChild())
    activeMdiChild()->paste();
}
#endif

void MainWindow::about()
{
  QMessageBox::about(this, tr("About Toki Level Editor"),
                     tr("The <b>Toki Level Editor</b> is used to create and update levels "
                     "for <b>Toki</b> platform game."));
}

void MainWindow::updateMenus()
{
  bool hasMdiChild = (activeMdiChild() != nullptr);
  saveAct->setEnabled(hasMdiChild);
  saveAsAct->setEnabled(hasMdiChild);
#ifndef QT_NO_CLIPBOARD
  pasteAct->setEnabled(hasMdiChild);
#endif
  groundAct->setEnabled(hasMdiChild);
  foregroundAct->setEnabled(hasMdiChild);
  backgroundAct->setEnabled(hasMdiChild);
  shoeboxAct->setEnabled(hasMdiChild);
  settingsAct->setEnabled(hasMdiChild);
  closeAct->setEnabled(hasMdiChild);
  closeAllAct->setEnabled(hasMdiChild);
  tileAct->setEnabled(hasMdiChild);
  cascadeAct->setEnabled(hasMdiChild);
  nextAct->setEnabled(hasMdiChild);
  previousAct->setEnabled(hasMdiChild);
  windowMenuSeparatorAct->setVisible(hasMdiChild);

#ifndef QT_NO_CLIPBOARD
  bool hasSelection = (activeMdiChild() &&
                       activeMdiChild()->hasSelection());
  cutAct->setEnabled(hasSelection);
  copyAct->setEnabled(hasSelection);
#endif
}

void MainWindow::updateWindowMenu()
{
  windowMenu->clear();
  windowMenu->addAction(settingsAct);
  windowMenu->addSeparator();
  windowMenu->addAction(closeAct);
  windowMenu->addAction(closeAllAct);
  windowMenu->addSeparator();
  windowMenu->addAction(tileAct);
  windowMenu->addAction(cascadeAct);
  windowMenu->addSeparator();
  windowMenu->addAction(nextAct);
  windowMenu->addAction(previousAct);
  windowMenu->addAction(windowMenuSeparatorAct);

  QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
  windowMenuSeparatorAct->setVisible(!windows.isEmpty());

  for (int i = 0; i < windows.size(); ++i) {
    QMdiSubWindow *mdiSubWindow = windows.at(i);
    MdiChild *child = qobject_cast<MdiChild *>(mdiSubWindow->widget());
    if (child == nullptr) return;

    QString text;
    if (i < 9) {
      text = tr("&%1 %2").arg(i + 1)
                         .arg(child->userFriendlyCurrentFile());
    } else {
      text = tr("%1 %2").arg(i + 1)
                        .arg(child->userFriendlyCurrentFile());
    }
    QAction *action = windowMenu->addAction(text, mdiSubWindow, [this, mdiSubWindow]() {
      mdiArea->setActiveSubWindow(mdiSubWindow);
    });
    action->setCheckable(true);
    action ->setChecked(child == activeMdiChild());
  }
}

void MainWindow::mdiChildItemClicked(TkGridItem *item, QMouseEvent *ev) {
  int index = dockToolbox->currentIndex();
  if (index < 0) return;

  // Entities are on the ground layer
  if (index == 3) index = 0;

  switch (ev->button()) {
    case Qt::LeftButton: {
      if (_selectedDockTile != nullptr) {
        uint32_t tile = _selectedDockTile->property("id").toUInt();

        if (tile == 0xFD) {
          TkGridItem *toki = activeMdiChild()->getTokiTile();
          if (toki != nullptr) {
            if (toki != item) {
              toki->clearPixmapLayer(index);
            } else
              break;
          }
          activeMdiChild()->setTokiTile(item);
        }

        if (item->updatePixmapLayer(_selectedDockPixmap, index, tile))
          activeMdiChild()->setModified(true);
      }
      break;
    }
    case Qt::RightButton: {
      if ((index == 0) && (item->property("item").toUInt()&0xFF) == 0xFD)
        activeMdiChild()->setTokiTile(nullptr);

      if (item->clearPixmapLayer(index))
        activeMdiChild()->setModified(true);
      break;
    }
    default:
      break;
  }
}

void MainWindow::mdiChildItemHovered(TkGridItem * /*item*/,
                                     QHoverEvent * /*ev*/) {
//  static QHash<TkGridItem *, QPixmap> list;

//  if (ev->type() == QEvent::HoverEnter) {
//    QPixmap pixmap = item->pixmap(Qt::ReturnByValue);
//    QPixmap tempPixmap = pixmap;
//    list.insert(item, tempPixmap);

//    QPainter painter;
//    painter.begin(&tempPixmap);
//    painter.fillRect(pixmap.rect(), QColor(127, 127, 127, 127));
//    painter.end();

//    item->setPixmap(tempPixmap);
//  }


//  if (ev->type() == QEvent::HoverLeave) {
//    item->setPixmap(*list.find(item));
//  }
}

MdiChild *MainWindow::createMdiChild()
{
  MdiChild *child = new MdiChild;
  mdiArea->addSubWindow(child);
  connect(child, &MdiChild::itemClicked, this,
          &MainWindow::mdiChildItemClicked);
  connect(child, &MdiChild::itemHovered, this,
          &MainWindow::mdiChildItemHovered);

//#ifndef QT_NO_CLIPBOARD
//    connect(child, &QTextEdit::copyAvailable, cutAct, &QAction::setEnabled);
//    connect(child, &QTextEdit::copyAvailable, copyAct, &QAction::setEnabled);
//#endif
  return child;
}

void MainWindow::createDockWindows()
{
  QDockWidget *dock = new QDockWidget(tr("Forest Tiles"), this);
  dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  dock->setFeatures(QDockWidget::DockWidgetMovable |
                    QDockWidget::DockWidgetFloatable);
  dock->setMinimumWidth(113); // 267
  dock->setMaximumWidth(281);

  QSizePolicy policy(dock->sizePolicy());
  policy.setHorizontalPolicy(QSizePolicy::Maximum);
  dock->setSizePolicy(policy);

  QScrollArea *scrollArea = new QScrollArea(dock);
  scrollArea->setBackgroundRole(QPalette::Dark);
  scrollArea->setWidgetResizable(true);

  dock->setWidget(scrollArea);
  addDockWidget(Qt::LeftDockWidgetArea, dock);

  dockToolbox = new QToolBox(scrollArea);
  FlowLayout* groundLayout = new FlowLayout(new QWidget(dockToolbox));
  dockToolbox->addItem(groundLayout->parentWidget(), tr("ground"));

  FlowLayout* foregroundtLayout = new FlowLayout(new QWidget(dockToolbox));
  dockToolbox->addItem(foregroundtLayout->parentWidget(), tr("foreground"));

  FlowLayout* backgroundLayout = new FlowLayout(new QWidget(dockToolbox));
  dockToolbox->addItem(backgroundLayout->parentWidget(), tr("background"));

  FlowLayout *entityLayout = new FlowLayout(new QWidget(dockToolbox));
  dockToolbox->addItem(entityLayout->parentWidget(), tr("entity"));

  connect(dockToolbox, &QToolBox::currentChanged, this, &MainWindow::dockItemUnselect);

  scrollArea->setWidget(dockToolbox);

  QPixmap *pixmap = new QPixmap("textures/tiles/forest_tiles.png");
  int spritePerLine = pixmap->width() >> 5;
  //  int nbTile = spritePerLine * (pixmap->height() >> 5);
  int nbTile = 108;

  for (int i = 0; i < nbTile; ++i) {
    TkLabel *label = new TkLabel(scrollArea);
    label->setPixmap(pixmap->copy((i % spritePerLine) << 5,
                                  (i / spritePerLine) << 5, 32, 32));
    label->setProperty("id", i);
    label->setFixedSize(36, 36);
    label->setFrameShape(QFrame::Box);
    label->setStyleSheet(DockItemDefaultStylesheet);
    connect(label, &TkLabel::mouseButtonEvent, this,
            &MainWindow::dockItemClicked);

    if (i < 64)
      groundLayout->addWidget(label);
    else if (i < 80)
      backgroundLayout->addWidget(label);
    else if (i < 108)
      foregroundtLayout->addWidget(label);

    dockItems.append(label);
  }

  TkLabel *startLabel = new TkLabel(entityLayout->parentWidget());
  startLabel->setPixmap(QPixmap(":/images/startIcon.png"));
  startLabel->setProperty("id", 0xFD);
  startLabel->setFixedSize(36, 36);
  startLabel->setFrameShape(QFrame::Box);
  startLabel->setStyleSheet(DockItemDefaultStylesheet);
  connect(startLabel, &TkLabel::mouseButtonEvent, this,
          &MainWindow::dockItemClicked);
  entityLayout->addWidget(startLabel);

  TkLabel *eggLabel = new TkLabel(entityLayout->parentWidget());
  eggLabel->setPixmap(QPixmap(":/images/egg.png"));
  eggLabel->setProperty("id", 0xFE);
  eggLabel->setFixedSize(36, 36);
  eggLabel->setFrameShape(QFrame::Box);
  eggLabel->setStyleSheet(DockItemDefaultStylesheet);
  connect(eggLabel, &TkLabel::mouseButtonEvent, this,
          &MainWindow::dockItemClicked);
  entityLayout->addWidget(eggLabel);
}

void MainWindow::dockItemUnselect() {
  if (nullptr != _selectedDockTile) {
    _selectedDockTile->setStyleSheet(DockItemDefaultStylesheet);
    ((QLabel *)_selectedDockTile)->setPixmap(_selectedDockPixmap);
    _selectedDockTile = nullptr;
  }
}

void MainWindow::dockItemClicked(TkLabel *tile, QMouseEvent *event) {
  if (tile == _selectedDockTile) return;

  if (event->type() == QEvent::MouseButtonPress) {
    // Restore previous button
    dockItemUnselect();

    _selectedDockTile = tile;
    tile->setStyleSheet(DockItemSelectedStylesheet);

    _selectedDockPixmap = tile->pixmap(Qt::ReturnByValue);
    QPixmap tempPixmap = _selectedDockPixmap;

    QPainter painter;
    painter.begin(&tempPixmap);
    painter.fillRect(_selectedDockPixmap.rect(), QColor(127, 127, 127, 127));
    painter.end();

    tile->setPixmap(tempPixmap);
  }
}


void MainWindow::createActions()
{
  // FILE MENU
  QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
  QToolBar *fileToolBar = addToolBar(tr("File"));
  {
    const QIcon newIcon =
        QIcon::fromTheme("document-new", QIcon(":/images/new.png"));
    newAct = new QAction(newIcon, tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);
    fileMenu->addAction(newAct);
    fileToolBar->addAction(newAct);

    const QIcon openIcon =
        QIcon::fromTheme("document-open", QIcon(":/images/open.png"));
    QAction *openAct = new QAction(openIcon, tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, &QAction::triggered, this, &MainWindow::open);
    fileMenu->addAction(openAct);
    fileToolBar->addAction(openAct);

    const QIcon saveIcon =
        QIcon::fromTheme("document-save", QIcon(":/images/save.png"));
    saveAct = new QAction(saveIcon, tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);
    fileToolBar->addAction(saveAct);

    const QIcon saveAsIcon = QIcon::fromTheme("document-save-as");
    saveAsAct = new QAction(saveAsIcon, tr("Save &As..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("Save the document under a new name"));
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAs);
    fileMenu->addAction(saveAsAct);

    fileMenu->addSeparator();

    QMenu *recentMenu = fileMenu->addMenu(tr("Recent opened files"));
    connect(recentMenu, &QMenu::aboutToShow, this,
            &MainWindow::updateRecentFileActions);
    recentFileSubMenuAct = recentMenu->menuAction();

    for (int i = 0; i < MaxRecentFiles; ++i) {
      recentFileActs[i] =
          recentMenu->addAction(QString(), this, &MainWindow::openRecentFile);
      recentFileActs[i]->setVisible(false);
    }

    recentFileSeparator = fileMenu->addSeparator();

    setRecentFilesVisible(MainWindow::hasRecentFiles());

    fileMenu->addSeparator();

    const QIcon exitIcon = QIcon::fromTheme("application-exit");
    QAction *exitAct = fileMenu->addAction(exitIcon, tr("E&xit"), qApp,
                                           &QApplication::closeAllWindows);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    fileMenu->addAction(exitAct);
  }

  // SELECTION MENU
  QToolBar *selectionToolBar = addToolBar(tr("Selection"));
  {
    const QIcon foregroundIcon = QIcon(":/images/selection/foreground.png");
    foregroundAct = new QAction(foregroundIcon, tr("&Foreground"), this);
    connect(foregroundAct, &QAction::triggered, this, &MainWindow::updateMdiChildren);
    foregroundAct->setCheckable(true);
    foregroundAct->setChecked(true);
    selectionToolBar->addAction(foregroundAct);

    const QIcon groundIcon = QIcon(":/images/selection/ground.png");
    groundAct = new QAction(groundIcon, tr("&Ground"), this);
    connect(groundAct, &QAction::triggered, this, &MainWindow::updateMdiChildren);
    groundAct->setCheckable(true);
    groundAct->setChecked(true);
    selectionToolBar->addAction(groundAct);

    const QIcon backgroundIcon = QIcon(":/images/selection/background.png");
    backgroundAct = new QAction(backgroundIcon, tr("&Background"), this);
    connect(backgroundAct, &QAction::triggered, this, &MainWindow::updateMdiChildren);
    backgroundAct->setCheckable(true);
    backgroundAct->setChecked(true);
    selectionToolBar->addAction(backgroundAct);

    const QIcon shoeboxIcon = QIcon(":/images/selection/shoebox.png");
    shoeboxAct = new QAction(shoeboxIcon, tr("&Shoebox"), this);
    connect(shoeboxAct, &QAction::triggered, this, &MainWindow::updateMdiShoebox);
    shoeboxAct->setCheckable(true);
    shoeboxAct->setChecked(true);
    selectionToolBar->addAction(shoeboxAct);
  }

#ifndef QT_NO_CLIPBOARD
  // EDIT MENU
  QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
  QToolBar *editToolBar = addToolBar(tr("Edit"));
  {
    const QIcon cutIcon =
        QIcon::fromTheme("edit-cut", QIcon(":/images/cut.png"));
    cutAct = new QAction(cutIcon, tr("Cu&t"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(
        tr("Cut the current selection's contents to the "
           "clipboard"));
    connect(cutAct, &QAction::triggered, this, &MainWindow::cut);
    editMenu->addAction(cutAct);
    editToolBar->addAction(cutAct);

    const QIcon copyIcon =
        QIcon::fromTheme("edit-copy", QIcon(":/images/copy.png"));
    copyAct = new QAction(copyIcon, tr("&Copy"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(
        tr("Copy the current selection's contents to the "
           "clipboard"));
    connect(copyAct, &QAction::triggered, this, &MainWindow::copy);
    editMenu->addAction(copyAct);
    editToolBar->addAction(copyAct);

    const QIcon pasteIcon =
        QIcon::fromTheme("edit-paste", QIcon(":/images/paste.png"));
    pasteAct = new QAction(pasteIcon, tr("&Paste"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(
        tr("Paste the clipboard's contents into the current "
           "selection"));
    connect(pasteAct, &QAction::triggered, this, &MainWindow::paste);
    editMenu->addAction(pasteAct);
    editToolBar->addAction(pasteAct);
  }
#endif

  // WINDOW MENU
  windowMenu = menuBar()->addMenu(tr("&Window"));
  {
    connect(windowMenu, &QMenu::aboutToShow, this,
            &MainWindow::updateWindowMenu);

    const QIcon settingsIcon = QIcon::fromTheme("emblem-system");
    settingsAct = new QAction(settingsIcon, tr("&Settings"), this);
    settingsAct->setStatusTip(tr("Display the active level settings"));
    connect(settingsAct, &QAction::triggered, this,
            &MainWindow::activeMdiChildSettingsDialog);

    closeAct = new QAction(tr("Cl&ose"), this);
    closeAct->setStatusTip(tr("Close the active window"));
    connect(closeAct, &QAction::triggered, mdiArea,
            &QMdiArea::closeActiveSubWindow);

    closeAllAct = new QAction(tr("Close &All"), this);
    closeAllAct->setStatusTip(tr("Close all the windows"));
    connect(closeAllAct, &QAction::triggered, mdiArea,
            &QMdiArea::closeAllSubWindows);

    tileAct = new QAction(tr("&Tile"), this);
    tileAct->setStatusTip(tr("Tile the windows"));
    connect(tileAct, &QAction::triggered, mdiArea, &QMdiArea::tileSubWindows);

    cascadeAct = new QAction(tr("&Cascade"), this);
    cascadeAct->setStatusTip(tr("Cascade the windows"));
    connect(cascadeAct, &QAction::triggered, mdiArea,
            &QMdiArea::cascadeSubWindows);

    const QIcon nextIcon = QIcon::fromTheme("go-next");
    nextAct = new QAction(nextIcon, tr("Ne&xt"), this);
    nextAct->setShortcuts(QKeySequence::NextChild);
    nextAct->setStatusTip(tr("Move the focus to the next window"));
    connect(nextAct, &QAction::triggered, mdiArea,
            &QMdiArea::activateNextSubWindow);

    const QIcon previousIcon = QIcon::fromTheme("go-previous");
    previousAct = new QAction(previousIcon, tr("Pre&vious"), this);
    previousAct->setShortcuts(QKeySequence::PreviousChild);
    previousAct->setStatusTip(
        tr("Move the focus to the previous "
           "window"));
    connect(previousAct, &QAction::triggered, mdiArea,
            &QMdiArea::activatePreviousSubWindow);

    windowMenuSeparatorAct = new QAction(this);
    windowMenuSeparatorAct->setSeparator(true);

    updateWindowMenu();
  }

  menuBar()->addSeparator();

  // HELP MENU
  QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
  {
    QAction *aboutAct =
        helpMenu->addAction(tr("&About"), this, &MainWindow::about);
    aboutAct->setStatusTip(tr("Show the application's About box"));

    QAction *aboutQtAct =
        helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
  }
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::readSettings()
{
  QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
  const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
  if (geometry.isEmpty()) {
    const QRect availableGeometry = screen()->availableGeometry();
    resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
    move((availableGeometry.width() - width()) / 2,
         (availableGeometry.height() - height()) / 2);
  } else {
      restoreGeometry(geometry);
  }
}

void MainWindow::writeSettings()
{
  QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
  settings.setValue("geometry", saveGeometry());
}

void MainWindow::activeMdiChildSettingsDialog() {
  MdiChild *child = activeMdiChild();

  SettingsDialog *settingsDialog = new SettingsDialog(this);
  settingsDialog->setData(child->getData());

  connect(settingsDialog, &SettingsDialog::finished,
          [child, settingsDialog](int result) {
            if (result == QDialog::Accepted) {
              child->setData(settingsDialog->getData());
              child->show();

              settingsDialog->disconnect();
              settingsDialog->deleteLater();
            }
          });
  settingsDialog->show();
}

MdiChild *MainWindow::activeMdiChild() const
{
  if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
    return qobject_cast<MdiChild *>(activeSubWindow->widget());
  return nullptr;
}

QMdiSubWindow *MainWindow::findMdiChild(const QString &fileName) const
{
  QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

  const QList<QMdiSubWindow *> subWindows = mdiArea->subWindowList();
  for (QMdiSubWindow *window : subWindows) {
    MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
    if (mdiChild && mdiChild->currentFile() == canonicalFilePath)
      return window;
  }
  return nullptr;
}

void MainWindow::updateMdiChildren() {
  // clang-format off
  QMap<TkLayer, bool> map = {
    { TkLayer::ground,     groundAct->isChecked()     },
    { TkLayer::background, backgroundAct->isChecked() },
    { TkLayer::foreground, foregroundAct->isChecked() } };
  // clang-format on

  const QList<QMdiSubWindow *> subWindows = mdiArea->subWindowList();
  for (QMdiSubWindow *window : subWindows) {
    MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
    if (mdiChild != nullptr) {
      mdiChild->updateGrid(map);
    }
  }
}

void MainWindow::updateMdiShoebox() {
  const QList<QMdiSubWindow *> subWindows = mdiArea->subWindowList();
  for (QMdiSubWindow *window : subWindows) {
    MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
    if (mdiChild != nullptr) {
      mdiChild->updateShoeboxState(shoeboxAct->isChecked());
    }
  }
}
