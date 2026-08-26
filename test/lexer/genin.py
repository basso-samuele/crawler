#!/usr/bin/env python3

import json
import sys
from pathlib import Path

STATE_NAMES = {
    "Data state": "Data",
    "RCDATA state": "Rcdata",
    "RAWTEXT state": "Rawtext",
    "Script data state": "ScriptData",
    "PLAINTEXT state": "Plaintext",
    "tag open state": "TagOpen",
    "end tag open state": "EndTagOpen",
    "tag name state": "TagName",
    "RCDATA less-than sign state": "RcdataLessThanSign",
    "RCDATA end tag open state": "RcdataEndTagOpen",
    "RCDATA end tag name state": "RcdataEndTagName",
    "RAWTEXT less-than sign state": "RawtextLessThanSign",
    "RAWTEXT end tag open state": "RawtextEndTagOpen",
    "RAWTEXT end tag name state": "RawtextEndTagName",
    "script data less-than sign state": "ScriptDataLessThanSign",
    "script data end tag open state": "ScriptDataEndTagOpen",
    "script data end tag name state": "ScriptDataEndTagName",
    "script data escape start state": "ScriptDataEscapeStart",
    "script data escape start dash state": "ScriptDataEscapeStartDash",
    "script data escaped state": "ScriptDataEscaped",
    "script data escaped dash state": "ScriptDataEscapedDash",
    "script data escaped dash dash state": "ScriptDataEscapedDashDash",
    "script data escaped less-than sign state": "ScriptDataEscapedLessThanSign",
    "script data escaped end tag open state": "ScriptDataEscapedEndTagOpen",
    "script data escaped end tag name state": "ScriptDataEscapedEndTagName",
    "script data double escape start state": "ScriptDataDoubleEscapeStart",
    "script data double escaped state": "ScriptDataDoubleEscaped",
    "script data double escaped dash state": "ScriptDataDoubleEscapedDash",
    "script data double escaped dash dash state": "ScriptDataDoubleEscapedDashDash",
    "script data double escaped less-than sign state": "ScriptDataDoubleEscapedLessThanSign",
    "script data double escape end state": "ScriptDataDoubleEscapeEnd",
    "before attribute name state": "BeforeAttributeName",
    "attribute name state": "AttributeName",
    "after attribute name state": "AfterAttributeName",
    "before attribute value state": "BeforeAttributeValue",
    "attribute value (double-quoted) state": "AttributeValueDoubleQuoted",
    "attribute value (single-quoted) state": "AttributeValueSingleQuoted",
    "attribute value (unquoted) state": "AttributeValueUnquoted",
    "after attribute value (quoted) state": "AfterAttributeValueQuoted",
    "self-closing start tag state": "SelfClosingStartTag",
    "bogus comment state": "BogusComment",
    "markup declaration open state": "MarkupDeclarationOpen",
    "comment start state": "CommentStart",
    "comment start dash state": "CommentStartDash",
    "comment state": "Comment",
    "comment less-than sign state": "CommentLessThanSign",
    "comment less-than sign bang state": "CommentLessThanSignBang",
    "comment less-than sign bang dash state": "CommentLessThanSignBangDash",
    "comment less-than sign bang dash dash state": "CommentLessThanSignBangDashDash",
    "comment end dash state": "CommentEndDash",
    "comment end state": "CommentEnd",
    "comment end bang state": "CommentEndBang",
    "DOCTYPE state": "Doctype",
    "before DOCTYPE name state": "BeforeDoctypeName",
    "DOCTYPE name state": "DoctypeName",
    "after DOCTYPE name state": "AfterDoctypeName",
    "after DOCTYPE public keyword state": "AfterDoctypePublicKeyword",
    "before DOCTYPE public identifier state": "BeforeDoctypePublicIdentifier",
    "DOCTYPE public identifier (double-quoted) state": "DoctypePublicIdentifierDoubleQuoted",
    "DOCTYPE public identifier (single-quoted) state": "DoctypePublicIdentifierSingleQuoted",
    "after DOCTYPE public identifier state": "AfterDoctypePublicIdentifier",
    "between DOCTYPE public and system identifiers state": "BetweenDoctypePublicAndSystemIdentifiers",
    "after DOCTYPE system keyword state": "AfterDoctypeSystemKeyword",
    "before DOCTYPE system identifier state": "BeforeDoctypeSystemIdentifier",
    "DOCTYPE system identifier (double-quoted) state": "DoctypeSystemIdentifierDoubleQuoted",
    "DOCTYPE system identifier (single-quoted) state": "DoctypeSystemIdentifierSingleQuoted",
    "after DOCTYPE system identifier state": "AfterDoctypeSystemIdentifier",
    "bogus DOCTYPE state": "BogusDoctype",
    "CDATA section state": "CdataSection",
    "CDATA section bracket state": "CdataSectionBracket",
    "CDATA section end state": "CdataSectionEnd",
    "processing instruction open state": "ProcessingInstructionOpen",
    "processing instruction target state": "ProcessingInstructionTarget",
    "after processing instruction target state": "AfterProcessingInstructionTarget",
    "processing instruction data state": "ProcessingInstructionData",
    "processing instruction questionable state": "ProcessingInstructionQuestionable",
    "character reference state": "CharacterReference",
    "named character reference state": "NamedCharacterReference",
    "ambiguous ampersand state": "AmbiguousAmpersand",
    "numeric character reference state": "NumericCharacterReference",
    "hexadecimal character reference start state": "HexadecimalCharacterReferenceStart",
    "hexadecimal character reference state": "HexadecimalCharacterReference",
    "decimal character reference state": "DecimalCharacterReference",
    "numeric character reference end state": "NumericCharacterReferenceEnd"
}

