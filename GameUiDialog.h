#pragma once

#include <QString>
#include <QStringList>

class QWidget;

namespace GameUi {
void showWoodMessage(QWidget* parent, const QString& title, const QString& body);
void showOperationGuide(QWidget* parent);
int selectWoodOption(QWidget* parent, const QString& title, const QString& body, const QStringList& options);
}
