#include <QtWidgets>
#include <QMap>

#include "tkflowlayout.h"
#include "mainwindow.h"
#include "mdichild.h"
#include "tklabel.h"
#include "tkutils.h"

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

/**
 * @brief Default constructor
 */
MainWindow::MainWindow()
    : mdiArea(new QMdiArea)
{
  setWindowIcon(QIcon(":/images/myappico.ico"));

  mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setCentralWidget(mdiArea);
  connect(mdiArea, &QMdiArea::subWindowActivated,
          this, &MainWindow::updateMenus);

  setWindowTitle(tr("Toki Level Editor"));
  setUnifiedTitleAndToolBarOnMac(true);
}

/**
 * @brief Initialize the main window child widgets
 */
void MainWindow::initialize() {
  try {
    createActions();
    createDockWindows();
    createStatusBar();
    updateMenus();

    readSettings();
  } catch (const char *msg) {
    QMessageBox::warning(this, tr("Toki Level Editor"), msg);
    throw;
  }
}

/**
 * @brief Close all the mdi windows
 * @param[in,out] event Close event
 */
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

/**
 * @brief Create a new tokilevel file
 */
void MainWindow::newFile()
{
  SettingsDialog *settingsDialog = new SettingsDialog(this, true);
  connect(settingsDialog, &SettingsDialog::finished, this,
          [this, settingsDialog](int result) {
            if (result == QDialog::Accepted) {
              MdiChild *child = createMdiChild();
              if (nullptr != child) {
                child->newFile(settingsDialog->getData());
                child->show();
              }

              settingsDialog->disconnect();
              settingsDialog->deleteLater();
            }
  });
  dockItemUnselect();
  settingsDialog->show();
}

/**
 * @brief Launch the open file dialog
 */
void MainWindow::open()
{
  const QString fileName = QFileDialog::getOpenFileName(this, tr("Open Level"), "~", "*.tokilevel");
  if (!fileName.isEmpty())
    openFile(fileName);
}

/**
 * @brief Open an existing tokilevel file
 * @param[in] fileName The path of the tokilevel file
 * @return \c true if file is correctly loaded, otherwise \c false
 */
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

