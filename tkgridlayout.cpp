#include "tkgridlayout.h"

/**
 * @brief Default constructor
 */
TkGridLayout::TkGridLayout() {}

/**
 * @brief Constructor with QWidget parent
 * @param parent QWidget Parent
 */
TkGridLayout::TkGridLayout(QWidget *parent) : QGridLayout(parent) {}

/**
 * @brief GridLayout::shiftRow
 * @param shiftCount
 */
void TkGridLayout::shiftRow(int shiftCount, std::function<QWidget*()> func) {
  if (shiftCount == 0) return;

  if (shiftCount > 0) {
    // Shift to the bottom
    for (int rowId = rowCount() - 1; rowId >= 0; rowId--) {
      for (int colId = 0; colId < columnCount(); colId++) {
        QLayoutItem *item = itemAtPosition(rowId, colId);
        if (item != nullptr) {
          if (rowId < rowCount() - shiftCount) {
            addWidget(item->widget(), rowId + shiftCount, colId);
          } else {
            deleteChildWidgets(item);
          }
        }

        if (func && rowId < shiftCount) {
          addWidget(func(), rowId, colId);
        }
      }
    }
  } else {
    shiftCount *= -1;
    // Shift to the top
    for (int rowId = 0; rowId < rowCount(); rowId++) {
      for (int colId = 0; colId < columnCount(); colId++) {
        QLayoutItem *item = itemAtPosition(rowId, colId);
        if (item != nullptr) {
          if (rowId >= shiftCount) {
            addWidget(item->widget(), rowId - shiftCount, colId);
          } else {
            deleteChildWidgets(item);
          }
        }

        if (func && rowId >= rowCount() - shiftCount) {
          addWidget(func(), rowId, colId);
        }
      }
    }

  }
}

/**
 * @brief GridLayout::shiftColumn
 * @param shiftCount
 */
void TkGridLayout::shiftColumn(int shiftCount, std::function<QWidget*()> func) {
  if (shiftCount == 0) return;

  if (shiftCount > 0) {
    // Shift to the right
    for (int colId = columnCount() - 1; colId >= 0; colId--) {
      for (int rowId = 0; rowId < rowCount(); rowId++) {
        QLayoutItem *item = itemAtPosition(rowId, colId);
        if (item != nullptr) {
          if (colId < columnCount() - shiftCount) {
            addWidget(item->widget(), rowId, colId + shiftCount);
          } else {
            deleteChildWidgets(item);
          }
        }

        if (func && colId < shiftCount) {
          addWidget(func(), rowId, colId);
        }
      }
    }
  } else {
    shiftCount *= -1;
    // Shift to the left
    for (int colId = 0; colId < columnCount(); colId++) {
      for (int rowId = 0; rowId < rowCount(); rowId++) {
        QLayoutItem *item = itemAtPosition(rowId, colId);
        if (item != nullptr) {
          if (colId >= shiftCount) {
            addWidget(item->widget(), rowId, colId - shiftCount);
          } else {
            deleteChildWidgets(item);
          }
        }

        if (func && colId >= columnCount() - shiftCount) {
          addWidget(func(), rowId, colId);
        }
      }
    }

  }
}

/**
 * @brief GridLayout::addRow
 * @param rowCount
 */
void TkGridLayout::addRow(uint32_t rowCount, std::function<QWidget*()> func) {
  if (rowCount == 0 || !func) return;

  rowCount += this->rowCount();
  for (uint32_t rowId = this->rowCount(); rowId < rowCount; rowId++)
    for (int colId = 0; colId < columnCount(); colId++)
      addWidget(func(), rowId, colId);
}

/**
 * @brief GridLayout::addColumn
 * @param columnCount
 */
void TkGridLayout::addColumn(uint32_t columnCount, std::function<QWidget*()> func) {
  if (columnCount == 0 || !func) return;

  columnCount += this->columnCount();
  for (uint32_t colId = this->columnCount(); colId < columnCount; colId++)
    for (int rowId = 0; rowId < rowCount(); rowId++)
      addWidget(func(), rowId, colId);
}

/**
 * @brief GridLayout::removeRow
 * @param row
 * @param deleteWidgets
 */
void TkGridLayout::removeRow(int row, bool deleteWidgets) {
  if (row == 0) return;

  for (int rowId = rowCount() - row; rowId < rowCount(); rowId++) {
    remove(rowId, -1, deleteWidgets);
    setRowMinimumHeight(rowId, 0);
    setRowStretch(rowId, 0);
  }
}

/**
 * @brief GridLayout::removeColumn
 * @param column
 * @param deleteWidgets
 */
void TkGridLayout::removeColumn(int column, bool deleteWidgets) {
  if (column == 0) return;

  for (int colId = columnCount() - column; colId < columnCount();
       colId++) {
    remove(-1, colId, deleteWidgets);
    setColumnMinimumWidth(colId, 0);
    setColumnStretch(colId, 0);
  }
}

/**
 * @brief GridLayout::removeCell
 * @param row
 * @param column
 * @param deleteWidgets
 */
void TkGridLayout::removeCell(int row, int column, bool deleteWidgets) {
  remove(row, column, deleteWidgets);
}

/**
 * @brief GridLayout::remove
 * @param row
 * @param column
 * @param deleteWidgets
 */
void TkGridLayout::remove(int row, int column, bool deleteWidgets) {
  // We avoid usage of QGridLayout::itemAtPosition() here to improve performance.
  for (int i = count() - 1; i >= 0; i--) {
    int r, c, rs, cs;
    getItemPosition(i, &r, &c, &rs, &cs);
    if (
        (row == -1 || (r <= row && r + rs > row)) &&
        (column == -1 || (c <= column && c + cs > column))) {
      // This layout item is subject to deletion.
      QLayoutItem *item = itemAt(i);
      if (deleteWidgets) {
        deleteChildWidgets(item);
      }
    }
  }
}

/**
 * @brief GridLayout::deleteChildWidgets
 * @param item
 */
void TkGridLayout::deleteChildWidgets(QLayoutItem *item) {
  if (item == nullptr) return;

  QLayout *layout = item->layout();
  if (layout) {
    // Process all child items recursively.
    int itemCount = count();
    for (int i = 0; i < itemCount; i++) {
      deleteChildWidgets(itemAt(i));
    }
  }
  delete item->widget();
}
