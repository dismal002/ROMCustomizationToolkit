#pragma once
#include <QDialog>
#include <QCheckBox>
#include <QGroupBox>
#include <QString>

/**
 * UnpackLevelDialog – ports UnpackLevel.cs
 * User selects which levels/partitions to unpack.
 * Result returned as semicolon-separated string e.g. "level1;level2;super;system;vendor;level3;logo;"
 */
class UnpackLevelDialog : public QDialog {
    Q_OBJECT
public:
    explicit UnpackLevelDialog(QWidget *parent = nullptr);
    QString unpackLevel() const { return m_unpackLevel; }

private slots:
    void onUnpackClicked();
    void onCancelClicked();
    void onLevel2Changed(bool checked);
    void onLevel3Changed(bool checked);
    void onSubPartitionChanged(bool checked);
    void onLevel3ItemChanged(bool checked);

private:
    void setupUi();
    QString m_unpackLevel;

    // Level checkboxes
    QCheckBox *cbLevel1;
    QCheckBox *cbLevel2;
    QCheckBox *cbLevel3;

    // Level 2 sub-partitions
    QCheckBox *cbSuper;
    QCheckBox *cbSystem;
    QCheckBox *cbVendor;
    QCheckBox *cbProduct;
    QCheckBox *cbOdm;
    QCheckBox *cbOem;
    QCheckBox *cbSystemExt;
    QCheckBox *cbOdmExt;
    QCheckBox *cbBoot;
    QCheckBox *cbRecovery;
    QCheckBox *cbDtb;

    // Level 3 sub-options
    QCheckBox *cbLogo;
    QCheckBox *cbBootanimation;
    QCheckBox *cbBootvideo;
    QCheckBox *cbLauncher;
};
