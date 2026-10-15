// Username/password, static and dynamic challenge entry.

#pragma once

#include "ManagementClient.h"

#include <QDialog>

class QCheckBox;
class QLineEdit;

class AuthDialog : public QDialog
{
    Q_OBJECT

  public:
    AuthDialog(const QString &profile,
               const OmiPasswordRequest &request,
               const QString &dynamicChallenge,
               const QString &savedUsername,
               QWidget *parent = nullptr);

    QString username() const;
    QString password() const;
    QString challengeResponse() const;
    bool rememberUsername() const;

  private:
    QLineEdit *username_ = nullptr;
    QLineEdit *password_ = nullptr;
    QLineEdit *response_ = nullptr;
    QCheckBox *remember_ = nullptr;
};
