#include "WavDecoder.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#define MA_NO_DEVICE_IO
#define MA_NO_THREADING
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

bool WavDecoder::decode(const QByteArray& wavData, std::vector<float>& outPcmf32, uint32_t targetSampleRate) {
    if (wavData.size() < 12 || targetSampleRate == 0) {
        return false;
    }

    ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 1, targetSampleRate);
    ma_decoder decoder;

    if (ma_decoder_init_memory(wavData.constData(), static_cast<size_t>(wavData.size()), &decoderConfig, &decoder) ==
        MA_SUCCESS) {
        ma_uint64 frameCount = 0;
        if (ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount) == MA_SUCCESS && frameCount > 0) {
            outPcmf32.resize(static_cast<size_t>(frameCount));
            ma_uint64 framesRead = 0;
            if (ma_decoder_read_pcm_frames(&decoder, outPcmf32.data(), frameCount, &framesRead) == MA_SUCCESS &&
                framesRead > 0) {
                outPcmf32.resize(static_cast<size_t>(framesRead));
                ma_decoder_uninit(&decoder);
                return true;
            }
        }
        ma_decoder_uninit(&decoder);
    }

    const char* data = wavData.constData();
    if (std::memcmp(data, "RIFF", 4) != 0 || std::memcmp(data + 8, "WAVE", 4) != 0) {
        return false;
    }

    int pos = 12;
    int dataOffset = -1;
    uint32_t dataBytes = 0;
    uint16_t audioFormat = 1;
    uint16_t numChannels = 1;
    uint32_t sampleRate = targetSampleRate;
    uint16_t bitsPerSample = 16;

    while (pos + 8 <= wavData.size()) {
        const char* chunkId = data + pos;
        uint32_t chunkSize = 0;
        std::memcpy(&chunkSize, data + pos + 4, 4);
        pos += 8;

        if (std::memcmp(chunkId, "fmt ", 4) == 0 && chunkSize >= 16 && pos + 16 <= wavData.size()) {
            std::memcpy(&audioFormat, data + pos, 2);
            std::memcpy(&numChannels, data + pos + 2, 2);
            std::memcpy(&sampleRate, data + pos + 4, 4);
            std::memcpy(&bitsPerSample, data + pos + 14, 2);
        } else if (std::memcmp(chunkId, "data", 4) == 0) {
            dataOffset = pos;
            dataBytes = std::min<uint32_t>(chunkSize, static_cast<uint32_t>(wavData.size() - pos));
            break;
        }

        pos += chunkSize + (chunkSize % 2);
    }

    if (dataOffset < 0 || dataBytes < sizeof(int16_t)) {
        if (wavData.size() > 44) {
            dataOffset = 44;
            dataBytes = static_cast<uint32_t>(wavData.size() - 44);
        } else {
            return false;
        }
    }

    if (numChannels == 0) {
        numChannels = 1;
    }

    const int bytesPerSample = bitsPerSample / 8;
    if (bytesPerSample <= 0) {
        return false;
    }

    const int totalSamples = static_cast<int>(dataBytes / bytesPerSample);
    const int frameCount = totalSamples / numChannels;
    if (frameCount <= 0) {
        return false;
    }

    std::vector<float> monoPcm(frameCount);
    const char* samplePtr = data + dataOffset;

    for (int f = 0; f < frameCount; ++f) {
        float sum = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch) {
            const int sampleIdx = (f * numChannels + ch) * bytesPerSample;
            if (audioFormat == 3 && bitsPerSample == 32) {
                float val = 0.0f;
                std::memcpy(&val, samplePtr + sampleIdx, sizeof(float));
                sum += val;
            } else if (bitsPerSample == 32) {
                int32_t val = 0;
                std::memcpy(&val, samplePtr + sampleIdx, sizeof(int32_t));
                sum += static_cast<float>(val) / 2147483648.0f;
            } else if (bitsPerSample == 16) {
                int16_t val = 0;
                std::memcpy(&val, samplePtr + sampleIdx, sizeof(int16_t));
                sum += static_cast<float>(val) / 32768.0f;
            } else if (bitsPerSample == 8) {
                const uint8_t val = static_cast<uint8_t>(samplePtr[sampleIdx]);
                sum += (static_cast<float>(val) - 128.0f) / 128.0f;
            }
        }
        monoPcm[f] = std::clamp(sum / static_cast<float>(numChannels), -1.0f, 1.0f);
    }

    if (sampleRate != targetSampleRate && sampleRate > 0) {
        const double ratio = static_cast<double>(sampleRate) / static_cast<double>(targetSampleRate);
        const int outSampleCount = static_cast<int>(static_cast<double>(frameCount) / ratio);
        outPcmf32.resize(outSampleCount);
        for (int i = 0; i < outSampleCount; ++i) {
            const double srcIdx = i * ratio;
            const int idx0 = static_cast<int>(srcIdx);
            const int idx1 = std::min(idx0 + 1, frameCount - 1);
            const float frac = static_cast<float>(srcIdx - idx0);
            outPcmf32[i] = std::lerp(monoPcm[idx0], monoPcm[idx1], frac);
        }
    } else {
        outPcmf32 = std::move(monoPcm);
    }

    return !outPcmf32.empty();
}
