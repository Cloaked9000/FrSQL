#ifndef COMPILER_LEXER_H
#define COMPILER_LEXER_H

#include <cstdio>
#include <algorithm>
#include <unordered_map>
#include <string_view>
#include <array>
#include <exceptions/SyntaxError.h>
#include <span>

class Lexer
{
public:
    Lexer();

    struct Token
    {
        enum Type
        {
            SELECT = 0,
            INSERT,
            SHOW,
            DESC,
            CREATE,
            DELETE,
            UPDATE,
            OPEN_PARENTHESIS,
            CLOSE_PARENTHESIS,
            EOI,
            SEMI_COLON,
            ID,
            COMMA,
            INT,
            STRING,
            EQUALS,
            PLUS,
            ASTERISK,
            FORWARDS_SLASH,
            MINUS,
            MODULO,
            DOES_NOT_EQUAL,
            NOT,
            ANGULAR_OPEN,
            ANGULAR_CLOSE,
            WHERE,
            FROM,
            LIMIT,
            INTO,
            VALUES,
            TABLES,
            TABLE,
            IN,
            SET,
            TokenCount, //Keep at end
        };

        [[nodiscard]] const std::string &str() const
        {
            static std::array<std::string, 34> types = {
                    "SELECT",
                    "INSERT",
                    "SHOW",
                    "DESC",
                    "CREATE",
                    "DELETE",
                    "UPDATE",
                    "(",
                    ")",
                    "EOI",
                    ";",
                    "id",
                    ",",
                    "int",
                    "string",
                    "=",
                    "+",
                    "*",
                    "/",
                    "-",
                    "%",
                    "!=",
                    "NOT",
                    "<",
                    ">",
                    "WHERE",
                    "FROM",
                    "LIMIT",
                    "INTO",
                    "VALUES",
                    "TABLES",
                    "TABLE",
                    "IN",
                    "SET",
            };
            static_assert(std::tuple_size<decltype(types)>::value == Type::TokenCount, "types needs updating");
            return types[type];
        }

        Token() = default;

        explicit Token(Type type_, std::string_view data_ = "")
        : type(type_), data(data_)
        {}

        Type type;
        std::string_view data;
    };

    /*!
     * Lexes input, converting it into a series of tokens.
     *
     * @param data The data to lex
     * @return True on success, false on failure
     */
    bool lex(std::string_view data);

    /*!
     * Gets the current token
     *
     * @return The current token
     */
    [[nodiscard]] const Token &current() const
    {
        return current_token;
    }

    /*!
     * Advances to the next token
     */
    void advance();

    /*!
     * Checks to see if the current token's type matches
     *
     * @param type The type of token to check against
     * @return True if the current token's type matches the provided type. False otherwise.
     */
    template<typename ...Args>
    [[nodiscard]] inline bool match(Token::Type type, Args &&...types) const
    {
        return match(type) || match(types...);
    }

    [[nodiscard]] inline bool match(Token::Type type) const
    {
        return current().type == type;
    }


    /*!
     * Checks to see if the current token matches a given type
     *
     * @param token The type to compare against
     * @return True if it matches, false otherwise.
     */
    inline bool legal_lookahead(Token::Type token) const
    {
        if(match(token))
            return true;

        throw SyntaxError("Unexpected token '" + current().str() + "'. Expected: '" + Token(token, "").str() + "'");
    }
    /*!
     * Checks to see if the current token matches any of the given arguments.
     *
     * Throws an std::exception and prints an error if not.
     *
     * @return True if the current token matches, false otherwise
     */
    template<typename T, typename... Args>
    bool legal_lookahead(T first, Args&&... args) const
    {
        std::array<Token::Type, sizeof...(args)> param_list = {args...};
        bool is_error = true;
        for(auto &param : param_list)
        {
            if(match(param))
            {
                is_error = false;
                break;
            }
        }
        if(match(first))
            is_error = false;

        if(is_error)
        {
            std::string expected_list;
            for(auto &expected : param_list)
            {
                expected_list += "'" + Token(expected).str() + "' or ";
            }

            throw SyntaxError("Unexpected token '" + current().str() + "'. Expected: " + expected_list + "'" + Token(first).str() + "'");
        }

        return !is_error;
    }

private:

    size_t token_offset;
    std::string_view data;
    Token current_token;
    std::unordered_map<std::string, Token::Type> types;
};

#endif //COMPILER_LEXER_H
