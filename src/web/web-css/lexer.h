#pragma once

#include <karm-base/ctype.h>
#include <karm-base/func.h>
#include <karm-base/res.h>
#include <karm-base/string.h>
#include <karm-base/vec.h>
#include <karm-io/fmt.h>
#include <karm-logger/logger.h>

namespace Web::Css {

#define FOREACH_TOKEN(TOKEN)    \
    TOKEN(IDENT)                \
    TOKEN(FUNCTION)             \
    TOKEN(AT_KEYWORD)           \
    TOKEN(HASH)                 \
    TOKEN(STRING)               \
    TOKEN(BAD_STRING)           \
    TOKEN(URL)                  \
    TOKEN(BAD_URL)              \
    TOKEN(DELIM)                \
    TOKEN(NUMBER)               \
    TOKEN(PERCENTAGE)           \
    TOKEN(DIMENSION)            \
    TOKEN(WHITESPACE)           \
    TOKEN(CDO)                  \
    TOKEN(CDC)                  \
    TOKEN(COLON)                \
    TOKEN(SEMICOLON)            \
    TOKEN(COMMA)                \
    TOKEN(LEFT_CURLY_BRACKET)   \
    TOKEN(RIGHT_CURLY_BRACKET)  \
    TOKEN(LEFT_SQUARE_BRACKET)  \
    TOKEN(RIGHT_SQUARE_BRACKET) \
    TOKEN(LEFT_PARENTHESIS)     \
    TOKEN(RIGHT_PARENTHESIS)    \
    TOKEN(COMMENT)              \
    TOKEN(END_OF_FILE)          \
    TOKEN(OTHER)

struct Token {
    enum struct Type {
#define ITER(NAME) NAME,
        FOREACH_TOKEN(ITER)
#undef ITER
    };

    using enum Type;

    Type type;
    Str data;

    Token(Type type, Str data = "")
        : type(type), data(data) {}
};

struct Lexer {
    Io::SScan &_scan;
    Token _token;

    Token curr();

    Token next();

    Res<Token> expect(Token::Type);
};

static inline Str toStr(Token::Type type) {
    switch (type) {
#define ITER(NAME)          \
    case Token::Type::NAME: \
        return #NAME;
        FOREACH_TOKEN(ITER)
#undef ITER
    default:
        panic("invalid token type");
    }
}

Res<Token> nextToken(Io::SScan &s);

} // namespace Web::Css

template <>
struct Karm::Io::Formatter<Web::Css::Token> {
    Res<usize> format(Io::TextWriter &writer, Web::Css::Token const &val) {
        usize written = try$(writer.writeRune('{'));
        written += try$(writer.writeStr(try$(Io::toParamCase(Web::Css::toStr(val.type)))));

        written += try$(Io::format(writer, " data={#}", val.data));

        written += try$(writer.writeRune('}'));
        return Ok(written);
    }
};
