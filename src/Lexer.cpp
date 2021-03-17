#include <stdexcept>
#include <array>
#include <cctype>
#include <Lexer.h>
#include <sys/types.h>
#include <sys/stat.h>

Lexer::Lexer()
:  token_offset(0),
  types{
        {"SELECT",  Token::SELECT},
        {"WHERE",  Token::WHERE},
        {"FROM", Token::FROM},
        {"LIMIT", Token::LIMIT},
        {"INSERT", Token::INSERT},
        {"INTO", Token::INTO},
        {"VALUES", Token::VALUES},
        {"SHOW", Token::SHOW},
        {"TABLES", Token::TABLES},
        {"DESC", Token::DESC},
        {"CREATE", Token::CREATE},
        {"TABLE", Token::TABLE},
        {"DELETE", Token::DELETE},
        {"IN", Token::IN},
        {"NOT", Token::NOT},
        {"UPDATE", Token::UPDATE},
        {"SET", Token::SET},
}
{
}

bool Lexer::lex(std::string_view data_)
{
    data = data_;
    token_offset = 0;
    advance();
    return true;
}

void Lexer::advance()
{
    if(token_offset >= data.size())
    {
        current_token = Token(Token::EOI, "");
        return;
    }

    for(; token_offset < data.size(); ++token_offset)
    {
        switch(data[token_offset])
        {
            case '\n':
            case '\t':
            case ' ':
                break;
            case '(':
                current_token = Token(Token::OPEN_PARENTHESIS, "(");
                token_offset++;
                return;
            case ')':
                current_token = Token(Token::CLOSE_PARENTHESIS, ")");
                token_offset++;
                return;
            case ';':
                current_token = Token(Token::SEMI_COLON, ";");
                token_offset++;
                return;
            case '<':
                current_token = Token(Token::ANGULAR_OPEN, "<");
                token_offset++;
                return;
            case '>':
                current_token = Token(Token::ANGULAR_CLOSE, ">");
                token_offset++;
                return;
            case ',':
                current_token = Token(Token::COMMA, ",");
                token_offset++;
                return;
            case '=':
                current_token = Token(Token::EQUALS, "=");
                token_offset++;
                return;
            case '+':
                current_token = Token(Token::PLUS, "+");
                token_offset++;
                return;
            case '*':
                current_token = Token(Token::ASTERISK, "*");
                token_offset++;
                return;
            case '/':
                current_token = Token(Token::FORWARDS_SLASH, "/");
                token_offset++;
                return;
            case '%':
                current_token = Token(Token::MODULO, "%");
                token_offset++;
                return;
            case '"':
            {
                ++token_offset;
                current_token = Token(Token::STRING, "");
                const char* beg = &data[token_offset];
                while (data[token_offset] != '"')
                {   
                    token_offset++;
                }
                current_token.data = std::string_view(beg, &data[token_offset] - beg);
                token_offset++;
                return;
            }
            default:
                if((data[token_offset] >= '0' && data[token_offset] <= '9') ||
                   data[token_offset] == '-') //If number, it's an INT type
                {
                    current_token = Token(Token::INT, "");
                    const char* str_begin = &data[token_offset];
                    while ((data[token_offset] >= '0' && data[token_offset] <= '9') || data[token_offset] == '-')
                    {
                        token_offset++;
                    }
                    current_token.data = std::string_view(str_begin, &data[token_offset] - str_begin);

                    if(current_token.data == "-")
                    {
                        current_token.type = Token::MINUS;
                    }
                }
                else //Else ID/TYPE/Variable
                {
                    //Read in whole thing, assuming it's an ID
                    current_token = Token(Token::ID, "");
                    const char* str_begin = &data[token_offset];
                    while ((std::isalnum(data[token_offset]) || data[token_offset] == '_' || data[token_offset] == '.') && token_offset < data.size())
                    {
                        token_offset++;
                    }
                    current_token.data = std::string_view(str_begin, &data[token_offset] - str_begin);

                    //Switch out true/false for integers
                    if(current_token.data == "true")
                    {
                        current_token.type = Token::INT;
                        current_token.data = "1";
                    }
                    else if(current_token.data == "false")
                    {
                        current_token.type = Token::INT;
                        current_token.data = "0";
                    }
                    else
                    {
                        //Check if type, change it to TYPE
                        std::string token_upper(current_token.data.size(), '\0');
                        std::transform(current_token.data.begin(), current_token.data.end(), token_upper.begin(), ::toupper);
                        auto type_iter = types.find(token_upper);
                        if(type_iter != types.end())
                            current_token.type = type_iter->second;
                    }

                }
                return;
        }
    }
}