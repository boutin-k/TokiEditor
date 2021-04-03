#include <QApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QTranslator>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(tokieditor);

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("Toki Level Editor");
    QCoreApplication::setOrganizationName("BOUTINEAU");
    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    parser.setApplicationDescription("Toki Level Editor");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The file to open.");
    parser.process(app);

    QTranslator myappTranslator;
    myappTranslator.load(QLocale(), QLatin1String("tokieditor"),
                         QLatin1String("_"), QLatin1String("languages"));
    app.installTranslator(&myappTranslator);

    MainWindow mainWin;
    mainWin.initialize();
    const QStringList posArgs = parser.positionalArguments();
    for (const QString &fileName : posArgs)
        mainWin.openFile(fileName);
    mainWin.show();

    return app.exec();
}
