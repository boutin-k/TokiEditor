#ifndef TKGRIDLAYOUT_H
#define TKGRIDLAYOUT_H

#include <QGridLayout>
#include <QObject>

class TkGridLayout : public QGridLayout
{
  using callback = std::function<QWidget *(uint32_t, uint32_t, uint32_t)>;

 public:
  explicit TkGridLayout(QWidget *parent);
  TkGridLayout();

  void shiftRow(int shiftCount, callback func = nullptr);
  void shiftColumn(int shiftCount, callback func = nullptr);
  void addRow(uint32_t rowCount, callback func = nullptr);
  void addColumn(uint32_t columnCount, callback func = nullptr);
  void removeRow(int row, bool deleteWidgets = true);
  void removeColumn(int column, bool deleteWidgets = true);

  inline void refreshItemCount() {
    itemRowCount = rowCount();
    itemColCount = columnCount();
  }
  inline void setMdiId(uint32_t id) { mdiId = id; }

 private:
  void remove(int row, int column, bool deleteWidgets);
  void deleteChildWidgets(QLayoutItem *item);

  int itemRowCount{0};
  int itemColCount{0};
  uint32_t mdiId{0U};
};

#endif // TKGRIDLAYOUT_H
