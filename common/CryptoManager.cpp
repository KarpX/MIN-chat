#include "CryptoManager.h"

void CryptoManager::setStrategy(std::unique_ptr<IEncryptionStrategy> s) { m_strategy = std::move(s); }
QByteArray CryptoManager::encryptData(const QByteArray &d, const QByteArray &k) { return m_strategy ? m_strategy->encrypt(d, k) : d; }
QByteArray CryptoManager::decryptData(const QByteArray &d, const QByteArray &k) { return m_strategy ? m_strategy->decrypt(d, k) : d; }