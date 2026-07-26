#include "Test.hpp"

#include "Encoding.hpp"

namespace Test
{

inline constexpr Crawler::ByteSpecPolicy M = Crawler::ByteSpecPolicy::MANDATORY;
inline constexpr Crawler::ByteSpecPolicy O = Crawler::ByteSpecPolicy::OPTIONAL;

void EncodingMatchSingleElementMandatoryTest() {
    std::vector<uint8_t> data {0x01, 0x02, 0x03, 0x04};
    Crawler::Pattern pattern({
        { 0x01, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x02, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x03, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x04, Crawler::ByteSpecPolicy::MANDATORY }
    });
    CRAWLER_ASSERT_TRUE(pattern.Match(data.data(), data.size()));
}

void EncodingMatchSetMandatoryTest() {
    std::vector<uint8_t> data {0x01, 0x02, 0x03, 0x04};
    Crawler::Pattern pattern({
        { { 0x01, 0x02 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x01, 0x02 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x03, 0x04 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x03, 0x04 }, Crawler::ByteSpecPolicy::MANDATORY }
    });
    CRAWLER_ASSERT_TRUE(pattern.Match(data.data(), data.size()));
}

void EncodingMatchOptionalTest() {
    std::vector<uint8_t> data {0x01, 0x02, 0x03, 0x04};
    Crawler::Pattern pattern({
        { { 0x01, 0x02 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x05, 0x06 }, Crawler::ByteSpecPolicy::OPTIONAL },
        { 0x02, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x03, 0x04 }, Crawler::ByteSpecPolicy::MANDATORY },
        { { 0x05, 0x06 }, Crawler::ByteSpecPolicy::OPTIONAL }
    });
    CRAWLER_ASSERT_TRUE(pattern.Match(data.data(), data.size()));
}

void EncodingMatchRangeTest() {
    std::vector<uint8_t> data {0x01, 0x02, 0x03, 0x04};
    Crawler::Pattern pattern({
        { 0x01, 0x04, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x01, 0x04, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x01, 0x04, Crawler::ByteSpecPolicy::MANDATORY },
        { 0x01, 0x04, Crawler::ByteSpecPolicy::MANDATORY },
    });
    CRAWLER_ASSERT_TRUE(pattern.Match(data.data(), data.size()));
}

struct AttributeDetectionTest
{
    std::string input;
    std::string expectedName;
    std::string expectedValue;
    size_t expectedPos;
};

void RunAttributeDetectionTest(const AttributeDetectionTest& test) {
    std::vector<uint8_t> data(test.input.begin(), test.input.end());
    size_t pos = 0;

    auto [name, value] = Crawler::GetAnAttribute(data.data(), data.size(), &pos);

    CRAWLER_ASSERT_EQ(test.expectedName, name);
    CRAWLER_ASSERT_EQ(test.expectedValue, value);
    CRAWLER_ASSERT_EQ(test.expectedPos, pos);
}

void GetAnAttributeTest() {
    std::vector<AttributeDetectionTest> tests({
        { ">", "", "", 0 },
        { " />", "", "", 2 },
        { " /", "", "", 2 },
        { " \t\n\f\r name=value", "name", "value", 16 },
        { "malf>", "malf", "", 4 },
        { "malf/>", "malf", "", 4 },
        { "/name=value", "name", "value", 11 },
        { ">rest", "", "", 0 },
        { "   >rest", "", "", 3 },
        { "/>rest", "", "", 1 },
        { "NaMe=value", "name", "value", 10 },
        { "name  =value", "name", "value", 12 },
        { "a>rest", "a", "", 1 },
        { "a=\"ABC\"", "a", "abc", 7 },
        { "a='ABC'", "a", "abc", 7 },
        { "a=v\tx", "a", "v", 3 }
    });

    for (const AttributeDetectionTest& t : tests) {
        RunAttributeDetectionTest(t);
    }
}

struct XMLEncodingTest
{
    std::string input;
    Crawler::Encoding expectedEncoding;
};

void RunXMLEncodingTest(const XMLEncodingTest& test) {
    std::vector<uint8_t> data(test.input.begin(), test.input.end());
    size_t pos = 0;

    Crawler::Encoding actual = Crawler::GetAnXMLEncoding(data.data(), data.size(), &pos);

    CRAWLER_ASSERT_TRUE((test.expectedEncoding == actual));
}

void GetAnXMLEncodingTest() {
    std::vector<XMLEncodingTest> tests({
        { "<?xml encoding='UTF8'>", Crawler::Encoding::UTF8 },
        { "<?xmp", Crawler::Encoding::UNDEFINED },
        { " <?xml", Crawler::Encoding::UNDEFINED },
        { "<?xml", Crawler::Encoding::UNDEFINED },
        { "<?xml >", Crawler::Encoding::UNDEFINED },
        { "<?xml encoding=>", Crawler::Encoding::UNDEFINED },
        { "<?xml encoding=", Crawler::Encoding::UNDEFINED },
        { "<?xml encoding='UT F8'>", Crawler::Encoding::UNDEFINED },
        { "<?xml encoding    =  'uNicOde20utf8'>", Crawler::Encoding::UTF8 },
        { "<?xml encoding \t \n \v \f \r =  'non-existent-encoding'>", Crawler::Encoding::UNDEFINED },
        { "<?xml encoding\t\n= \t \n \v \f \r 'csunIcode'>", Crawler::Encoding::UTF8 },
        { "<?xml encoding='CP1254' someotherstuff>", Crawler::Encoding::WINDOWS1254 },
        { "<?xml encoding='big5' encoding='utF8'>", Crawler::Encoding::BIG5 },
        { "<?xml encoding='bi\tg5'>", Crawler::Encoding::UNDEFINED }
    });

    for (const XMLEncodingTest& t : tests) {
        RunXMLEncodingTest(t);
    }
}

