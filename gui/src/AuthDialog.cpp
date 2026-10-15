#include "AuthDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

AuthDialog::AuthDialog(const QString &profile,
                       const OmiPasswordRequest &request,
                       const QString &dynamicChallenge,
                       const QString &savedUsername,
                       const QString &savedPassword,
                       bool canSavePassword,
                       QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("%1 - %2").arg(profile, request.type == "Auth" ? tr("Sign in") : request.type));
    setWindowFlag(Qt::WindowStaysOnTopHint);

    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout;
    layout->addLayout(form);

    if (!dynamicChallenge.isEmpty())
    {
        // CRV1 challenge: only the response is asked for
        auto *text = new QLabel(dynamicChallenge);
        text->setTextFormat(Qt::PlainText);
        text->setWordWrap(true);
        form->addRow(text);
        response_ = new QLineEdit;
        form->addRow(tr("Response:"), response_);
    }
    else
    {
        if (!request.passwordOnly)
        {
            username_ = new QLineEdit(savedUsername);
            form->addRow(tr("Username:"), username_);
        }
        password_ = new QLineEdit(savedPassword);
        password_->setEchoMode(QLineEdit::Password);
        form->addRow(tr("Password:"), password_);

        if (!request.staticChallenge.isEmpty())
        {
            response_ = new QLineEdit;
            if (!request.staticChallengeEcho)
                response_->setEchoMode(QLineEdit::Password);
            auto *label = new QLabel(request.staticChallenge + ':');
            label->setTextFormat(Qt::PlainText);
            label->setWordWrap(true);
            form->addRow(label, response_);
        }

        if (username_)
        {
            remember_ = new QCheckBox(tr("Remember username"));
            remember_->setChecked(!savedUsername.isEmpty());
            form->addRow(remember_);
        }

        if (canSavePassword && request.type == "Auth")
        {
            rememberPassword_ = new QCheckBox(tr("Remember password"));
            rememberPassword_->setChecked(!savedPassword.isEmpty());
            if (remember_)
            {
                // a saved password is useless without the username
                connect(rememberPassword_, &QCheckBox::toggled, remember_, [this](bool on)
                        {
                    if (on)
                        remember_->setChecked(true); });
                connect(remember_, &QCheckBox::toggled, rememberPassword_, [this](bool on)
                        {
                    if (!on)
                        rememberPassword_->setChecked(false); });
            }
            form->addRow(rememberPassword_);
        }
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    if (username_ && username_->text().isEmpty())
        username_->setFocus();
    else if (password_ && password_->text().isEmpty())
        password_->setFocus();
    else if (response_)
        response_->setFocus();
    else if (password_)
        password_->setFocus();

    setMinimumWidth(360);
}

QString AuthDialog::username() const
{
    return username_ ? username_->text() : QString();
}

QString AuthDialog::password() const
{
    return password_ ? password_->text() : QString();
}

QString AuthDialog::challengeResponse() const
{
    return response_ ? response_->text() : QString();
}

bool AuthDialog::rememberUsername() const
{
    return remember_ && remember_->isChecked();
}

bool AuthDialog::rememberPassword() const
{
    return rememberPassword_ && rememberPassword_->isChecked();
}
