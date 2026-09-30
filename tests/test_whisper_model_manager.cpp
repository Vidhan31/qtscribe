#include "ModelDownloader.h"
#include "WhisperModelManager.h"

#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;

class WhisperModelManagerTest : public QObject {
    Q_OBJECT

private slots:
    void testPresetsCountAndMetadata() {
        WhisperModelManager manager;
        QCOMPARE(manager.modelCount(), 10);
        QCOMPARE(manager.models().size(), 10);

        for (const auto& item : manager.models()) {
            QVERIFY(!item.id.isEmpty());
            QVERIFY(!item.name.isEmpty());
            QVERIFY(!item.fileName.isEmpty());
            QVERIFY(item.fileName.endsWith(u".bin"_s));
            QVERIFY(item.downloadUrl.startsWith(u"https://huggingface.co/"_s));
            QVERIFY(item.sizeBytes > 0);
            QVERIFY(!item.sizeFormatted.isEmpty());
            QVERIFY(!item.memoryFormatted.isEmpty());
            QVERIFY(!item.description.isEmpty());
        }

        const int tinyIdx = manager.findModelIndex(u"tiny.en"_s);
        QVERIFY(tinyIdx >= 0);

        const auto modelOpt = manager.model(u"tiny.en"_s);
        QVERIFY(modelOpt.has_value());
        QCOMPARE(modelOpt->id, u"tiny.en"_s);
        QCOMPARE(modelOpt->fileName, u"ggml-tiny.en.bin"_s);

        const auto invalidOpt = manager.model(u"non_existent_model"_s);
        QVERIFY(!invalidOpt.has_value());
        QCOMPARE(manager.findModelIndex(u"non_existent_model"_s), -1);
    }

    void testModelDownloaderBasics() {
        ModelDownloader downloader;
        QVERIFY(!downloader.isDownloadingAny());
        QVERIFY(downloader.downloadingModelId().isEmpty());
        QCOMPARE(downloader.downloadProgress(), 0.0);
        QCOMPARE(downloader.downloadBytesReceived(), 0);
        QCOMPARE(downloader.downloadTotalBytes(), 0);
        QVERIFY(downloader.downloadSpeedFormatted().isEmpty());
        QVERIFY(downloader.downloadBytesFormatted().isEmpty());

        downloader.cancelDownload(u"non_existent"_s);
        QVERIFY(!downloader.isDownloadingAny());

        QCOMPARE(ModelDownloader::formatBytes(0), u"0 B"_s);
        QCOMPARE(ModelDownloader::formatBytes(512), u"512 B"_s);
        QCOMPARE(ModelDownloader::formatBytes(1024), u"1.0 KiB"_s);
        QCOMPARE(ModelDownloader::formatBytes(1024 * 1024), u"1.0 MiB"_s);
        QCOMPARE(ModelDownloader::formatBytes(1024LL * 1024LL * 1024LL), u"1.00 GiB"_s);
    }

    void testCustomDirectoryAndScanning() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        WhisperModelManager manager;
        manager.setModelsDirectory(tempDir.path());
        QCOMPARE(manager.modelsDirectory(), tempDir.path());

        QVERIFY(!manager.isModelInstalled(u"tiny.en"_s));

        const QString filePath = tempDir.path() + u"/ggml-tiny.en.bin"_s;
        {
            QFile file(filePath);
            QVERIFY(file.open(QIODevice::WriteOnly));
            constexpr quint32 kGgmlMagic = 0x67676d6c;
            file.write(reinterpret_cast<const char*>(&kGgmlMagic), sizeof(kGgmlMagic));
            file.write("dummy_content_bytes");
            file.close();
        }

        QSignalSpy statusSpy(&manager, &WhisperModelManager::modelStatusChanged);
        QSignalSpy dataSpy(&manager, &QAbstractItemModel::dataChanged);
        manager.scanInstalledModels();

