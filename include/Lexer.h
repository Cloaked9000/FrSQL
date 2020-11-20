#ifndef COMPILER_LEXER_H
#define COMPILER_LEXER_H

#include <cstdio>
#include <algorithm>
#include <unordered_map>
#include <exceptions/SyntaxError.h>

class Lexer
{
public:
    Lexer();

    struct Token
    {
        enum Type
        {
            OPEN_PARENTHESIS = 0,
            CLOSE_PARENTHESIS = 1,
            EOI = 2,
            SEMI_COLON = 3,
            ID = 4,
            VAR = 5,
            COMMA = 6,
            INT = 7,
            STRING = 8,
            EQUALS = 9,
            VARIABLE = 10,
            PLUS = 11,
            ASTERISK = 12,
            FORWARDS_SLASH = 13,
            MINUS = 14,
            IF = 15,
            OPEN_BRACE = 16,
            CLOSE_BRACE = 17,
            ELSE = 18,
            WHILE = 19,
            ANGULAR_OPEN = 20,
            ANGULAR_CLOSE = 21,
            MODULO = 22,
            DOES_EQUAL = 23,
            DOES_NOT_EQUAL = 24,
            NOT = 25,
            OPEN_BRACKET = 26,
            CLOSE_BRACKET = 27,
            ARRAY = 28,
            PLUSPLUS = 29,
            MINUSMINUS = 30,
            SELECT = 31,
            WHERE = 32,
            FROM = 33,
            TokenCount = 34, //Keep at end
        };

        const std::string &str()
        {
            static std::array<std::string, 34> types = {
                    "(",
                    ")",
                    "EOI",
                    ";",
                    "id",
                    "var",
                    ",",
                    "int",
                    "string",
                    "=",
                    "variable",
                    "+",
                    "*",
                    "/",
                    "-",
                    "if",
                    "{",
                    "}",
                    "else",
                    "while",
                    "<",
                    ">",
                    "%",
                    "=",
                    "!=",
                    "!",
                    "[",
                    "]",
                    "array",
                    "++",
                    "--",
                    "SELECT",
                    "WHERE",
                    "FROM",
            };
            static_assert(std::tuple_size<decltype(types)>::value == Type::TokenCount, "types needs updating");
            return types[type];
        }

        Token() = default;

        explicit Token(Type type_, std::string data_ = "")
                : type(type_), data(std::move(data_))
        {}

        Type type;
        std::string data;
    };

    /*!
     * Lexes input, converting it into a series of tokens.
     *
     * @param data The data to lex
     * @return True on success, false on failure
     */
    bool lex(std::string data);

    /*!
     * Gets the current token
     *
     * @return The current token
     */
    Token current();

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
    bool match(Token::Type type, Args ...types)
    {
        return match(type) || match(types...);
    }

    bool match(Token::Type type);


    /*!
     * Checks to see if the current token matches a given type
     *
     * @param token The type to compare against
     * @return True if it matches, false otherwise.
     */
    bool legal_lookahead(Token::Type token);

    /*!
     * Register a variable with the lexer, so that it can properly classify them.
     *
     * @param name Name of the variable to register
     */
    void register_variable(std::string name);

    /*!
     * Checks to see if the current token matches any of the given arguments.
     *
     * Throws an std::exception and prints an error if not.
     *
     * @return True if the current token matches, false otherwise
     */
    template<typename T, typename... Args>
    bool legal_lookahead(T first, Args... args)
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

    /*!
     * Gets the current line number of the lexer
     *
     * @return The current line number
     */
    size_t get_line_number();

    /*!
     * Gets current token offset
     *
     * @return Current token offset
     */
    inline size_t get_token_offset() const
    {
        return token_offset;
    }

    /*!
     * Sets the current token offset
     *
     * @param offset The new offset
     */
    inline void set_token_offset(size_t offset)
    {
        token_offset = offset;
    }

private:
    size_t token_offset;
    size_t line_number;
    std::string data;
    Token current_token;
    std::unordered_map<std::string, Token::Type> types;
    std::unordered_map<std::string, int> variables;
};

#endif //COMPILER_LEXER_H
