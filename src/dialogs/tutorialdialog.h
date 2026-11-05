#pragma once
#include <QDialog>

class TutorialDialog : public QDialog {
    Q_OBJECT
public:
    explicit TutorialDialog(QWidget *parent = nullptr);
    explicit TutorialDialog(const QString &tutorialFile, QWidget *parent = nullptr);
};
