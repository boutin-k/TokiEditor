#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class MdiChild;
class TkLabel;
class TkGridItem;
class TkFlowLayout;

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

    void initialize();
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

    void mdiChildItemClicked(TkGridItem*, Qt::MouseButton button);
    void activeMdiChildSettingsDialog();
    void updateMdiChildren();
    void updateMdiShoebox();

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

    QMdiArea *mdiArea{nullptr};

    QMenu *windowMenu{nullptr};
    QAction *newAct{nullptr};
    QAction *saveAct{nullptr};
    QAction *saveAsAct{nullptr};
    QAction *recentFileActs[MaxRecentFiles];
    QAction *recentFileSeparator{nullptr};
    QAction *recentFileSubMenuAct{nullptr};

    QAction *groundAct{nullptr};
    QAction *backgroundAct{nullptr};
    QAction *foregroundAct{nullptr};
    QAction *shoeboxAct{nullptr};
#ifndef QT_NO_CLIPBOARD
    QAction *cutAct{nullptr};
    QAction *copyAct{nullptr};
    QAction *pasteAct{nullptr};
#endif
    QAction *settingsAct{nullptr};
    QAction *closeAct{nullptr};
    QAction *closeAllAct{nullptr};
    QAction *tileAct{nullptr};
    QAction *cascadeAct{nullptr};
    QAction *nextAct{nullptr};
    QAction *previousAct{nullptr};
    QAction *windowMenuSeparatorAct{nullptr};

    QToolBox *dockToolbox{nullptr};
    TkFlowLayout *dockLayout{nullptr};
    QWidget *selectedDockTile{nullptr};
    QPixmap selectedDockPixmap;

    QVector<TkLabel *> dockItems;
};

#endif
