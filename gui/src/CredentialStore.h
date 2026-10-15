// Encryption of saved passwords. On Windows the data is protected with
// DPAPI for the current user, so only this user on this machine can
// decrypt it.

#pragma once

#include <QByteArray>
#include <QString>

namespace CredentialStore {

bool isAvailable();

// return an empty array on failure
QByteArray protect(const QString &secret);

// return a null string on failure
QString unprotect(const QByteArray &blob);

} // namespace CredentialStore