ERROR_NAMES = {
    "abrupt-closing-of-empty-comment": "AbruptClosingOfEmptyComment",
    "abrupt-doctype-public-identifier": "AbruptDoctypePublicIdentifier",
    "abrupt-doctype-system-identifier": "AbruptDoctypeSystemIdentifier",
    "absence-of-digits-in-numeric-character-reference": "AbsenceOfDigitsInNumericCharacterReference",
    "cdata-in-html-content": "CdataInHtmlContent",
    "character-reference-outside-unicode-range": "CharacterReferenceOutsideUnicodeRange",
    "control-character-in-input-stream": "ControlCharacterInInputStream",
    "control-character-reference": "ControlCharacterReference",
    "disallowed-processing-instruction-target": "DisallowedProcessingInstructionTarget",
    "duplicate-attribute": "DuplicateAttribute",
    "end-tag-with-attributes": "EndTagWithAttributes",
    "end-tag-with-trailing-solidus": "EndTagWithTrailingSolidus",
    "eof-before-tag-name": "EofBeforeTagName",
    "eof-in-cdata": "EofInCdata",
    "eof-in-comment": "EofInComment",
    "eof-in-doctype": "EofInDoctype",
    "eof-in-processing-instruction": "EofInProcessingInstruction",
    "eof-in-script-html-comment-like-text": "EofInScriptHtmlCommentLikeText",
    "eof-in-tag": "EofInTag",
    "incorrectly-closed-comment": "IncorrectlyClosedComment",
    "incorrectly-opened-comment": "IncorrectlyOpenedComment",
    "invalid-character-sequence-after-doctype-name": "InvalidCharacterSequenceAfterDoctypeName",
    "invalid-first-character-of-processing-instruction-target": "InvalidFirstCharacterOfProcessingInstructionTarget",
    "invalid-first-character-of-tag-name": "InvalidFirstCharacterOfTagName",
    "invalid-processing-instruction-target": "InvalidProcessingInstructionTarget",
    "missing-attribute-value": "MissingAttributeValue",
    "missing-doctype-name": "MissingDoctypeName",
    "missing-doctype-public-identifier": "MissingDoctypePublicIdentifier",
    "missing-doctype-system-identifier": "MissingDoctypeSystemIdentifier",
    "missing-end-tag-name": "MissingEndTagName",
    "missing-quote-before-doctype-public-identifier": "MissingQuoteBeforeDoctypePublicIdentifier",
    "missing-quote-before-doctype-system-identifier": "MissingQuoteBeforeDoctypeSystemIdentifier",
    "missing-semicolon-after-character-reference": "MissingSemicolonAfterCharacterReference",
    "missing-whitespace-after-doctype-public-keyword": "MissingWhitespaceAfterDoctypePublicKeyword",
    "missing-whitespace-after-doctype-system-keyword": "MissingWhitespaceAfterDoctypeSystemKeyword",
    "missing-whitespace-before-doctype-name": "MissingWhitespaceBeforeDoctypeName",
    "missing-whitespace-between-attributes": "MissingWhitespaceBetweenAttributes",
    "missing-whitespace-between-doctype-public-and-system-identifiers": "MissingWhitespaceBetweenDoctypePublicAndSystemIdentifiers",
    "nested-comment": "NestedComment",
    "noncharacter-character-reference": "NoncharacterCharacterReference",
    "noncharacter-in-input-stream": "NoncharacterInInputStream",
    "non-void-html-element-start-tag-with-trailing-solidus": "NonVoidHtmlElementStartTagWithTrailingSolidus",
    "null-character-reference": "NullCharacterReference",
    "surrogate-character-reference": "SurrogateCharacterReference",
    "surrogate-in-input-stream": "SurrogateInInputStream",
    "unexpected-character-after-doctype-system-identifier": "UnexpectedCharacterAfterDoctypeSystemIdentifier",
    "unexpected-character-in-attribute-name": "UnexpectedCharacterInAttributeName",
    "unexpected-character-in-unquoted-attribute-value": "UnexpectedCharacterInUnquotedAttributeValue",
    "unexpected-equals-sign-before-attribute-name": "UnexpectedEqualsSignBeforeAttributeName",
    "unexpected-null-character": "UnexpectedNullCharacter",
    "unexpected-solidus-in-tag": "UnexpectedSolidusInTag",
    "unknown-named-character-reference": "UnknownNamedCharacterReference"
}

