#include "MainWindow.h"

#include <QApplication>
#include <QDebug>
#include <QLocale>
#include <QVector>
#include <QTranslator>

namespace {
bool parseBenchmarkArguments(
	const QStringList &arguments,
	QStringList &romPaths,
	QVector<int> &durationsMilliseconds) {
	int index = 1;
	while (index < arguments.size()) {
		const QString argument = arguments.at(index);
		if (argument == QStringLiteral("--benchmark")) {
			if (index + 2 >= arguments.size()) {
				return false;
			}
			romPaths = {arguments.at(index + 1), arguments.at(index + 2)};
			index += 3;
			continue;
		}
		if (argument == QStringLiteral("--durations-ms")) {
			if (index + 2 >= arguments.size()) {
				return false;
			}
			durationsMilliseconds.clear();
			for (int offset = 1; offset <= 2; ++offset) {
				bool ok = false;
				const int duration = arguments.at(index + offset).toInt(&ok);
				if (!ok || duration <= 0) {
					return false;
				}
				durationsMilliseconds.append(duration);
			}
			index += 3;
			continue;
		}
		return false;
	}
	return romPaths.size() == 2 && durationsMilliseconds.size() == 2;
}
}

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
			QApplication::installTranslator(&translator);
			break;
		}
	}
	MainWindow w;
	w.show();

	const QStringList arguments = QCoreApplication::arguments();
	if (arguments.size() > 1) {
		QStringList benchmarkRomPaths;
		QVector<int> benchmarkDurationsMilliseconds = {20000, 8000};
		if (!parseBenchmarkArguments(
				arguments, benchmarkRomPaths, benchmarkDurationsMilliseconds)
			|| !w.startBenchmark(
				benchmarkRomPaths, benchmarkDurationsMilliseconds)) {
			qCritical().noquote()
				<< "Usage: NesV3 --benchmark <rom1> <rom2>"
				<< "[--durations-ms <rom1-ms> <rom2-ms>]";
			return 2;
		}
	}
	return QApplication::exec();
}