/**
 * @brief Load the tokilevel data into a mdi child
 * @param fileName The path of the tokilevel file
 * @return \c true if file is correctly loaded, otherwise \c false
 */
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
        uint32_t tile = item->getTile();
        if (tile != 0xFF) {
          uint32_t groundTile = tile & 0xFF;
          if (groundTile < 0xFD) {
            item->updatePixmapLayer(
                dockItems[groundTile]->pixmap(Qt::ReturnByValue), TkLayer::ground, groundTile,
                true);
          } else if (groundTile == 0xFD) {
            item->updatePixmapLayer(QPixmap(":/images/startIcon.png"), TkLayer::ground,
                                    groundTile, true);
            child->setTokiTile(item);
          } else if (groundTile == 0xFE) {
            item->updatePixmapLayer(QPixmap(":/images/egg.png"), TkLayer::ground, groundTile, true);
          }

          uint32_t backTile = (tile & 0xFF00) >> 8;
          if (backTile)
            item->updatePixmapLayer(
                dockItems[backTile]->pixmap(Qt::ReturnByValue), TkLayer::background, backTile, true);

          uint32_t frontTile = (tile & 0xFF0000) >> 16;
          if (frontTile)
            item->updatePixmapLayer(
                dockItems[frontTile]->pixmap(Qt::ReturnByValue), TkLayer::foreground, frontTile, true);
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

/**
 * @brief Get from the QSettings the list of recent opened files
 * @param[in,out] settings The internal data storage
 * @return The list of recent opened files
 */
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

/**
 * @brief Set the list of recent opened files
 * @param[in] files The list of recent opened filed
 * @param[in,out] settings The internal data storage
 */
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

/**
 * @brief Check if recent opened file exists in internal data storage
 * @return \c true if it exists, otherwise \c false
 */
bool MainWindow::hasRecentFiles()
{
  QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
  const int count = settings.beginReadArray(recentFilesKey());
  settings.endArray();
  return count > 0;
}

/**
 * @brief Prepend the file name into the list of opened file
 * @param[in] fileName The file to prepend
 */
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

/**
 * @brief Enable/Disable the visibility of recent file menu
 * @param[in] visible The visibility state
 */
void MainWindow::setRecentFilesVisible(bool visible)
{
  recentFileSubMenuAct->setVisible(visible);
  recentFileSeparator->setVisible(visible);
}

/**
 * @brief Update recent file actions visbility
 */
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

/**
 * @brief Open a file from the recent file list
 */
void MainWindow::openRecentFile()
{
  if (const QAction *action = qobject_cast<const QAction *>(sender()))
    openFile(action->data().toString());
}

/**
 * @brief Save the current tokilevel file
 */
void MainWindow::save()
{
  if (activeMdiChild() && activeMdiChild()->save())
    statusBar()->showMessage(tr("File saved"), 2000);
}

/**
 * @brief Save the current tokilevel file to an other name
 */
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

/**
 * @brief Display about message box
 */
void MainWindow::about()
{
  QMessageBox::about(this, tr("About Toki Level Editor"),
                     tr("The <b>Toki Level Editor</b> is used to create and update levels "
                     "for <b>Toki</b> platform game."));
}

/**
 * @brief Update the menu states
 */
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

/**
 * @brief Update window menu
 */
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

/**
 * @brief Listener called when a child of mdi window is clicked
 * @param[in,out] item The clicked widget
 * @param[in] button The button related to the click event
 */
void MainWindow::mdiChildItemClicked(TkGridItem *item, Qt::MouseButton button) {
  if (activeMdiChild() == nullptr) return;

  int index = dockToolbox->currentIndex();
  if (index < 0) return;

  // Entities are on the ground layer
  if (index == 3) index = 0;

  switch (button) {
    case Qt::LeftButton: {
      if (selectedDockTile != nullptr) {
        uint32_t tile = selectedDockTile->property("id").toUInt();

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

        if (item->updatePixmapLayer(selectedDockPixmap, index, tile))
          activeMdiChild()->setModified(true);
      }
      break;
    }
    case Qt::RightButton: {
      if ((index == 0) && (item->getTile()&0xFF) == 0xFD)
        activeMdiChild()->setTokiTile(nullptr);

      if (item->clearPixmapLayer(index))
        activeMdiChild()->setModified(true);
      break;
    }
    default:
      break;
  }
}

/**
 * @brief Create a new mdi child
 * @return The instance of mdi child
 */
MdiChild *MainWindow::createMdiChild()
{
  MdiChild *child = new MdiChild;
  mdiArea->addSubWindow(child);
  connect(child, &MdiChild::itemClicked, this,
          &MainWindow::mdiChildItemClicked);

//#ifndef QT_NO_CLIPBOARD
//    connect(child, &QTextEdit::copyAvailable, cutAct, &QAction::setEnabled);
//    connect(child, &QTextEdit::copyAvailable, copyAct, &QAction::setEnabled);
//#endif
  return child;
}

/**
 * @brief Build the left dock
 */
void MainWindow::createDockWindows()
{
  QDockWidget *dock = new QDockWidget(tr("Forest Tiles"), this);
  dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  dock->setFeatures(QDockWidget::DockWidgetMovable |
                    QDockWidget::DockWidgetFloatable);
  dock->setMinimumWidth(113);
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
  TkFlowLayout* groundLayout = new TkFlowLayout(new QWidget(dockToolbox));
  dockToolbox->addItem(groundLayout->parentWidget(), tr("ground"));

  TkFlowLayout* backgroundLayout = new TkFlowLayout(new QWidget(dockToolbox));
  dockToolbox->addItem(backgroundLayout->parentWidget(), tr("background"));

  TkFlowLayout* foregroundtLayout = new TkFlowLayout(new QWidget(dockToolbox));
  dockToolbox->addItem(foregroundtLayout->parentWidget(), tr("foreground"));

  TkFlowLayout *entityLayout = new TkFlowLayout(new QWidget(dockToolbox));
  dockToolbox->addItem(entityLayout->parentWidget(), tr("entity"));

  connect(dockToolbox, &QToolBox::currentChanged, this, &MainWindow::dockItemUnselect);

  scrollArea->setWidget(dockToolbox);

  QPixmap *pixmap = new QPixmap(absolutePath("textures/tiles/forest_tiles.png"));
  if (pixmap == nullptr || pixmap->isNull())
    throw "Could not find the file : 'textures/tiles/forest_tiles.png'";

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

/**
 * @brief Listener called when a child of dock widget is unselected
 */
void MainWindow::dockItemUnselect() {
  if (nullptr != selectedDockTile) {
    selectedDockTile->setStyleSheet(DockItemDefaultStylesheet);
    ((QLabel *)selectedDockTile)->setPixmap(selectedDockPixmap);
    selectedDockTile = nullptr;
  }
}

/**
 * @brief Listener called when a child of dock widget is selected
 * @param[in,out] tile The widget related to the click
 * @param[in,out] event The mouse event
 */
void MainWindow::dockItemClicked(TkLabel *tile, QMouseEvent *event) {
  if (tile == selectedDockTile) return;

  if (event->type() == QEvent::MouseButtonPress) {
    // Restore previous button
    dockItemUnselect();

    selectedDockTile = tile;
    tile->setStyleSheet(DockItemSelectedStylesheet);

    selectedDockPixmap = tile->pixmap(Qt::ReturnByValue);
    QPixmap tempPixmap = selectedDockPixmap;

    QPainter painter;
    painter.begin(&tempPixmap);
    painter.fillRect(selectedDockPixmap.rect(), QColor(127, 127, 127, 127));
    painter.end();

    tile->setPixmap(tempPixmap);
  }
}

/**
 * @brief Build the menu and toolbar
 */
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

/**
 * @brief Build the statusbar
 */
void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

/**
 * @brief Restore the last position of the application on the screen
 */
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

/**
 * @brief Save the position of the application on the screen
 */
void MainWindow::writeSettings()
{
  QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
  settings.setValue("geometry", saveGeometry());
}

/**
 * @brief Open the mdi settings dialog
 */
void MainWindow::activeMdiChildSettingsDialog() {
  MdiChild *child = activeMdiChild();

  SettingsDialog *settingsDialog = new SettingsDialog(this);
  settingsDialog->setData(child->getData());

  connect(settingsDialog, &SettingsDialog::finished, child,
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

/**
 * @brief Get the focused mdi child
 * @return The pointer of the active mdi child or nullptr if none
 */
MdiChild *MainWindow::activeMdiChild() const
{
  if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
    return qobject_cast<MdiChild *>(activeSubWindow->widget());
  return nullptr;
}

/**
 * @brief Find a mdi file from its file name
 * @param[in] fileName The file name related to the mdi child
 * @return The pointer of the active mdi child or nullptr if not found
 */
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

/**
 * @brief Update the content of mdi children
 */
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

/**
 * @brief Update the shoebox of mdi children
 */
void MainWindow::updateMdiShoebox() {
  const QList<QMdiSubWindow *> subWindows = mdiArea->subWindowList();
  for (QMdiSubWindow *window : subWindows) {
    MdiChild *mdiChild = qobject_cast<MdiChild *>(window->widget());
    if (mdiChild != nullptr) {
      mdiChild->updateShoeboxState(shoeboxAct->isChecked());
    }
  }
}