def lookup_state(name):
    try:
        return STATE_NAMES[name]
    except KeyError:
        raise ValueError(f"Unknown tokenizer state: {name!r}")

def lookup_error(code):
    try:
        return ERROR_NAMES[code]
    except KeyError:
        raise ValueError(f"Unknown parse error: {code!r}")

def cpp_string(s):
    result = ['"']

    for c in s:
        cp = ord(c)

        if c == "\\":
            result.append("\\\\")
        elif c == '"':
            result.append('\\"')
        elif c == "\n":
            result.append("\\n")
        elif c == "\r":
            result.append("\\r")
        elif c == "\t":
            result.append("\\t")
        elif 0x20 <= cp < 0x7F:
            result.append(c)
        elif cp <= 0xFFFF:
            result.append(f"\\u{cp:04X}")
        else:
            result.append(f"\\U{cp:08X}")

    result.append('"')
    return "".join(result)


def cpp_u32string(s):
    return "U" + cpp_string(s)

def cpp_initial_state(state):
    return f"LexerState::{lookup_state(state)}"

def cpp_error(error):
    return (
        f'{{ ErrorType::{lookup_error(error["code"])}, '
        f'{error["col"]} }}'
    )

def cpp_doctype(token):
    name = token[1]
    public_identifier = token[2]
    system_identifier = token[3]
    force_quirks = token[4]

    cpp_name = cpp_u32string(name) if name is not None else "std::nullopt"
    cpp_public_identifier = cpp_u32string(public_identifier) if public_identifier is not None else "std::nullopt"
    cpp_system_identifier = cpp_u32string(system_identifier) if system_identifier is not None else "std::nullopt"

    return (
        "DoctypeToken{"
        f"{cpp_name}, "
        f"{cpp_public_identifier}, "
        f"{cpp_system_identifier}, "
        f"{str(force_quirks).lower()}"
        "}"
    )

