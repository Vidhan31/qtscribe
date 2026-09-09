#include <QAudioFormat>
#include <QBuffer>
#include <QByteArray>
#include <QCoreApplication>
#include <QDataStream>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include "WavDecoder.h"
#include "WhisperModelManager.h"
#include "WhisperSttClient.h"
#include "WhisperWorker.h"

#include <cmath>
#include <numbers>
#include <vector>

using namespace Qt::StringLiterals;

namespace {
QByteArray createWav(uint32_t sampleRate, uint16_t numChannels, uint16_t bitsPerSample, uint16_t audioFormat,
                     const QByteArray& pcmData) {
    const uint32_t dataSize = static_cast<uint32_t>(pcmData.size());
    const uint32_t fileSize = 36 + dataSize;
    const uint16_t blockAlign = numChannels * (bitsPerSample / 8);
    const uint32_t byteRate = sampleRate * blockAlign;

    QByteArray wav;
    wav.reserve(44 + pcmData.size());
    QDataStream stream(&wav, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);

    stream.writeRawData("RIFF", 4);
    stream << fileSize;
    stream.writeRawData("WAVE", 4);

    stream.writeRawData("fmt ", 4);
    stream << quint32(16);
    stream << audioFormat;
    stream << numChannels;
    stream << sampleRate;
    stream << byteRate;
    stream << blockAlign;
    stream << bitsPerSample;

    stream.writeRawData("data", 4);
    stream << dataSize;

    wav.append(pcmData);
    return wav;
}
} // namespace

class TestWhisperPipeline : public QObject {
    Q_OBJECT

private slots:
    void testAudioResampling_48kFloatTo16kMono() {
        constexpr int sampleRate = 48000;
        constexpr int frameCount = 48000;
        QByteArray pcm;
        pcm.resize(frameCount * sizeof(float));
        auto* floatPtr = reinterpret_cast<float*>(pcm.data());

        for (int i = 0; i < frameCount; ++i) {
            floatPtr[i] = 0.5f * std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * i / sampleRate);
        }

        const QByteArray wav = createWav(sampleRate, 1, 32, 3, pcm);
        std::vector<float> pcmf32;
        const bool success = WavDecoder::decode(wav, pcmf32);

        QVERIFY(success);
        QVERIFY(std::abs(static_cast<int>(pcmf32.size()) - 16000) <= 5);