struct MetaEncodingTest
{
    std::string input;
    Crawler::Encoding expectedEncoding;
};

void RunMetaEncodingTest(const MetaEncodingTest& test) {
    Crawler::Encoding actual = Crawler::ExtractEncodingFromMetaElement(test.input);

    CRAWLER_ASSERT_TRUE((test.expectedEncoding == actual));
}

void ExtractEncodingFromMetaElement() {
    std::vector<MetaEncodingTest> tests({
        { "no-match-return-nothing", Crawler::Encoding::UNDEFINED },
        { "charset=", Crawler::Encoding::UNDEFINED },
        { "charset=charset", Crawler::Encoding::UNDEFINED },
        { "charset=\'utf8\"", Crawler::Encoding::UNDEFINED },
        { "charset=\"utf8\'", Crawler::Encoding::UNDEFINED },
        { "charset='utf8'", Crawler::Encoding::UTF8 },
        { "charset=\"utf8\"", Crawler::Encoding::UTF8 },
        { "charset=utf8", Crawler::Encoding::UTF8 },
        { "charset=utf8;iso88592", Crawler::Encoding::UTF8 },
        { "charset=iso885911\tiso88592", Crawler::Encoding::WINDOWS874 },
        { "charset=csisolatin5\niso88592", Crawler::Encoding::WINDOWS1254 },
        { "charset=big5\riso88592", Crawler::Encoding::BIG5 },
        { "charset=unicode\fiso88592", Crawler::Encoding::UTF16LE },
        { "charsetcharset=big5", Crawler::Encoding::BIG5 }
    });

    for (const MetaEncodingTest& t : tests) {
        RunMetaEncodingTest(t);
    }
}

struct PrescanStreamTest
{
    std::vector<uint8_t> input;
    Crawler::Encoding expectedEncoding;
};

void RunPrescanStreamTest(const PrescanStreamTest& test) {
    size_t pos = 0;
    Crawler::Encoding actual = Crawler::Prescan(test.input.data(), test.input.size(), &pos);

    CRAWLER_ASSERT_TRUE((test.expectedEncoding == actual));
}

void PrescanTest() {
    std::string inputCharset1("<meta charset=\"utf8\"/>");
    std::string inputCharset2("<meta http-equiv=\"content-type\" content=\"text/html; charset=iso-8859-2\">");
    std::string inputCharset3("<meta content=\"text/html; charset=UTF-16\">");
    std::string inputCharset4("<meta charset=\"UTF-16BE\">");
    std::string inputCharset5("<meta charset=\"x-user-defined\">");
    std::string inputCharset6("<!-- comment --><meta charset=\"Shift_JIS\">");
    std::string inputCharset7("<meta http-equiv=\"refresh\" content=\"0;url=...\"><meta charset=\"EUC-KR\">");
    std::string inputCharset8("<meta charset=\"invalid\"><meta http-equiv=\"content-type\" content=\"text/html; charset=windows-1255\">");

    std::vector<PrescanStreamTest> tests({
        { std::vector<uint8_t>(inputCharset1.begin(), inputCharset1.end()), Crawler::Encoding::UTF8 },
        { std::vector<uint8_t>(inputCharset2.begin(), inputCharset2.end()), Crawler::Encoding::ISO88592 },
        { std::vector<uint8_t>(inputCharset3.begin(), inputCharset3.end()), Crawler::Encoding::UNDEFINED },
        { std::vector<uint8_t>(inputCharset4.begin(), inputCharset4.end()), Crawler::Encoding::UTF8 },
        { std::vector<uint8_t>(inputCharset5.begin(), inputCharset5.end()), Crawler::Encoding::WINDOWS1252 },
        { std::vector<uint8_t>(inputCharset6.begin(), inputCharset6.end()), Crawler::Encoding::SHIFTJIS },
        { std::vector<uint8_t>(inputCharset7.begin(), inputCharset7.end()), Crawler::Encoding::EUCKR },
        { std::vector<uint8_t>(inputCharset8.begin(), inputCharset8.end()), Crawler::Encoding::WINDOWS1255 },
        { { 0x3C, 0x00, 0x3F, 0x00, 0x78, 0x00 }, Crawler::Encoding::UTF16LE },
        { { 0x00, 0x3C, 0x00, 0x3F, 0x00, 0x78 }, Crawler::Encoding::UTF16BE },
        { { 0x3C, 0x21, 0x2D, 0x2D, 0x3E }, Crawler::Encoding::UNDEFINED }, // final position 0 due to get an xml
        { { 0x3C, 0x21, 0x3E }, Crawler::Encoding::UNDEFINED },
        { { 0x3C, 0x2F, 0x3E }, Crawler::Encoding::UNDEFINED },
        { { 0x3C, 0x3F, 0x3E }, Crawler::Encoding::UNDEFINED }
    });

    for (const PrescanStreamTest& t : tests) {
        RunPrescanStreamTest(t);
    }
}

void EncodingTest() {
    EncodingMatchSingleElementMandatoryTest();
    EncodingMatchSetMandatoryTest();
    EncodingMatchOptionalTest();
    EncodingMatchRangeTest();
    GetAnAttributeTest();
    GetAnXMLEncodingTest();
    ExtractEncodingFromMetaElement();
    PrescanTest();
}

}

int main(int argc, char** argv) {
    Test::EncodingTest();
    CRAWLER_PRINT_TEST_SUMMARY;
    return CRAWLER_TEST_RESULT;
}