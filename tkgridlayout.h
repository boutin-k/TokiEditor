#ifndef TKGRIDLAYOUT_H
#define TKGRIDLAYOUT_H

#include <QGridLayout>
#include <QObject>

class TkGridLayout : public QGridLayout
{
 public:
  explicit TkGridLayout(QWidget *parent);
  TkGridLayout();

  void shiftRow(int shiftCount, std::function<QWidget*()> func = nullptr);
  void shiftColumn(int shiftCount, std::function<QWidget*()> func = nullptr);
  void addRow(uint32_t rowCount, std::function<QWidget*()> func = nullptr);
  void addColumn(uint32_t columnCount, std::function<QWidget*()> func = nullptr);
  void removeRow(int row, bool deleteWidgets = true);
  void removeColumn(int column, bool deleteWidgets = true);
  void removeCell(int row, int column, bool deleteWidgets = true);
  void remove(int row, int column, bool deleteWidgets);

 private:
  void deleteChildWidgets(QLayoutItem *item);

};

#endif // TKGRIDLAYOUT_H
