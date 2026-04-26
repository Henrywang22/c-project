#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_FishingVoyage.h"

class FishingVoyage : public QMainWindow
{
    Q_OBJECT

public:
    FishingVoyage(QWidget *parent = nullptr);
    ~FishingVoyage();

private:
    Ui::FishingVoyageClass ui;
};

