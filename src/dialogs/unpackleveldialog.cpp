#include "unpackleveldialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QFrame>

UnpackLevelDialog::UnpackLevelDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Select unpack level"));
    // Never lock the height: a fixed size squeezes the checkbox rows below their
    // size hint, which clips the labels and crops the check indicators.
    setMinimumWidth(580);
    setupUi();
    adjustSize();
}

void UnpackLevelDialog::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(8);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    // Description label
    auto *descLabel = new QLabel(
        tr("Choose an unpack level to start the unpacking procedure or cancel the operation."));
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);

    // Level 1 (always checked, disabled)
    cbLevel1 = new QCheckBox(tr("Unpack level 1 – split upgrade package to partitions"));
    cbLevel1->setChecked(true);
    cbLevel1->setEnabled(false);
    QFont f1 = cbLevel1->font(); f1.setPointSize(10); cbLevel1->setFont(f1);
    mainLayout->addWidget(cbLevel1);

    // Level 2
    cbLevel2 = new QCheckBox(tr("Unpack level 2 – unpack partitions to modify internal content"));
    QFont f2 = cbLevel2->font(); f2.setPointSize(10); cbLevel2->setFont(f2);
    mainLayout->addWidget(cbLevel2);
    connect(cbLevel2, &QCheckBox::toggled, this, &UnpackLevelDialog::onLevel2Changed);

    // Level 2 sub-partitions in a scrollable group
    auto *l2Group = new QGroupBox(tr("Level 2 partitions"));
    auto *l2Layout = new QVBoxLayout(l2Group);
    l2Layout->setSpacing(2);

    auto makeSubCB = [&](const QString &label) {
        auto *cb = new QCheckBox(label);
        cb->setEnabled(false);
        l2Layout->addWidget(cb);
        connect(cb, &QCheckBox::toggled, this, &UnpackLevelDialog::onSubPartitionChanged);
        return cb;
    };

    cbSuper      = makeSubCB(tr("Super (dynamic partition container)"));
    cbSystem     = makeSubCB(tr("System"));
    cbVendor     = makeSubCB(tr("Vendor"));
    cbProduct    = makeSubCB(tr("Product"));
    cbOdm        = makeSubCB(tr("Odm"));
    cbOem        = makeSubCB(tr("Oem"));
    cbSystemExt  = makeSubCB(tr("System_ext"));
    cbOdmExt     = makeSubCB(tr("Odm_ext"));
    cbBoot       = makeSubCB(tr("Boot"));
    cbRecovery   = makeSubCB(tr("Recovery"));
    cbDtb        = makeSubCB(tr("Dtb / Meson (Device Tree)"));

    mainLayout->addWidget(l2Group);

    // Level 3
    cbLevel3 = new QCheckBox(tr("Unpack level 3 – Extract UI files"));
    QFont f3 = cbLevel3->font(); f3.setPointSize(10); cbLevel3->setFont(f3);
    mainLayout->addWidget(cbLevel3);
    connect(cbLevel3, &QCheckBox::toggled, this, &UnpackLevelDialog::onLevel3Changed);

    auto *l3Group = new QGroupBox(tr("Level 3 assets"));
    auto *l3Layout = new QVBoxLayout(l3Group);
    l3Layout->setSpacing(2);

    auto makeL3CB = [&](const QString &label) {
        auto *cb = new QCheckBox(label);
        cb->setEnabled(false);
        l3Layout->addWidget(cb);
        connect(cb, &QCheckBox::toggled, this, &UnpackLevelDialog::onLevel3ItemChanged);
        return cb;
    };

    cbLogo           = makeL3CB(tr("Boot Logo (BMP image)"));
    cbBootanimation  = makeL3CB(tr("Boot Animation (ZIP frames)"));
    cbBootvideo      = makeL3CB(tr("Boot Video (MP4)"));
    cbLauncher       = makeL3CB(tr("Launcher APK (decompiled)"));

    mainLayout->addWidget(l3Group);
    mainLayout->addStretch();

    // Buttons
    auto *btnLayout = new QHBoxLayout;
    auto *btnCancel = new QPushButton(tr("Cancel"));
    auto *btnUnpack = new QPushButton(tr("Unpack"));
    btnUnpack->setDefault(true);
    btnLayout->addStretch();
    btnLayout->addWidget(btnCancel);
    btnLayout->addWidget(btnUnpack);
    mainLayout->addLayout(btnLayout);

    connect(btnUnpack, &QPushButton::clicked, this, &UnpackLevelDialog::onUnpackClicked);
    connect(btnCancel, &QPushButton::clicked, this, &UnpackLevelDialog::onCancelClicked);
}

