#include "settingsdialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QFileDialog>
#include <QCheckBox>

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel) {
  connect(&buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(&buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  QVBoxLayout *mainLayout = new QVBoxLayout();
  {
    QTabWidget *tabWidget = new QTabWidget(this);
    {
      tabWidget->addTab(buildPathTab(), tr("Paths"));
      tabWidget->addTab(buildSizeTab(), tr("Size"));
      tabWidget->addTab(buildActionTab(), tr("Actions"));
    }

    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(&buttonBox);
  }
  setLayout(mainLayout);

  setMinimumWidth(400);
  setWindowTitle(tr("Level Settings"));
  setModal(true);
}

/**
 * @brief SettingsDialog::buildPathTab
 * @return
 */
QWidget *SettingsDialog::buildPathTab() {
  QVBoxLayout *pathLayout = new QVBoxLayout(new QWidget);
  {
    QLabel *tileLabel = new QLabel(tr("Level tile file name:"));
    pathLayout->addWidget(tileLabel);
    QHBoxLayout *tileLayout = new QHBoxLayout();
    {
      tileLayout->addWidget(&tileEdit);
      tileLayout->addWidget(&tileButton);
      tileButton.setText("...");
      tileButton.setFixedSize(30, 30);
      tileLayout->setContentsMargins(0, 0, 0, 10);
      connect(&tileButton, &QPushButton::clicked, this,
              &SettingsDialog::tileButtonClicked);
      pathLayout->addLayout(tileLayout);
    }

    QLabel *eggsLabel = new QLabel(tr("Eggs sprite file name:"));
    pathLayout->addWidget(eggsLabel);
    QHBoxLayout *eggsLayout = new QHBoxLayout();
    {
      eggsLayout->addWidget(&eggsEdit);
      eggsLayout->addWidget(&eggsButton);
      eggsButton.setText("...");
      eggsButton.setFixedSize(30, 30);
      eggsLayout->setContentsMargins(0, 0, 0, 10);
      connect(&eggsButton, &QPushButton::clicked, this,
              &SettingsDialog::eggsButtonClicked);
      pathLayout->addLayout(eggsLayout);
    }

    QLabel *backLabel = new QLabel(tr("Shoebox file name:"));
    pathLayout->addWidget(backLabel);
    QHBoxLayout *backLayout = new QHBoxLayout();
    {
      backLayout->addWidget(&backEdit);
      backLayout->addWidget(&backButton);
      backButton.setText("...");
      backButton.setFixedSize(30, 30);
      backLayout->setContentsMargins(0, 0, 0, 10);
      connect(&backButton, &QPushButton::clicked, this,
              &SettingsDialog::backButtonClicked);
      pathLayout->addLayout(backLayout);
    }

    QLabel *musicLabel = new QLabel(tr("Music file name:"));
    pathLayout->addWidget(musicLabel);
    QHBoxLayout *musicLayout = new QHBoxLayout();
    {
      musicLayout->addWidget(&musicEdit);
      musicLayout->addWidget(&musicButton);
      musicButton.setText("...");
      musicButton.setFixedSize(30, 30);
      musicLayout->setContentsMargins(0, 0, 0, 10);
      connect(&musicButton, &QPushButton::clicked, this,
              &SettingsDialog::musicButtonClicked);
      pathLayout->addLayout(musicLayout);
    }
  }
  return pathLayout->parentWidget();
}

/**
 * @brief SettingsDialog::buildSizeTab
 * @return
 */
QWidget *SettingsDialog::buildSizeTab() {
  QVBoxLayout *sizeLayout = new QVBoxLayout(new QWidget);
  {
    QLabel *levelSizeLabel = new QLabel(
        tr("<font color='#7f7f7f'><b>Level Size:</b></font>"));
    sizeLayout->addWidget(levelSizeLabel);

    QLabel *levelWidthLabel = new QLabel(tr("Width :  "));
    levelWidthLabel->setSizePolicy(QSizePolicy::Preferred,
                                   QSizePolicy::Preferred);
    gridWidthSpinbox.setRange(tkGridMinWidth, tkGridMaxWidth);
    gridWidthSpinbox.setSizePolicy(QSizePolicy::Expanding,
                                   QSizePolicy::Preferred);
    connect(&gridWidthSpinbox, SIGNAL(valueChanged(int)), this, SLOT(widthValueChange(int)));
    QHBoxLayout *levelHLayout = new QHBoxLayout();
    {
      levelHLayout->addWidget(levelWidthLabel);
      levelHLayout->addWidget(&gridWidthSpinbox);
      levelHLayout->setContentsMargins(10, 0, 0, 10);
    }
    sizeLayout->addLayout(levelHLayout);

    QLabel *levelHeightLabel = new QLabel(tr("Height : "));
    levelHeightLabel->setSizePolicy(QSizePolicy::Preferred,
                                    QSizePolicy::Preferred);
    gridHeightSpinbox.setRange(tkGridMinHeight, tkGridMaxHeight);
    gridHeightSpinbox.setSizePolicy(QSizePolicy::Expanding,
                                    QSizePolicy::Preferred);
    connect(&gridHeightSpinbox, SIGNAL(valueChanged(int)), this, SLOT(heightValueChange(int)));
    QHBoxLayout *levelVLayout = new QHBoxLayout();
    {
      levelVLayout->addWidget(levelHeightLabel);
      levelVLayout->addWidget(&gridHeightSpinbox);
      levelVLayout->setContentsMargins(10, 0, 0, 10);
    }
    sizeLayout->addLayout(levelVLayout);

    QLabel *levelShiftLabel = new QLabel(
        tr("<font color='#7f7f7f'><b>Shift:</b></font>"));
    sizeLayout->addWidget(levelShiftLabel);

    QLabel *levelXLabel = new QLabel(tr("X : "));
    levelXLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    levelXSpinBox.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QHBoxLayout *levelXLayout = new QHBoxLayout();
    {
      levelXLayout->addWidget(levelXLabel);
      levelXLayout->addWidget(&levelXSpinBox);
      levelXLayout->setContentsMargins(10, 0, 0, 10);
    }
    sizeLayout->addLayout(levelXLayout);

    QLabel *levelYLabel = new QLabel(tr("Y : "));
    levelYLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    levelYSpinBox.setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QHBoxLayout *levelYLayout = new QHBoxLayout();
    {
      levelYLayout->addWidget(levelYLabel);
      levelYLayout->addWidget(&levelYSpinBox);
      levelYLayout->setContentsMargins(10, 0, 0, 10);
    }
    sizeLayout->addLayout(levelYLayout);
  }
  return sizeLayout->parentWidget();
}

/**
 * @brief SettingsDialog::buildActionTab
 * @return
 */
QWidget *SettingsDialog::buildActionTab() {
  QVBoxLayout *actionsLayout = new QVBoxLayout(new QWidget);
  {
    for (uint32_t index = 0U; index < TkAction::actionCount; ++index) {
      QHBoxLayout *actionHBoxLayout = new QHBoxLayout();
      {
        QLabel *actionIcon = new QLabel();
        actionIcon->setPixmap(QPixmap(list[index].imagePath));
        actionIcon->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        actionIcon->setMargin(10);
        actionHBoxLayout->addWidget(actionIcon);

        QVBoxLayout *unlimitedLayout = new QVBoxLayout();
        {
          list[index].checkbox->setText(tr("Unlimited"));
          connect(list[index].checkbox, &QCheckBox::stateChanged, this, [this, index](int state) {
            list[index].spinbox->setEnabled(state == Qt::Unchecked);
          });
          unlimitedLayout->addWidget(list[index].checkbox);

          list[index].spinbox->setSizePolicy(QSizePolicy::Expanding,
                                             QSizePolicy::Preferred);
          unlimitedLayout->addWidget(list[index].spinbox);
        }
        actionHBoxLayout->addLayout(unlimitedLayout);
      }
      actionsLayout->addLayout(actionHBoxLayout);

      // Add spacing except at the last
      if (index < TkAction::actionCount-1) actionsLayout->addSpacing(14);
    }

    actionsLayout->setContentsMargins(0, 10, 10, 10);
  }
  return actionsLayout->parentWidget();
}

/**
 * @brief SettingsDialog::tileButtonClicked
 */
void SettingsDialog::tileButtonClicked(bool) {
  QFileInfo fileInfo(tileEdit.text());

  const QString fileName =
      QFileDialog::getOpenFileName(this, tr("Open Tiles"), fileInfo.absolutePath(), "*.png");
  if (!fileName.isEmpty()) {
    tileEdit.setText(relativePath(fileName));
  }
}

/**
 * @brief SettingsDialog::eggsButtonClicked
 */
void SettingsDialog::eggsButtonClicked(bool) {
  QFileInfo fileInfo(eggsEdit.text());

  const QString fileName =
      QFileDialog::getOpenFileName(this, tr("Open Sprites"), fileInfo.absolutePath(), "*.png");
  if (!fileName.isEmpty()) {
    eggsEdit.setText(relativePath(fileName));
  }
}

/**
 * @brief SettingsDialog::backButtonClicked
 */
void SettingsDialog::backButtonClicked(bool) {
  QFileInfo fileInfo(backEdit.text());

  const QString fileName =
      QFileDialog::getOpenFileName(this, tr("Open Shoebox"), fileInfo.absolutePath(), "*.shoebox");
  if (!fileName.isEmpty()) {
    backEdit.setText(relativePath(fileName));
  }
}

/**
 * @brief SettingsDialog::musicButtonClicked
 */
void SettingsDialog::musicButtonClicked(bool) {
  QFileInfo fileInfo(musicEdit.text());

  const QString fileName =
      QFileDialog::getOpenFileName(this, tr("Open Music"), fileInfo.absolutePath(), "*.ttim");
  if (!fileName.isEmpty()) {
    musicEdit.setText(relativePath(fileName));
  }
}

/**
 * @brief SettingsDialog::widthValueChange
 */
void SettingsDialog::widthValueChange(int value) {
  levelXSpinBox.setRange(-value, value);
}

/**
 * @brief SettingsDialog::heightValueChange
 */
void SettingsDialog::heightValueChange(int value) {
  levelYSpinBox.setRange(-value, value);
}

/**
 * @brief SettingsDialog::relativePath
 * @param path
 * @return
 */
QString SettingsDialog::relativePath(const QString &path) {
  QDir dir(QCoreApplication::applicationDirPath());
  return dir.relativeFilePath(path);
}
