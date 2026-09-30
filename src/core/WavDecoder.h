#pragma once

#include <QByteArray>

#include <cstdint>
#include <vector>

class WavDecoder {
public:
    static constexpr uint32_t DefaultTargetSampleRate = 16000;

    WavDecoder() = delete;

    [[nodiscard]] static bool decode(const QByteArray& wavData, std::vector<float>& outPcmf32,
                                     uint32_t targetSampleRate = DefaultTargetSampleRate);
};
