#pragma once

#include <QDialog>

class ConnectionManager;
class QCheckBox;
class QLineEdit;
class QSpinBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit SettingsDialog(ConnectionManager *manager, QWidget *parent = nullptr);

    void accept() override;

  private:
    ConnectionManager *manager_;
    QLineEdit *configDir_ = nullptr;
    QLineEdit *workerPath_ = nullptr;
    QCheckBox *useProxy_ = nullptr;
    QLineEdit *proxyHost_ = nullptr;
    QSpinBox *proxyPort_ = nullptr;
};
