#pragma once

#include "Test.hpp"
#include "Utils.hpp"

namespace Encoding
{

struct AttributeDetectionTest
{
    AttributeDetectionTest(std::string input, std::string expectedName, std::string expectedValue, size_t expectedPos)
        : input(Test::StringToBS(input)), expectedName(expectedName), expectedValue(expectedValue), expectedPos(expectedPos) {
    }

    std::vector<std::byte> input;
    std::string expectedName;
    std::string expectedValue;
    size_t expectedPos;
};

const std::vector<AttributeDetectionTest> attributeDetectionTests({
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

void RunAttributeDetectionTest(const AttributeDetectionTest& test) {
    size_t pos = 0;
    auto [name, value] = Crawler::GetAnAttribute(test.input.data(), test.input.size(), &pos);

    CRAWLER_ASSERT_EQ(test.expectedName, name);
    CRAWLER_ASSERT_EQ(test.expectedValue, value);
    CRAWLER_ASSERT_EQ(test.expectedPos, pos);
}

void GetAnAttributeTest() {
    for (const AttributeDetectionTest& t : attributeDetectionTests) {
        RunAttributeDetectionTest(t);
    }
}

struct XMLEncodingTest
{
    XMLEncodingTest(std::string input, Crawler::Encoding expectedEncoding)
        : input(Test::StringToBS(input)), expectedEncoding(expectedEncoding) {
    }

    std::vector<std::byte> input;
    Crawler::Encoding expectedEncoding;
};

const std::vector<XMLEncodingTest> xmlEncodingTests({
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


void RunXMLEncodingTest(const XMLEncodingTest& test) {
    size_t pos = 0;
    Crawler::Encoding actual = Crawler::GetAnXMLEncoding(test.input.data(), test.input.size(), &pos);

    CRAWLER_ASSERT_TRUE((test.expectedEncoding == actual));
}

void GetAnXMLEncodingTest() {
    for (const XMLEncodingTest& t : xmlEncodingTests) {
        RunXMLEncodingTest(t);
    }
}

struct MetaEncodingTest
{
    std::string input;
    Crawler::Encoding expectedEncoding;
};

const std::vector<MetaEncodingTest> metaEncodingTests({
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


void RunMetaEncodingTest(const MetaEncodingTest& test) {
    Crawler::Encoding actual = Crawler::ExtractEncodingFromMetaElement(test.input);

    CRAWLER_ASSERT_TRUE((test.expectedEncoding == actual));
}

void ExtractEncodingFromMetaElement() {
    for (const MetaEncodingTest& t : metaEncodingTests) {
        RunMetaEncodingTest(t);
    }
}

struct PrescanStreamTest
{
    PrescanStreamTest(std::string input, Crawler::Encoding expectedEncoding)
        : input(Test::StringToBS(input)), expectedEncoding(expectedEncoding) {
    }

    std::vector<std::byte> input;
    Crawler::Encoding expectedEncoding;
};


const std::vector<PrescanStreamTest> prescanStreamTests({
    { "<meta charset=\"utf8\"/>", Crawler::Encoding::UTF8 },
    { "<meta http-equiv=\"content-type\" content=\"text/html; charset=iso-8859-2\">", Crawler::Encoding::ISO88592 },
    { "<meta content=\"text/html; charset=UTF-16\">", Crawler::Encoding::UNDEFINED },
    { "<meta charset=\"UTF-16BE\">", Crawler::Encoding::UTF8 },
    { "<meta charset=\"x-user-defined\">", Crawler::Encoding::WINDOWS1252 },
    { "<!-- comment --><meta charset=\"Shift_JIS\">", Crawler::Encoding::SHIFTJIS },
    { "<meta http-equiv=\"refresh\" content=\"0;url=...\"><meta charset=\"EUC-KR\">", Crawler::Encoding::EUCKR },
    { "<meta charset=\"invalid\"><meta http-equiv=\"content-type\" content=\"text/html; charset=windows-1255\">", Crawler::Encoding::WINDOWS1255 }
});

void RunPrescanStreamTest(const PrescanStreamTest& test) {
    size_t pos = 0;
    Crawler::Encoding actual = Crawler::Prescan(test.input.data(), test.input.size(), &pos);

    CRAWLER_ASSERT_TRUE((test.expectedEncoding == actual));
}

void PrescanTest() {
    for (const PrescanStreamTest& t : prescanStreamTests) {
        RunPrescanStreamTest(t);
    }
}

void Test() {
    GetAnAttributeTest();
    GetAnXMLEncodingTest();
    ExtractEncodingFromMetaElement();
    PrescanTest();
}

}