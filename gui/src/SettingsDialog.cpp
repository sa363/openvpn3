#include "SettingsDialog.h"

#include "ConnectionManager.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
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

    autoReconnect_ = new QCheckBox(tr("Connect again when a connection fails"));
    autoReconnect_->setChecked(manager->autoReconnect());
    autoReconnect_->setToolTip(tr("Waits 5 s after the first failure and up to 60 s after further ones"));
    form->addRow(autoReconnect_);
    restoreOnStartup_ = new QCheckBox(tr("Restore the connections at startup"));
    restoreOnStartup_->setChecked(manager->restoreOnStartup());
    restoreOnStartup_->setToolTip(tr("Connects what was connected when the GUI last exited"));
    form->addRow(restoreOnStartup_);

    // HTTP proxy, overrides http-proxy in the profiles
    const ProxySettings proxy = manager->proxy();
    useProxy_ = new QCheckBox(tr("Connect through an HTTP proxy"));
    useProxy_->setChecked(!proxy.host.isEmpty());
    form->addRow(useProxy_);
    proxyHost_ = new QLineEdit(proxy.host);
    proxyHost_->setPlaceholderText(tr("proxy.example.com"));
    proxyPort_ = new QSpinBox;
    proxyPort_->setRange(1, 65535);
    proxyPort_->setValue(proxy.port ? proxy.port : 8080);
    auto *proxyRow = new QWidget;
    auto *proxyLayout = new QHBoxLayout(proxyRow);
    proxyLayout->setContentsMargins(0, 0, 0, 0);
    proxyLayout->addWidget(proxyHost_, 1);
    proxyLayout->addWidget(new QLabel(tr("Port:")));
    proxyLayout->addWidget(proxyPort_);
    form->addRow(tr("Proxy:"), proxyRow);
    auto *proxyHint = new QLabel(tr("Used for all connections instead of the http-proxy of the profiles. "
                                    "Proxy credentials are asked for when the proxy needs them. "
                                    "Connections through a proxy don't use ovpn-dco-win."));
    proxyHint->setWordWrap(true);
    form->addRow(proxyHint);
    auto enableProxy = [this](bool on)
    {
        proxyHost_->setEnabled(on);
        proxyPort_->setEnabled(on);
    };
    connect(useProxy_, &QCheckBox::toggled, this, enableProxy);
    enableProxy(useProxy_->isChecked());

    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

void SettingsDialog::accept()
{
    const QString host = proxyHost_->text().trimmed();
    if (useProxy_->isChecked() && host.isEmpty())
    {
        QMessageBox::warning(this, windowTitle(), tr("Enter the proxy host name or address."));
        proxyHost_->setFocus();
        return;
    }
    ProxySettings proxy;
    if (useProxy_->isChecked())
    {
        proxy.host = host;
        proxy.port = static_cast<quint16>(proxyPort_->value());
    }
    manager_->setProxy(proxy);
    manager_->setAutoReconnect(autoReconnect_->isChecked());
    manager_->setRestoreOnStartup(restoreOnStartup_->isChecked());

    manager_->setWorkerPath(QDir::fromNativeSeparators(workerPath_->text().trimmed()));
    const QString dir = QDir::fromNativeSeparators(configDir_->text().trimmed());
    if (dir != manager_->configDir())
        manager_->setConfigDir(dir);
    QDialog::accept();
}