        QVERIFY(manager.isModelInstalled(u"tiny.en"_s));
        QCOMPARE(manager.getModelPath(u"tiny.en"_s), filePath);
        QVERIFY(statusSpy.count() >= 1);
        QVERIFY(dataSpy.count() >= 1);
    }

    void testModelSelection() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        WhisperModelManager manager;
        manager.setModelsDirectory(tempDir.path());

        const QString filePath = tempDir.path() + u"/ggml-base.en.bin"_s;
        {
            QFile file(filePath);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("dummy_model_data");
            file.close();
        }
        manager.scanInstalledModels();
        QVERIFY(manager.isModelInstalled(u"base.en"_s));

        QSignalSpy selectSpy(&manager, &WhisperModelManager::selectedModelChanged);
        manager.setSelectedModelId(u"base.en"_s);

        QCOMPARE(manager.selectedModelId(), u"base.en"_s);
        QCOMPARE(manager.selectedModelPath(), filePath);
        QVERIFY(manager.isSelectedModelInstalled());
        QCOMPARE(selectSpy.count(), 1);
    }

    void testModelDeletion() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        WhisperModelManager manager;
        manager.setModelsDirectory(tempDir.path());

        const QString filePath = tempDir.path() + u"/ggml-tiny.en.bin"_s;
        {
            QFile file(filePath);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("dummy");
            file.close();
        }

        manager.scanInstalledModels();
        QVERIFY(manager.isModelInstalled(u"tiny.en"_s));

        QSignalSpy statusSpy(&manager, &WhisperModelManager::modelStatusChanged);
        const bool deleted = manager.deleteModel(u"tiny.en"_s);

        QVERIFY(deleted);
        QVERIFY(!QFile::exists(filePath));
        QVERIFY(!manager.isModelInstalled(u"tiny.en"_s));
        QCOMPARE(statusSpy.count(), 1);
    }

    void testOrphanPartFileCleanup() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());

        const QString partFile = tempDir.path() + u"/ggml-base.en.bin.part"_s;
        {
            QFile file(partFile);
            QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("incomplete_chunk");
            file.close();
        }
        QVERIFY(QFile::exists(partFile));

        WhisperModelManager manager;
        manager.setModelsDirectory(tempDir.path());

        QVERIFY(!QFile::exists(partFile));
    }

    void testQAbstractListModelInterface() {
        WhisperModelManager manager;
        QCOMPARE(manager.rowCount(), 10);

        const QModelIndex firstIndex = manager.index(0);
        QVERIFY(firstIndex.isValid());
        QCOMPARE(manager.data(firstIndex, WhisperModelManager::IdRole).toString(), u"tiny.en"_s);
        QCOMPARE(manager.data(firstIndex, WhisperModelManager::FileNameRole).toString(), u"ggml-tiny.en.bin"_s);
        QCOMPARE(manager.data(firstIndex, WhisperModelManager::CanDeleteRole).toBool(), false);

        const auto roles = manager.roleNames();
        QCOMPARE(roles.value(WhisperModelManager::IdRole), "modelId");
        QCOMPARE(roles.value(WhisperModelManager::NameRole), "name");
        QCOMPARE(roles.value(WhisperModelManager::FileNameRole), "fileName");
        QCOMPARE(roles.value(WhisperModelManager::DownloadUrlRole), "downloadUrl");
        QCOMPARE(roles.value(WhisperModelManager::SizeBytesRole), "sizeBytes");
        QCOMPARE(roles.value(WhisperModelManager::SizeFormattedRole), "sizeFormatted");
        QCOMPARE(roles.value(WhisperModelManager::IsInstalledRole), "isInstalled");
        QCOMPARE(roles.value(WhisperModelManager::IsSelectedRole), "isSelected");
        QCOMPARE(roles.value(WhisperModelManager::IsDownloadingRole), "isDownloading");
    }

    void testFormatBytes() {
        QCOMPARE(WhisperModelManager::formatBytes(0), u"0 B"_s);
        QCOMPARE(WhisperModelManager::formatBytes(512), u"512 B"_s);
        QCOMPARE(WhisperModelManager::formatBytes(1024), u"1.0 KiB"_s);
        QCOMPARE(WhisperModelManager::formatBytes(1024 * 1024), u"1.0 MiB"_s);
        QCOMPARE(WhisperModelManager::formatBytes(1024LL * 1024LL * 1024LL), u"1.00 GiB"_s);
    }
};

QTEST_GUILESS_MAIN(WhisperModelManagerTest)
#include "test_whisper_model_manager.moc"