void UnpackLevelDialog::onLevel2Changed(bool checked)
{
    QList<QCheckBox*> l2cbs = {cbSuper, cbSystem, cbVendor, cbProduct,
                                cbOdm, cbOem, cbSystemExt, cbOdmExt,
                                cbBoot, cbRecovery, cbDtb};
    for (auto *cb : l2cbs) cb->setEnabled(checked);
    if (!checked) {
        for (auto *cb : l2cbs) cb->setChecked(false);
        cbLevel3->setChecked(false);
    }
}

void UnpackLevelDialog::onLevel3Changed(bool checked)
{
    QList<QCheckBox*> l3cbs = {cbLogo, cbBootanimation, cbBootvideo, cbLauncher};
    for (auto *cb : l3cbs) cb->setEnabled(checked);
    if (!checked) {
        for (auto *cb : l3cbs) cb->setChecked(false);
    }
}

void UnpackLevelDialog::onSubPartitionChanged(bool checked)
{
    Q_UNUSED(checked);
    QList<QCheckBox*> l2cbs = {cbSuper, cbSystem, cbVendor, cbProduct,
                                cbOdm, cbOem, cbSystemExt, cbOdmExt,
                                cbBoot, cbRecovery, cbDtb};
    bool anyChecked = false;
    for (auto *cb : l2cbs) {
        if (cb->isChecked()) { anyChecked = true; break; }
    }
    cbLevel2->setChecked(anyChecked);
}

void UnpackLevelDialog::onLevel3ItemChanged(bool checked)
{
    Q_UNUSED(checked);
    QList<QCheckBox*> l3cbs = {cbLogo, cbBootanimation, cbBootvideo, cbLauncher};
    bool anyChecked = false;
    for (auto *cb : l3cbs) {
        if (cb->isChecked()) { anyChecked = true; break; }
    }
    cbLevel3->setChecked(anyChecked);
}

void UnpackLevelDialog::onUnpackClicked()
{
    m_unpackLevel = "level1;";
    if (cbLevel2->isChecked()) {
        m_unpackLevel += "level2;";
        if (cbSuper->isChecked())     m_unpackLevel += "super;";
        if (cbSystem->isChecked())    m_unpackLevel += "system;";
        if (cbVendor->isChecked())    m_unpackLevel += "vendor;";
        if (cbProduct->isChecked())   m_unpackLevel += "product;";
        if (cbOdm->isChecked())       m_unpackLevel += "odm;";
        if (cbOem->isChecked())       m_unpackLevel += "oem;";
        if (cbSystemExt->isChecked()) m_unpackLevel += "system_ext;";
        if (cbOdmExt->isChecked())    m_unpackLevel += "odm_ext;";
        if (cbBoot->isChecked())      m_unpackLevel += "boot;";
        if (cbRecovery->isChecked())  m_unpackLevel += "recovery;";
        if (cbDtb->isChecked())       m_unpackLevel += "dtb;";
    }
    if (cbLevel3->isChecked()) {
        m_unpackLevel += "level3;";
        if (cbLogo->isChecked())          m_unpackLevel += "logo;";
        if (cbBootanimation->isChecked()) m_unpackLevel += "bootanimation;";
        if (cbBootvideo->isChecked())     m_unpackLevel += "bootvideo;";
        if (cbLauncher->isChecked())      m_unpackLevel += "launcher;";
    }
    accept();
}

void UnpackLevelDialog::onCancelClicked()
{
    m_unpackLevel.clear();
    reject();
}
