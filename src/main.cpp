#include "MainWindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

    QApplication::setApplicationName("NesV3");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("GT-Soft Studio");
    QApplication::setOrganizationDomain("https://github.com/Ruilx/NesV3");

	QTranslator translator;
	const QStringList uiLanguages = QLocale::system().uiLanguages();
	for (const QString &locale : uiLanguages) {
		const QString baseName = "NesV3_" + QLocale(locale).name();
		if (translator.load(":/i18n/" + baseName)) {
			a.installTranslator(&translator);
			break;
		}
	}
	MainWindow w;
	w.show();
	return QApplication::exec();
}
