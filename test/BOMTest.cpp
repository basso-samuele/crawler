#include <data/DataSource.h>
#include <FileDataSource.h>
#include <decode/BOM.h>

#include <cassert>

int undefinedEncodingTest() {
    std::filesystem::path path(BOM_TEST_INDEX);
    Crawler::DataSource* ds = new Crawler::FileDataSource(path);

    std::vector<std::byte> byteDataFromDisk(ds->Size());
    size_t readBytes;

    ds->Read(byteDataFromDisk.data(), &readBytes, ds->Size());
    assert(readBytes == ds->Size());

    Crawler::BOMSniffResult sniffedValue = Crawler::BOMSniff(byteDataFromDisk.data());
    assert(sniffedValue.encoding == Crawler::Encoding::UNDEFINED);
    assert(sniffedValue.confidence == Crawler::Confidence::UNDEFINED);

    return 0;
}

int utf8EncodingTest() {
    std::filesystem::path path(BOM_TEST_UTF8);
    Crawler::DataSource* ds = new Crawler::FileDataSource(path);

    std::vector<std::byte> byteDataFromDisk(ds->Size());
    size_t readBytes;

    ds->Read(byteDataFromDisk.data(), &readBytes, ds->Size());
    assert(readBytes == ds->Size());

    Crawler::BOMSniffResult sniffedValue = Crawler::BOMSniff(byteDataFromDisk.data());
    assert(sniffedValue.encoding == Crawler::Encoding::UTF8);
    assert(sniffedValue.confidence == Crawler::Confidence::CERTAIN);

    return 0;
}

int utf16beEncodingTest() {
    std::filesystem::path path(BOM_TEST_UTF16BE);
    Crawler::DataSource* ds = new Crawler::FileDataSource(path);

    std::vector<std::byte> byteDataFromDisk(ds->Size());
    size_t readBytes;

    ds->Read(byteDataFromDisk.data(), &readBytes, ds->Size());
    assert(readBytes == ds->Size());

    Crawler::BOMSniffResult sniffedValue = Crawler::BOMSniff(byteDataFromDisk.data());
    assert(sniffedValue.encoding == Crawler::Encoding::UTF16BE);
    assert(sniffedValue.confidence == Crawler::Confidence::CERTAIN);

    return 0;
}

int utf16leEncodingTest() {
    std::filesystem::path path(BOM_TEST_UTF16LE);
    Crawler::DataSource* ds = new Crawler::FileDataSource(path);

    std::vector<std::byte> byteDataFromDisk(ds->Size());
    size_t readBytes;

    ds->Read(byteDataFromDisk.data(), &readBytes, ds->Size());
    assert(readBytes == ds->Size());

    Crawler::BOMSniffResult sniffedValue = Crawler::BOMSniff(byteDataFromDisk.data());
    assert(sniffedValue.encoding == Crawler::Encoding::UTF16LE);
    assert(sniffedValue.confidence == Crawler::Confidence::CERTAIN);

    return 0;
}

int main(int argc, char** argv) {
    int testResult = 0;
    testResult |= undefinedEncodingTest();
    testResult |= utf8EncodingTest();
    testResult |= utf16beEncodingTest();
    testResult |= utf16leEncodingTest();
    return testResult;
}