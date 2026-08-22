#pragma once

#include <vector>
#include <variant>
#include <string>
#include <optional>

#include <parser.h>
#include <lexer.h>
#include <utils.h>

namespace Lexer
{

enum class LexerState {
    Data,
    Rcdata,
    Rawtext,
    ScriptData,
    Plaintext,
    TagOpen,
    EndTagOpen,
    TagName,
    RcdataLessThanSign,
    RcdataEndTagOpen,
    RcdataEndTagName,
    RawtextLessThanSign,
    RawtextEndTagOpen,
    RawtextEndTagName,
    ScriptDataLessThanSign,
    ScriptDataEndTagOpen,
    ScriptDataEndTagName,
    ScriptDataEscapeStart,
    ScriptDataEscapeStartDash,
    ScriptDataEscaped,
    ScriptDataEscapedDash,
    ScriptDataEscapedDashDash,
    ScriptDataEscapedLessThanSign,
    ScriptDataEscapedEndTagOpen,
    ScriptDataEscapedEndTagName,
    ScriptDataDoubleEscapeStart,
    ScriptDataDoubleEscaped,
    ScriptDataDoubleEscapedDash,
    ScriptDataDoubleEscapedDashDash,
    ScriptDataDoubleEscapedLessThanSign,
    ScriptDataDoubleEscapeEnd,
    BeforeAttributeName,
    AttributeName,
    AfterAttributeName,
    BeforeAttributeValue,
    AttributeValueDoubleQuoted,
    AttributeValueSingleQuoted,
    AttributeValueUnquoted,
    AfterAttributeValueQuoted,
    SelfClosingStartTag,
    BogusComment,
    MarkupDeclarationOpen,
    CommentStart,
    CommentStartDash,
    Comment,
    CommentLessThanSign,
    CommentLessThanSignBang,
    CommentLessThanSignBangDash,
    CommentLessThanSignBangDashDash,
    CommentEndDash,
    CommentEnd,
    CommentEndBang,
    Doctype,
    BeforeDoctypeName,
    DoctypeName,
    AfterDoctypeName,
    AfterDoctypePublicKeyword,
    BeforeDoctypePublicIdentifier,
    DoctypePublicIdentifierDoubleQuoted,
    DoctypePublicIdentifierSingleQuoted,
    AfterDoctypePublicIdentifier,
    BetweenDoctypePublicAndSystemIdentifiers,
    AfterDoctypeSystemKeyword,
    BeforeDoctypeSystemIdentifier,
    DoctypeSystemIdentifierDoubleQuoted,
    DoctypeSystemIdentifierSingleQuoted,
    AfterDoctypeSystemIdentifier,
    BogusDoctype,
    CdataSection,
    CdataSectionBracket,
    CdataSectionEnd,
    ProcessingInstructionOpen,
    ProcessingInstructionTarget,
    AfterProcessingInstructionTarget,
    ProcessingInstructionData,
    ProcessingInstructionQuestionable,
    CharacterReference,
    NamedCharacterReference,
    AmbiguousAmpersand,
    NumericCharacterReference,
    HexadecimalCharacterReferenceStart,
    HexadecimalCharacterReference,
    DecimalCharacterReference,
    NumericCharacterReferenceEnd
};

enum class ErrorType {
    AbruptClosingOfEmptyComment,
    AbruptDoctypePublicIdentifier,
    AbruptDoctypeSystemIdentifier,
    AbsenceOfDigitsInNumericCharacterReference,
    CdataInHtmlContent,
    CharacterReferenceOutsideUnicodeRange,
    ControlCharacterInInputStream,
    ControlCharacterReference,
    DisallowedProcessingInstructionTarget,
    DuplicateAttribute,
    EndTagWithAttributes,
    EndTagWithTrailingSolidus,
    EofBeforeTagName,
    EofInCdata,
    EofInComment,
    EofInDoctype,
    EofInProcessingInstruction,
    EofInScriptHtmlCommentLikeText,
    EofInTag,
    IncorrectlyClosedComment,
    IncorrectlyOpenedComment,
    InvalidCharacterSequenceAfterDoctypeName,
    InvalidFirstCharacterOfProcessingInstructionTarget,
    InvalidFirstCharacterOfTagName,
    InvalidProcessingInstructionTarget,
    MissingAttributeValue,
    MissingDoctypeName,
    MissingDoctypePublicIdentifier,
    MissingDoctypeSystemIdentifier,
    MissingEndTagName,
    MissingQuoteBeforeDoctypePublicIdentifier,
    MissingQuoteBeforeDoctypeSystemIdentifier,
    MissingSemicolonAfterCharacterReference,
    MissingWhitespaceAfterDoctypePublicKeyword,
    MissingWhitespaceAfterDoctypeSystemKeyword,
    MissingWhitespaceBeforeDoctypeName,
    MissingWhitespaceBetweenAttributes,
    MissingWhitespaceBetweenDoctypePublicAndSystemIdentifiers,
    NestedComment,
    NoncharacterCharacterReference,
    NoncharacterInInputStream,
    NonVoidHtmlElementStartTagWithTrailingSolidus,
    NullCharacterReference,
    SurrogateCharacterReference,
    SurrogateInInputStream,
    UnexpectedCharacterAfterDoctypeSystemIdentifier,
    UnexpectedCharacterInAttributeName,
    UnexpectedCharacterInUnquotedAttributeValue,
    UnexpectedEqualsSignBeforeAttributeName,
    UnexpectedNullCharacter,
    UnexpectedSolidusInTag,
    UnknownNamedCharacterReference
};

struct CharacterToken
{
    std::u32string data;
};

struct Attribute
{
    std::u32string name;
    std::u32string value;
};

struct StartTagToken
{
    std::u32string name;
    std::vector<Attribute> attributes;
    bool selfClosing;
};

struct EndTagToken
{
    std::u32string name;
};

struct CommentToken
{
    std::u32string data;
};

struct DoctypeToken
{
    std::optional<std::u32string> name;
    std::optional<std::u32string> publicIdentifier;
    std::optional<std::u32string> systemIdentifier;
    // true corresponds to the force-quirks flag being false, and vice-versa.
    bool correctness;
};

struct ProcessingInstructionToken
{
    std::u32string data;
    std::u32string target;
};

struct EOFToken
{
};

using ExpectedToken = std::variant<
    CharacterToken,
    StartTagToken,
    EndTagToken,
    CommentToken,
    DoctypeToken,
    ProcessingInstructionToken,
    EOFToken
>;

struct TokenizerTest
{
    std::string description;
    std::string input;

    std::vector<ExpectedToken> expectedTokens;

    LexerState initialState;

    std::optional<std::u32string> lastStartTag;

    struct Error
    {
        ErrorType type;
        std::size_t column;
    };

    std::vector<Error> expectedErrors;
};

extern const std::vector<TokenizerTest> kNamedEntitiesTests;
extern const std::vector<TokenizerTest> kNumericEntitiesTests;
extern const std::vector<TokenizerTest> kUnicodeCharTests;
extern const std::vector<TokenizerTest> kTest1Tests;
extern const std::vector<TokenizerTest> kTest2Tests;
extern const std::vector<TokenizerTest> kTest3Tests;
extern const std::vector<TokenizerTest> kTest4Tests;

}