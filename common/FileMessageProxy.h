#ifndef FILEMESSAGEPROXY_H
#define FILEMESSAGEPROXY_H

#include <QString>
#include <QByteArray>
#include <QFile>
#include <memory>
#include <QDebug>

class IFileContent {
public:
    virtual ~IFileContent() = default;
    virtual QByteArray getBytes() = 0;
    virtual QString getFileName() const = 0;
    virtual int getFileSize() const = 0;
};

class RealFileContent : public IFileContent {
private:
    QString m_filePath;
    QString m_fileName;
    int m_fileSize;
    QByteArray m_data;

public:
    RealFileContent(const QString& path, const QString& name, int size)
        : m_filePath(path), m_fileName(name), m_fileSize(size) {}

    QByteArray getBytes() override {
        if (m_data.isEmpty()) {
            qDebug() << "RealFileContent: Начинаю физическую загрузку файла с диска/сети...";
            QFile file(m_filePath);
            if (file.open(QIODevice::ReadOnly)) {
                m_data = file.readAll();
            }
        }
        return m_data;
    }

    QString getFileName() const override { return m_fileName; }
    int getFileSize() const override { return m_fileSize; }
};

class FileMessageProxy : public IFileContent {
private:
    QString m_filePath;
    QString m_fileName;
    int m_fileSize;
    std::unique_ptr<RealFileContent> m_realContent;

public:
    FileMessageProxy(const QString& path, const QString& name, int size)
        : m_filePath(path), m_fileName(name), m_fileSize(size) {}

    QByteArray getBytes() override {
        if (!m_realContent) {
            qDebug() << "FileMessageProxy: Создаю тяжелый объект RealFileContent...";
            m_realContent = std::make_unique<RealFileContent>(m_filePath, m_fileName, m_fileSize);
        }
        return m_realContent->getBytes();
    }

    QString getFileName() const override { return m_fileName; }
    int getFileSize() const override { return m_fileSize; }
};

#endif // FILEMESSAGEPROXY_H