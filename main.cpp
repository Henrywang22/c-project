#define QT_DISABLE_DEPRECATED_BEFORE 0x060000
#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QIcon>
#include <QMessageBox>
#include <QResource>
#include "GameWindow.h"

int main(int argc, char* argv[])
{
	QApplication a(argc, argv);//创建Qt应用，管理整个程序生命周期
	QCoreApplication::setOrganizationName(QStringLiteral("FishingVoyage"));
	QCoreApplication::setOrganizationDomain(QStringLiteral("fishingvoyage.local"));
	QCoreApplication::setApplicationName(QStringLiteral("FishingVoyage"));
	QCoreApplication::setApplicationVersion(QStringLiteral("1.0.4"));

	// 正式版使用外部二进制资源包，避免生成约 500 MB 的 qrc C++ 文件。
	// 开发环境若仍编译了内嵌资源，此调用失败也不影响运行。
	const QString resourcePath = QDir(QCoreApplication::applicationDirPath())
		.filePath(QStringLiteral("FishingVoyage.rcc"));
	if (QFile::exists(resourcePath)) {
		QResource::registerResource(resourcePath);
	}
	if (!QFile::exists(QStringLiteral(":/FishingVoyage/ui/menu/background.png"))) {
		QMessageBox::critical(nullptr, QStringLiteral("渔途"),
			QStringLiteral("游戏资源包缺失或损坏，请重新解压完整正式版。"));
		return 1;
	}
	// 同步窗口、任务栏与 exe 的航海主题图标。
	a.setWindowIcon(QIcon(QStringLiteral(":/FishingVoyage/ui/app_icon.png")));

	GameWindow w; //创建游戏窗口
	w.show();

	return a.exec();// 进入事件循环，等待用户操作
}
