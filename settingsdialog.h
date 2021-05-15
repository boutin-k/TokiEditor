#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QCheckBox>
#include <QDialogButtonBox>

#include "tkdata.h"

/**
 * @brief The SettingsDialog class is a dialog popup used to configure the mdi child data.
 * The data are : The tokilevel size, the music, the eggs animation, the shoebox, the actions
 */
class SettingsDialog : public QDialog
{
  Q_OBJECT

  // clang-format off
  struct actions {
    QString    imagePath;
    QCheckBox* checkbox{nullptr};
    QSpinBox*  spinbox{nullptr};

    actions(const QString &path, QCheckBox *check, QSpinBox *spin)
        : imagePath(path), checkbox(check), spinbox(spin) {}
  };

  const actions list[TkAction::actionCount] = {
    {":/images/actions/bridge.png",   &actionCheckbox[TkAction::bridge],   &actionSpinbox[TkAction::bridge]   },
    {":/images/actions/teleport.png", &actionCheckbox[TkAction::teleport], &actionSpinbox[TkAction::teleport] },
    {":/images/actions/crate.png",    &actionCheckbox[TkAction::crate],    &actionSpinbox[TkAction::crate]    },
    {":/images/actions/bloc.png",     &actionCheckbox[TkAction::bloc],     &actionSpinbox[TkAction::bloc]     }
  };
  // clang-format on


 public:
  SettingsDialog(QWidget *parent = nullptr, bool newFile = false);

  inline void setData(const levelData &d) {
    data = d;
    tileEdit.setText(d.tileFile);
    eggsEdit.setText(d.eggsFile);
    backEdit.setText(d.backFile);
    musicEdit.setText(d.musicFile);

    gridWidthSpinbox.setValue(d.gridWidth);
    gridHeightSpinbox.setValue(d.gridHeight);
    levelXSpinBox.setRange(-d.gridWidth, d.gridWidth);
    levelYSpinBox.setRange(-d.gridHeight, d.gridHeight);

    for (uint32_t index = 0U; index < TkAction::actionCount; ++index)
      setActionCounter((TkAction)index, d.action[index]);
  }

  inline levelData &getData() {
    std::memcpy(data.tileFile, tileEdit.text().toStdString().c_str(),
                tileEdit.text().size() + 1);
    std::memcpy(data.eggsFile, eggsEdit.text().toStdString().c_str(),
                eggsEdit.text().size() + 1);
    std::memcpy(data.backFile, backEdit.text().toStdString().c_str(),
                backEdit.text().size() + 1);
    std::memcpy(data.musicFile, musicEdit.text().toStdString().c_str(),
                musicEdit.text().size() + 1);

    data.gridWidth = gridWidthSpinbox.value() | (levelXSpinBox.value() << 16);
    data.gridHeight = gridHeightSpinbox.value() | (levelYSpinBox.value() << 16);

    for (uint32_t index = 0U; index < TkAction::actionCount; ++index)
      data.action[index] = getActionCounter((TkAction)index);

    return data;
  }

  inline uint32_t getActionCounter(const TkAction &action) const {
    if (action == TkAction::actionCount) return 0;
    if (list[action].checkbox->checkState() == Qt::Checked) return 0xFF;
    return list[action].spinbox->value();
  }

  inline void setActionCounter(const TkAction &action, uint32_t counter = 0) {
    if (counter == 0xFF) {
      list[action].spinbox->setValue(0);
      list[action].spinbox->setEnabled(false);
      list[action].checkbox->setChecked(true);
    } else {
      list[action].spinbox->setValue(counter);
      list[action].spinbox->setEnabled(true);
      list[action].checkbox->setChecked(false);
    }
  }

 private:
  Q_SLOT void tileButtonClicked(bool checked);
  Q_SLOT void eggsButtonClicked(bool checked);
  Q_SLOT void backButtonClicked(bool checked);
  Q_SLOT void musicButtonClicked(bool checked);

  Q_SLOT void widthValueChange(int value);
  Q_SLOT void heightValueChange(int value);

  QWidget *buildPathTab();
  QWidget *buildSizeTab();
  QWidget *buildActionTab();

  QString relativePath(const QString &path);


  levelData data;

  QDialogButtonBox buttonBox;
  QLineEdit tileEdit;
  QPushButton tileButton;

  QLineEdit eggsEdit;
  QPushButton eggsButton;

  QLineEdit backEdit;
  QPushButton backButton;

  QLineEdit musicEdit;
  QPushButton musicButton;

  QSpinBox gridWidthSpinbox;
  QSpinBox gridHeightSpinbox;
  QSpinBox levelXSpinBox;
  QSpinBox levelYSpinBox;

  QCheckBox actionCheckbox[TkAction::actionCount];
  QSpinBox  actionSpinbox[TkAction::actionCount];
};

#endif // SETTINGSDIALOG_H
