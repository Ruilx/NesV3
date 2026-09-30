#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
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
			QApplication::installTranslator(&translator);
			break;
		}
	}
	QCommandLineParser parser;
	parser.setApplicationDescription(
		"Qt NES emulator with optional automated ROM runs and screenshots.");
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addOption({
		QStringLiteral("run-ms"),
		QStringLiteral("Exit after this many milliseconds."),
		QStringLiteral("milliseconds")});
	parser.addOption({
		QStringLiteral("screenshot-dir"),
		QStringLiteral("Save viewport screenshots in this directory."),
		QStringLiteral("directory")});
	parser.addOption({
		QStringLiteral("screenshot-every-ms"),
		QStringLiteral("Save a screenshot at this interval."),
		QStringLiteral("milliseconds")});
	parser.addOption({
		QStringLiteral("screenshot-on-exit"),
		QStringLiteral("Save one final viewport screenshot before exit.")});
	parser.addPositionalArgument("rom", "ROM file to load automatically.");
	parser.process(a);

	const QStringList positionalArguments = parser.positionalArguments();
	if (positionalArguments.size() > 1) {
		parser.showMessageAndExit(
			QCommandLineParser::MessageType::Error,
			"Only one ROM path may be provided.",
			2);
	}
	const bool hasRunOption = parser.isSet("run-ms");
	bool runDurationOk = true;
	const int runDuration = hasRunOption
		? parser.value("run-ms").toInt(&runDurationOk)
		: 0;
	if (hasRunOption && (!runDurationOk || runDuration <= 0)) {
		parser.showMessageAndExit(
			QCommandLineParser::MessageType::Error,
			"--run-ms must be a positive integer.",
			2);
	}
	const bool hasScreenshotDirectory = parser.isSet("screenshot-dir");
	const bool hasScreenshotInterval = parser.isSet("screenshot-every-ms");
	const bool screenshotOnExit = parser.isSet("screenshot-on-exit");
	bool screenshotIntervalOk = true;
	const int screenshotInterval = hasScreenshotInterval
		? parser.value("screenshot-every-ms").toInt(&screenshotIntervalOk)
		: 0;
	if (hasScreenshotInterval
			&& (!screenshotIntervalOk || screenshotInterval <= 0)) {
		parser.showMessageAndExit(
			QCommandLineParser::MessageType::Error,
			"--screenshot-every-ms must be a positive integer.",
			2);
	}
	if ((hasScreenshotDirectory || hasScreenshotInterval || screenshotOnExit)
			&& !hasRunOption) {
		parser.showMessageAndExit(
			QCommandLineParser::MessageType::Error,
			"Screenshot options require --run-ms.",
			2);
	}
	if ((hasScreenshotInterval || screenshotOnExit) && !hasScreenshotDirectory) {
		parser.showMessageAndExit(
			QCommandLineParser::MessageType::Error,
			"Screenshot options require --screenshot-dir.",
			2);
	}
	MainWindow w;
	w.show();
	if (!positionalArguments.isEmpty()) {
		const QString romPath = positionalArguments.constFirst();
		QTimer::singleShot(0, &w, [&w, &a, romPath, runDuration, hasRunOption,
				screenshotDirectory = parser.value("screenshot-dir"),
				screenshotInterval, screenshotOnExit]() {
			QString error;
			if (!w.loadRomFromPath(romPath, error)) {
				qCritical().noquote() << "ROM load failed:" << error;
				a.exit(1);
				return;
			}
			if (hasRunOption && !w.startAutomatedRun(
					runDuration,
					screenshotDirectory,
					screenshotInterval,
					screenshotOnExit,
					error)) {
				qCritical().noquote() << "Automated run failed:" << error;
				a.exit(1);
			}
		});
	}
	return QApplication::exec();
}
