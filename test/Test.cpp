#include "Test.hpp"

#include "Encoding.hpp"

#include <cstddef>
#include <concepts>
#include <format>
#include <string>
#include <vector>
#include <cstdint>

namespace Test
{

inline constexpr Crawler::ByteSpecPolicy M = Crawler::ByteSpecPolicy::MANDATORY;
inline constexpr Crawler::ByteSpecPolicy O = Crawler::ByteSpecPolicy::OPTIONAL;

template <std::integral... Ts>
constexpr auto bytes(Ts... values) {
    return std::vector<std::byte>{
        static_cast<std::byte>(values)...
    };
}

void EncodingMatchSingleElementMandatoryTest() {
    std::vector<std::byte> data = bytes(0x01, 0x02, 0x03, 0x04);
    Crawler::Pattern pattern({
        { M, 0x01 },
        { M, 0x02 },
        { M, 0x03 },
        { M, 0x04 }
    });
    CRAWLER_ASSERT_TRUE(pattern.Match(data.data(), data.size()));
}

void EncodingMatchSetMandatoryTest() {
    std::vector<std::byte> data = bytes(0x01, 0x02, 0x03, 0x04);
    Crawler::Pattern pattern({
        { M, 0x01, 0x02 },
        { M, 0x01, 0x02 },
        { M, 0x03, 0x04 },
        { M, 0x03, 0x04 }
    });
    CRAWLER_ASSERT_TRUE(pattern.Match(data.data(), data.size()));
}

void EncodingMatchOptionalTest() {
    std::vector<std::byte> data = bytes(0x01, 0x02, 0x03, 0x04);
    Crawler::Pattern pattern({
        { M, 0x01, 0x02 },
        { O, 0x05, 0x06 },
        { M, 0x02 },
        { M, 0x03, 0x04 },
        { O, 0x05, 0x06 }
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
    std::vector<std::byte> data(
        reinterpret_cast<const std::byte*>(test.input.data()),
        reinterpret_cast<const std::byte*>(test.input.data() + test.input.size())
    );
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
    std::vector<std::byte> data(
        reinterpret_cast<const std::byte*>(test.input.data()),
        reinterpret_cast<const std::byte*>(test.input.data() + test.input.size())
    );
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
    PrescanStreamTest(std::string input, Crawler::Encoding expectedEncoding)
    : input(reinterpret_cast<const std::byte*>(input.data()), reinterpret_cast<const std::byte*>(input.data() + input.size()))
    , expectedEncoding(expectedEncoding) { }

    std::vector<std::byte> input;
    Crawler::Encoding expectedEncoding;
};

void RunPrescanStreamTest(const PrescanStreamTest& test) {
    size_t pos = 0;
    Crawler::Encoding actual = Crawler::Prescan(test.input.data(), test.input.size(), &pos);

    CRAWLER_ASSERT_TRUE((test.expectedEncoding == actual));
}

void PrescanTest() {
    std::vector<PrescanStreamTest> tests({
        { "<meta charset=\"utf8\"/>", Crawler::Encoding::UTF8 },
        { "<meta http-equiv=\"content-type\" content=\"text/html; charset=iso-8859-2\">", Crawler::Encoding::ISO88592 },
        { "<meta content=\"text/html; charset=UTF-16\">", Crawler::Encoding::UNDEFINED },
        { "<meta charset=\"UTF-16BE\">", Crawler::Encoding::UTF8 },
        { "<meta charset=\"x-user-defined\">", Crawler::Encoding::WINDOWS1252 },
        { "<!-- comment --><meta charset=\"Shift_JIS\">", Crawler::Encoding::SHIFTJIS },
        { "<meta http-equiv=\"refresh\" content=\"0;url=...\"><meta charset=\"EUC-KR\">", Crawler::Encoding::EUCKR },
        { "<meta charset=\"invalid\"><meta http-equiv=\"content-type\" content=\"text/html; charset=windows-1255\">", Crawler::Encoding::WINDOWS1255 }
    });

    for (const PrescanStreamTest& t : tests) {
        RunPrescanStreamTest(t);
    }
}

void EncodingTest() {
    EncodingMatchSingleElementMandatoryTest();
    EncodingMatchSetMandatoryTest();
    EncodingMatchOptionalTest();
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