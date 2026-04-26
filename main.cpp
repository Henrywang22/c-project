#include<QApplication>
#include"GameWindow.h"
int main(int argc, char* argv[])
{
	QApplication a(argc, argv);//创建Qt应用，管理整个程序生命周期

	GameWindow w; //创建游戏窗口
	w.show;

	return a.exec();// 进入事件循环，等待用户操作
}
