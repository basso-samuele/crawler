#include <utils/Log.h>
#include <data/DataSource.h>
#include <FileDataSource.h>

#include <cstddef>

namespace Crawler
{

    enum class Confidence
    {
        TENTATIVE, CERTAIN, IRRELEVANT, UNDEFINED
    };

    enum class Encoding
    {
        UTF8, UTF16BE, UTF16LE, UNDEFINED
    };

    constexpr uint16_t GenKey2(std::byte b1, std::byte b2) {
        return (static_cast<uint16_t>(b1) << 8) |
                static_cast<uint16_t>(b2);
    }

    constexpr uint32_t GenKey3(std::byte b1, std::byte b2, std::byte b3) {
        return (static_cast<uint32_t>(b1) << 16) |
               (static_cast<uint32_t>(b2) << 8) |
                static_cast<uint32_t>(b3);
    }

    struct BOMSniffResult
    {
        Encoding encoding;
        Confidence confidence;
    };

    BOMSniffResult BOMSniff(std::byte* data) {
        constexpr uint16_t UTF16BE_PAT = GenKey2(std::byte{0xFE}, std::byte{0xFF});
        constexpr uint16_t UTF16LE_PAT = GenKey2(std::byte{0xFF}, std::byte{0xFE});

        const uint16_t dataKey2 = GenKey2(data[0], data[1]);

        switch (dataKey2) {
            case UTF16BE_PAT:
                return { Encoding::UTF16BE, Confidence::CERTAIN };
            case UTF16LE_PAT:
                return { Encoding::UTF16LE, Confidence::CERTAIN };
        }

        constexpr uint32_t UTF8_PARTIAL_PAT = GenKey3(std::byte{0xEF}, std::byte{0xBB}, std::byte{0xBF});

        const uint32_t dataKey3 = GenKey3(data[0], data[1], data[2]);

        switch (dataKey3) {
            case UTF8_PARTIAL_PAT:
                return { Encoding::UTF8, Confidence::CERTAIN };
        }

        return { Encoding::UNDEFINED, Confidence::UNDEFINED };
    }

    constexpr std::string_view ToString(Encoding enc) {
        switch (enc) {
            case Encoding::UTF8:
                return "UTF8";
            case Encoding::UTF16BE:
                return "UTF16BE";
            case Encoding::UTF16LE:
                return "UTF16LE";
            case Encoding::UNDEFINED:
                return "UNDEFINED";
        }
        return "UNKNOWN";
    }

    constexpr std::string_view ToString(Confidence enc) {
        switch (enc) {
            case Confidence::TENTATIVE:
                return "TENTATIVE";
            case Confidence::CERTAIN:
                return "CERTAIN";
            case Confidence::IRRELEVANT:
                return "IRRELEVANT";
            case Confidence::UNDEFINED:
                return "UNDEFINED";
        }
        return "UNKNOWN";
    }
}

int main(int argc, char** argv) {
    std::filesystem::path path("./assets/utf16le.html");
    Crawler::DataSource* ds = new Crawler::FileDataSource(path);
    CLIENT_TRACE("Data source size: {}.", ds->Size());

    std::vector<std::byte> byteDataFromDisk(ds->Size());
    size_t readBytes;

    ds->Read(byteDataFromDisk.data(), &readBytes, ds->Size());

    if (readBytes == ds->Size()) {
        Crawler::BOMSniffResult sniffedValue = Crawler::BOMSniff(byteDataFromDisk.data());
        CLIENT_TRACE("\n\tEncoding: {}\n\tConfidence: {}", Crawler::ToString(sniffedValue.encoding), Crawler::ToString(sniffedValue.confidence));
    }

    return 0;
}