def cpp_attribute(attribute):
    name, value = attribute

    return (
        "Attribute{"
        f"{cpp_u32string(name)}, "
        f"{cpp_u32string(value)}"
        "}"
    )

def cpp_start_tag(token):
    name = token[1]
    attributes = token[2]
    self_closing = token[3] if len(token) > 3 else False

    attribute_initializers = ", ".join(
        cpp_attribute(attribute)
        for attribute in reversed(attributes.items())
    )

    return (
        "StartTagToken{"
        f"{cpp_u32string(name)}, "
        f"{{ {attribute_initializers} }}, "
        f"{str(self_closing).lower()}"
        "}"
    )

def cpp_proc_in(token):
    data = token[1]
    target = token[2]

    return (
        "ProcessingInstructionToken{"
        f"{cpp_u32string(data)}, "
        f"{cpp_u32string(target)}"
        "}"
    )

def cpp_token(token):
    kind = token[0]

    if kind == "Character":
        return f"CharacterToken{{{cpp_u32string(token[1])}s}}"

    if kind == "StartTag":
        return cpp_start_tag(token)

    if kind == "EndTag":
        return f"EndTagToken{{{cpp_u32string(token[1])}}}"

    if kind == "Comment":
        return f"CommentToken{{{cpp_u32string(token[1])}}}"

    if kind == "DOCTYPE":
        return cpp_doctype(token)

    if kind == "ProcessingInstruction":
        return cpp_proc_in(token)

    if kind == "EOF":
        return "EOFToken{}"

    raise ValueError(f"Unsupported token type: {kind!r}")

def test_case(test, initial_state):
    lines = []

    # opening structure
    lines.append("TokenizerTest{")

    # description
    lines.append(f'    {cpp_string(test["description"])}, // description')

    # input
    lines.append(f'    {cpp_string(test["input"])}s, // input')

    # expected tokens
    lines.append("    { // expectedTokens")
    output = test["output"]
    for i, token in enumerate(output):
        comma = "," if i+1 < len(output) else ""
        lines.append(f"        {cpp_token(token)}{comma}")
    lines.append("    },")

    # initial state
    lines.append(f"    {cpp_initial_state(initial_state)}, // initialState")

    # last start tag
    last_start_tag = test.get("lastStartTag")
    if not last_start_tag:
        lines.append("    std::nullopt, // lastStartTag")
    else:
        lines.append(f"    {cpp_u32string(last_start_tag)}, // lastStartTag")

    # errors
    lines.append("    { // expectedErrors")
    errors = test.get("errors", [])
    for i, err in enumerate(errors):
        comma = "," if i+1 < len(errors) else ""
        lines.append(f"        {cpp_error(err)}{comma}")
    lines.append("    }")

    # closing structure
    lines.append("},")

    return "\n".join(lines)

def flatten_test(test):
    # the current tokenizer implementation may not emit any token otherwise and some tests may not run correctly
    test["output"].append(["EOF"])
    initial_states = test.get("initialStates") or ["Data state"]
    return "".join(
        test_case(test, initial_state)
        for initial_state in initial_states
    )

def genin(dir, finname):
    fin_path = dir / finname
    fout_path = fin_path.with_suffix(".in")
    with open(fin_path, "r", encoding="utf-8") as fin, \
         open(fout_path, "w", encoding="utf-8") as fout:
        data = json.load(fin)
        for test in data["tests"]:
            fout.write(flatten_test(test))

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} first.test second.test ...", file=sys.stderr)
        sys.exit(1)

    script_dir = Path(__file__).resolve().parent
    for filename in sys.argv[1:]:
        if not filename.endswith(".test"):
            print(f"Skipping {filename}: not a .test file.")
            continue
        file_path = script_dir / filename
        if not file_path.is_file():
            print(f"File not found: {file_path}")
            continue
        genin(script_dir, filename)

    exit(0)

if __name__ == "__main__":
    main()