        for (float s : pcmf32) {
            QVERIFY(s >= -1.05f && s <= 1.05f);
        }
    }

    void testAudioResampling_44kStereoTo16kMono() {
        constexpr int sampleRate = 44100;
        constexpr int frameCount = 44100;
        QByteArray pcm;
        pcm.resize(frameCount * 2 * sizeof(int16_t));
        auto* shortPtr = reinterpret_cast<int16_t*>(pcm.data());

        for (int i = 0; i < frameCount; ++i) {
            const int16_t val = static_cast<int16_t>(16000.0f * std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * i / sampleRate));
            shortPtr[2 * i] = val;
            shortPtr[2 * i + 1] = val;
        }

        const QByteArray wav = createWav(sampleRate, 2, 16, 1, pcm);
        std::vector<float> pcmf32;
        const bool success = WavDecoder::decode(wav, pcmf32);

        QVERIFY(success);
        QVERIFY(std::abs(static_cast<int>(pcmf32.size()) - 16000) <= 10);

        for (float s : pcmf32) {
            QVERIFY(s >= -1.05f && s <= 1.05f);
        }
    }

    void testAudioResampling_48kInt32To16kMono() {
        constexpr int sampleRate = 48000;
        constexpr int frameCount = 48000;
        QByteArray pcm;
        pcm.resize(frameCount * sizeof(int32_t));
        auto* intPtr = reinterpret_cast<int32_t*>(pcm.data());

        for (int i = 0; i < frameCount; ++i) {
            intPtr[i] = static_cast<int32_t>(1000000000.0f * std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * i / sampleRate));
        }

        const QByteArray wav = createWav(sampleRate, 1, 32, 1, pcm);
        std::vector<float> pcmf32;
        const bool success = WavDecoder::decode(wav, pcmf32);

        QVERIFY(success);
        QVERIFY(std::abs(static_cast<int>(pcmf32.size()) - 16000) <= 5);
    }

    void testAudioPassthrough_16kInt16Mono() {
        constexpr int sampleRate = 16000;
        constexpr int frameCount = 16000;
        QByteArray pcm;
        pcm.resize(frameCount * sizeof(int16_t));
        auto* shortPtr = reinterpret_cast<int16_t*>(pcm.data());

        for (int i = 0; i < frameCount; ++i) {
            shortPtr[i] = static_cast<int16_t>(16000.0f * std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * i / sampleRate));
        }

        const QByteArray wav = createWav(sampleRate, 1, 16, 1, pcm);
        std::vector<float> pcmf32;
        const bool success = WavDecoder::decode(wav, pcmf32);

        QVERIFY(success);
        QCOMPARE(static_cast<int>(pcmf32.size()), 16000);
    }

    void testAudioResampling_8BitUnsignedTo16kMono() {
        constexpr int sampleRate = 22050;
        constexpr int frameCount = 22050;
        QByteArray pcm;
        pcm.resize(frameCount);
        auto* u8Ptr = reinterpret_cast<uint8_t*>(pcm.data());

        for (int i = 0; i < frameCount; ++i) {
            const float sineVal = std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * i / sampleRate);
            u8Ptr[i] = static_cast<uint8_t>(std::clamp(128.0f + 100.0f * sineVal, 0.0f, 255.0f));
        }

        const QByteArray wav = createWav(sampleRate, 1, 8, 1, pcm);
        std::vector<float> pcmf32;
        const bool success = WavDecoder::decode(wav, pcmf32);

        QVERIFY(success);
        QVERIFY(std::abs(static_cast<int>(pcmf32.size()) - 16000) <= 10);
        for (float s : pcmf32) {
            QVERIFY(s >= -1.05f && s <= 1.05f);
        }
    }

    void testWavDecoder_CustomTargetSampleRate() {
        constexpr int sampleRate = 48000;
        constexpr int frameCount = 48000;
        QByteArray pcm;
        pcm.resize(frameCount * sizeof(int16_t));
        auto* shortPtr = reinterpret_cast<int16_t*>(pcm.data());

        for (int i = 0; i < frameCount; ++i) {
            shortPtr[i] = static_cast<int16_t>(16000.0f * std::sin(2.0f * std::numbers::pi_v<float> * 440.0f * i / sampleRate));
        }

        const QByteArray wav = createWav(sampleRate, 1, 16, 1, pcm);
        std::vector<float> pcmf32;
        const bool success = WavDecoder::decode(wav, pcmf32, 24000);

        QVERIFY(success);
        QVERIFY(std::abs(static_cast<int>(pcmf32.size()) - 24000) <= 5);
    }

    void testWavDecoder_InvalidAndTruncated() {
        std::vector<float> pcmf32;

        QVERIFY(!WavDecoder::decode(QByteArray(), pcmf32));
        QVERIFY(!WavDecoder::decode(QByteArray("RIFF1234"), pcmf32));
        QVERIFY(!WavDecoder::decode(QByteArray("NOT_A_WAV_HEADER_DATA_STREAM"), pcmf32));

        const QByteArray validHeaderWav = createWav(16000, 1, 16, 1, QByteArray(320, 0));
        QVERIFY(!WavDecoder::decode(validHeaderWav, pcmf32, 0));
    }

    void testModelSelectionSynchronization() {
        WhisperModelManager manager;
        WhisperSttClient client;

        client.setModelManager(&manager);

        const QString initialId = manager.selectedModelId();
        QVERIFY(!initialId.isEmpty());
        QCOMPARE(client.modelPath(), manager.selectedModelPath());

        QVERIFY(!client.isModelLoaded());
        QVERIFY(!client.isReady());

        if (manager.rowCount() > 1) {
            const QString secondId = manager.data(manager.index(1), WhisperModelManager::IdRole).toString();
            manager.setSelectedModelId(secondId);
            QCOMPARE(manager.selectedModelId(), secondId);
            QCOMPARE(client.modelPath(), manager.selectedModelPath());
        }
    }

    void testStaleModelLoadHandling() {
        WhisperSttClient client;

        QVERIFY(!client.isModelLoaded());
        QVERIFY(client.loadedModelPath().isEmpty());

        client.loadModel(u"/nonexistent/dummy/model.bin"_s);
        QVERIFY(!client.isModelLoaded());
        QVERIFY(client.loadedModelPath().isEmpty());

        client.unloadModel();
        QVERIFY(!client.isModelLoaded());
        QVERIFY(client.loadedModelPath().isEmpty());
    }

    void testWorkerAbortLogic() {
        WhisperWorker worker;

        QVERIFY(!worker.isAborted(1));
        QVERIFY(!worker.isAborted(0));
        QVERIFY(!worker.isLoadAborted(1));
        QVERIFY(!worker.isLoadAborted(0));

        worker.cancel(42);
        QVERIFY(worker.isAborted(42));
        QVERIFY(worker.isAborted(10));
        QVERIFY(!worker.isAborted(50));
        QVERIFY(!worker.isAborted(0));
        QVERIFY(!worker.isLoadAborted(1));

        worker.cancelLoad(5);
        QVERIFY(worker.isLoadAborted(5));
        QVERIFY(worker.isLoadAborted(2));
        QVERIFY(!worker.isLoadAborted(10));
        QVERIFY(!worker.isLoadAborted(0));

        worker.cancel(0);
        QVERIFY(worker.isAborted(0));
        QVERIFY(worker.isAborted(100));
        QVERIFY(worker.isLoadAborted(0));
        QVERIFY(worker.isLoadAborted(100));

        worker.resetAbort();
        QVERIFY(!worker.isAborted(0));
        QVERIFY(!worker.isLoadAborted(0));
        QVERIFY(worker.isAborted(42));
        QVERIFY(worker.isLoadAborted(5));
        QVERIFY(!worker.isAborted(50));
        QVERIFY(!worker.isLoadAborted(10));
    }

    void testVulkanSupportReporting() {
        WhisperSttClient client;
#if defined(GGML_USE_VULKAN)
        QVERIFY(client.isVulkanSupported());
#else
        QVERIFY(!client.isVulkanSupported());
#endif
    }

    void testClientLifecycleAndGracefulShutdown() {
        for (int i = 0; i < 3; ++i) {
            auto client = std::make_unique<WhisperSttClient>();
            client->loadModel(u"/tmp/nonexistent_test_model.bin"_s);
            client->cancel();
        }
    }
};

QTEST_GUILESS_MAIN(TestWhisperPipeline)
#include "test_whisper_pipeline.moc"
