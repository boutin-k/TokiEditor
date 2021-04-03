#ifndef TKUTILS_H
#define TKUTILS_H

#include <QString>
#include <QCoreApplication>

/**
 * @brief Translate a relative path to absolute
 * @param[in] relativePath The path to convert
 * @return The converted path
 */
static QString absolutePath(const QString &relativePath) {
  return QCoreApplication::applicationDirPath() + '/' + relativePath;
}

#endif // TKUTILS_H
