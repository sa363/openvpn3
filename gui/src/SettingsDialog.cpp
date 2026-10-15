#include "SettingsDialog.h"

#include "ConnectionManager.h"

#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

QWidget *withBrowse(QLineEdit *edit, const std::function<void()> &browse)
{
    auto *w = new QWidget;
    auto *l = new QHBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    l->addWidget(edit, 1);
    auto *b = new QPushButton(QObject::tr("Browse..."));
    QObject::connect(b, &QPushButton::clicked, w, browse);
    l->addWidget(b);
    return w;
}

} // namespace

SettingsDialog::SettingsDialog(ConnectionManager *manager, QWidget *parent)
    : QDialog(parent),
      manager_(manager)
{
    setWindowTitle(tr("Settings"));
    setMinimumWidth(560);

    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;

    configDir_ = new QLineEdit(QDir::toNativeSeparators(manager->configDir()));
    form->addRow(tr("Profiles folder:"), withBrowse(configDir_, [this]()
                                                      {
        const QString dir = QFileDialog::getExistingDirectory(this, tr("Profiles folder"), configDir_->text());
        if (!dir.isEmpty())
            configDir_->setText(QDir::toNativeSeparators(dir)); }));

    workerPath_ = new QLineEdit(QDir::toNativeSeparators(manager->workerPath()));
    form->addRow(tr("Worker executable:"), withBrowse(workerPath_, [this]()
                                                        {
        const QString fn = QFileDialog::getOpenFileName(this, tr("Worker executable"), workerPath_->text(), tr("Programs (*.exe)"));
        if (!fn.isEmpty())
            workerPath_->setText(QDir::toNativeSeparators(fn)); }));

    auto *hint = new QLabel(tr("omicliagent.exe sets up tunnels through the ovpnagent service and needs no "
                               "administrator rights. omicli.exe works without the service but the GUI "
                               "must then run as administrator."));
    hint->setWordWrap(true);
    form->addRow(hint);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void SettingsDialog::accept()
{
    manager_->setWorkerPath(QDir::fromNativeSeparators(workerPath_->text().trimmed()));
    const QString dir = QDir::fromNativeSeparators(configDir_->text().trimmed());
    if (dir != manager_->configDir())
        manager_->setConfigDir(dir);
    QDialog::accept();
}
