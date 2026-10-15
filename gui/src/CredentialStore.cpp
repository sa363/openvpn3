#include "CredentialStore.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <dpapi.h>
#endif

namespace {

#ifdef Q_OS_WIN
// ties the blobs to this application, other DPAPI users can't open them
// by accident
const char entropy[] = "ovpn3-gui saved password v1";

DATA_BLOB entropyBlob()
{
    DATA_BLOB b;
    b.pbData = reinterpret_cast<BYTE *>(const_cast<char *>(entropy));
    b.cbData = sizeof(entropy) - 1;
    return b;
}
#endif

} // namespace

namespace CredentialStore {

bool isAvailable()
{
#ifdef Q_OS_WIN
    return true;
#else
    return false;
#endif
}

QByteArray protect(const QString &secret)
{
#ifdef Q_OS_WIN
    QByteArray plain = secret.toUtf8();
    DATA_BLOB in{static_cast<DWORD>(plain.size()), reinterpret_cast<BYTE *>(plain.data())};
    DATA_BLOB ent = entropyBlob();
    DATA_BLOB out{};
    const BOOL ok = ::CryptProtectData(&in, L"ovpn3-gui", &ent, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out);
    ::SecureZeroMemory(plain.data(), plain.size());
    if (!ok)
        return QByteArray();
    QByteArray ret(reinterpret_cast<const char *>(out.pbData), static_cast<qsizetype>(out.cbData));
    ::LocalFree(out.pbData);
    return ret;
#else
    Q_UNUSED(secret);
    return QByteArray();
#endif
}

QString unprotect(const QByteArray &blob)
{
#ifdef Q_OS_WIN
    if (blob.isEmpty())
        return QString();
    QByteArray copy = blob;
    DATA_BLOB in{static_cast<DWORD>(copy.size()), reinterpret_cast<BYTE *>(copy.data())};
    DATA_BLOB ent = entropyBlob();
    DATA_BLOB out{};
    if (!::CryptUnprotectData(&in, nullptr, &ent, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &out))
        return QString();
    const QString ret = QString::fromUtf8(reinterpret_cast<const char *>(out.pbData), static_cast<qsizetype>(out.cbData));
    ::SecureZeroMemory(out.pbData, out.cbData);
    ::LocalFree(out.pbData);
    return ret;
#else
    Q_UNUSED(blob);
    return QString();
#endif
}

} // namespace CredentialStore
