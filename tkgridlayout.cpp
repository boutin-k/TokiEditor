#include "tkgridlayout.h"
#include "tkgriditem.h"

#include <QWidget>
#include <QVariant>

/**
 * @brief Default constructor
 */
TkGridLayout::TkGridLayout() {}

/**
 * @brief Constructor with QWidget parent
 * @param[in,out] parent QWidget Parent
 */
TkGridLayout::TkGridLayout(QWidget *parent) : QGridLayout(parent) {}

/**
 * @brief Shift the rows in the grid
 * @param[in] shiftCount The shift bound
 */
void TkGridLayout::shiftRow(int shiftCount, callback func) {
  if (shiftCount == 0) return;

  if (shiftCount > 0) {
    // Shift to the bottom
    for (int rowId = itemRowCount - 1; rowId >= 0; rowId--) {
      for (int colId = 0; colId < itemColCount; colId++) {
        QLayoutItem *item = itemAtPosition(rowId, colId);
        if (item != nullptr) {
          if (rowId < itemRowCount - shiftCount) {
            // Update row in the widget property
            item->widget()->setProperty("row", rowId + shiftCount);
            addWidget(item->widget(), rowId + shiftCount, colId);
          } else {
            deleteChildWidgets(item);
          }
        }

        if (func && rowId < shiftCount) {
          addWidget(func(rowId, colId, mdiId), rowId, colId);
        }
      }
    }
  } else {
    shiftCount *= -1;
    // Shift to the top
    for (int rowId = 0; rowId < itemRowCount; rowId++) {
      for (int colId = 0; colId < itemColCount; colId++) {
        QLayoutItem *item = itemAtPosition(rowId, colId);
        if (item != nullptr) {
          if (rowId >= shiftCount) {
            // Update row in the widget property
            item->widget()->setProperty("row", rowId - shiftCount);
            addWidget(item->widget(), rowId - shiftCount, colId);
          } else {
            deleteChildWidgets(item);
          }
        }

        if (func && rowId >= itemRowCount - shiftCount) {
          addWidget(func(rowId, colId, mdiId), rowId, colId);
        }
      }
    }

  }
}

/**
 * @brief Shift the column in the grid
 * @param[in] shiftCount The shift bound
 */
void TkGridLayout::shiftColumn(int shiftCount, callback func) {
  if (shiftCount == 0) return;

  if (shiftCount > 0) {
    // Shift to the right
    for (int colId = itemColCount - 1; colId >= 0; colId--) {
      for (int rowId = 0; rowId < itemRowCount; rowId++) {
        QLayoutItem *item = itemAtPosition(rowId, colId);
        if (item != nullptr) {
          if (colId < itemColCount - shiftCount) {
            // Update row in the widget property
            item->widget()->setProperty("col", colId + shiftCount);
            addWidget(item->widget(), rowId, colId + shiftCount);
          } else {
            deleteChildWidgets(item);
          }
        }

        if (func && colId < shiftCount) {
          addWidget(func(rowId, colId, mdiId), rowId, colId);
        }
      }
    }
  } else {
    shiftCount *= -1;
    // Shift to the left
    for (int colId = 0; colId < itemColCount; colId++) {
      for (int rowId = 0; rowId < itemRowCount; rowId++) {
        QLayoutItem *item = itemAtPosition(rowId, colId);
        if (item != nullptr) {
          if (colId >= shiftCount) {
            // Update col in the widget property
            item->widget()->setProperty("col", colId - shiftCount);
            addWidget(item->widget(), rowId, colId - shiftCount);
          } else {
            deleteChildWidgets(item);
          }
        }

        if (func && colId >= itemColCount - shiftCount) {
          addWidget(func(rowId, colId, mdiId), rowId, colId);
        }
      }
    }

  }
}

/**
 * @brief Add rows in the grid
 * @param[in] rowCount The quantity of row to add
 */
void TkGridLayout::addRow(uint32_t rowCount, callback func) {
  if (rowCount == 0 || !func) return;

  rowCount += itemRowCount;
  for (uint32_t rowId = itemRowCount; rowId < rowCount; rowId++)
    for (int colId = 0; colId < itemColCount; colId++) {
      addWidget(func(rowId, colId, mdiId), rowId, colId);
    }
  // Update the row counter
  emit rowCountChanged(itemRowCount = rowCount);
}

/**
 * @brief Add columns in the grid
 * @param[in] columnCount The quantity of columns to add
 */
void TkGridLayout::addColumn(uint32_t columnCount, callback func) {
  if (columnCount == 0 || !func) return;

  columnCount += itemColCount;
  for (uint32_t colId = itemColCount; colId < columnCount; colId++)
    for (int rowId = 0; rowId < itemRowCount; rowId++)
      addWidget(func(rowId, colId, mdiId), rowId, colId);
  // Update the column counter
  emit colCountChanged(itemColCount = columnCount);
}

/**
 * @brief Remove rows in the grid
 * @param[in] row The quantity of row to remove
 * @param[in] deleteWidgets \c true to delete the cells children, otherwise \c false
 */
void TkGridLayout::removeRow(int row, bool deleteWidgets) {
  if (row == 0) return;

  for (int rowId = itemRowCount - row; rowId < itemRowCount; rowId++) {
    remove(rowId, -1, deleteWidgets);
    setRowMinimumHeight(rowId, 0);
    setRowStretch(rowId, 0);
  }
  // Update the row counter
  emit rowCountChanged(itemRowCount -= row);
}

/**
 * @brief Remove columns in the grid
 * @param[in] column The quantity of column to remove
 * @param[in] deleteWidgets \c true to delete the cells children, otherwise \c false
 */
void TkGridLayout::removeColumn(int column, bool deleteWidgets) {
  if (column == 0) return;

  for (int colId = itemColCount - column; colId < itemColCount; colId++) {
    remove(-1, colId, deleteWidgets);
    setColumnMinimumWidth(colId, 0);
    setColumnStretch(colId, 0);
  }
  // Update the row counter
  emit colCountChanged(itemColCount -= column);
}

/**
 * @brief GridLayout::remove
 * @param[in] row The row position to remove
 * @param[in] column The column position to remove
 * @param[in] deleteWidgets \c true to delete the cells children, otherwise \c false
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
 * @brief Delete the child widgets of the item
 * @param[in] item The object on which the child must be deleted
 */
void TkGridLayout::deleteChildWidgets(QLayoutItem *item) {
  if (item == nullptr) return;

  QLayout *layout = item->layout();
  if (layout != nullptr) {
    // Process all child items recursively.
    int itemCount = count();
    for (int i = 0; i < itemCount; i++) {
      deleteChildWidgets(itemAt(i));
    }
  }
  delete static_cast<TkGridItem*>(item->widget());
}
