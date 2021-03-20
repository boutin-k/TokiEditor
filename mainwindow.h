#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MdiChild;
class FlowLayout;
class TkLabel;
class TkGridItem;

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QMdiArea;
class QMdiSubWindow;
class QToolBox;
QT_END_NAMESPACE

#define QT_NO_CLIPBOARD


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();

    bool openFile(const QString &fileName);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void open();
    void save();
    void saveAs();
    void updateRecentFileActions();
    void openRecentFile();
    void dockItemClicked(TkLabel *, QMouseEvent *);
    void dockItemUnselect();

#ifndef QT_NO_CLIPBOARD
    void cut();
    void copy();
    void paste();
#endif

    void about();
    void updateMenus();
    void updateWindowMenu();
    MdiChild *createMdiChild();

    void mdiChildItemClicked(TkGridItem*, QMouseEvent *);
    void mdiChildItemHovered(TkGridItem*, QHoverEvent *);
    void activeMdiChildSettingsDialog();
    void updateMdiChildren();

private:
    enum { MaxRecentFiles = 5 };

    void createActions();
    void createDockWindows();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    bool loadFile(const QString &fileName);
    static bool hasRecentFiles();
    void prependToRecentFiles(const QString &fileName);
    void setRecentFilesVisible(bool visible);
    MdiChild *activeMdiChild() const;
    QMdiSubWindow *findMdiChild(const QString &fileName) const;

    QMdiArea *mdiArea;

    QMenu *windowMenu;
    QAction *newAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *recentFileActs[MaxRecentFiles];
    QAction *recentFileSeparator;
    QAction *recentFileSubMenuAct;

    QAction *groundAct;
    QAction *backgroundAct;
    QAction *foregroundAct;
#ifndef QT_NO_CLIPBOARD
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
#endif
    QAction *settingsAct;
    QAction *closeAct;
    QAction *closeAllAct;
    QAction *tileAct;
    QAction *cascadeAct;
    QAction *nextAct;
    QAction *previousAct;
    QAction *windowMenuSeparatorAct;

    QToolBox *dockToolbox;
    FlowLayout *dockLayout{nullptr};
    QWidget *_selectedDockTile{nullptr};
    QPixmap _selectedDockPixmap;

    QVector<TkLabel *> dockItems;
};

#endif
