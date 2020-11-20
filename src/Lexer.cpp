#include <stdexcept>
#include <array>
#include <Lexer.h>

Lexer::Lexer()
: line_number(0),
  token_offset(0),
  types{{"var",   Token::VAR},
        {"if",    Token::IF},
        {"while",  Token::WHILE},
        {"else",  Token::ELSE},
        {"SELECT",  Token::SELECT},
        {"WHERE",  Token::WHERE},
        {"FROM", Token::FROM}}
{

}

bool Lexer::lex(std::string data_)
{
    data = std::move(data_);
    token_offset = 0;
    advance();
    return true;
}

Lexer::Token Lexer::current()
{
    return current_token;
}

void Lexer::advance()
{
    if(token_offset >= data.size())
    {
        current_token = Token(Token::EOI);
        return;
    }

    for(; token_offset < data.size(); ++token_offset)
    {
        switch(data[token_offset])
        {
            case '\n':
                ++line_number;
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
            case ',':
                current_token = Token(Token::COMMA, ",");
                token_offset++;
                return;
            case '=':
                current_token = Token(Token::EQUALS, "=");
                if(++token_offset < data.size() && data[token_offset] == '=')
                {
                    current_token = Token(Token::DOES_EQUAL, "=");
                    token_offset++;
                }
                return;
            case '+':
                current_token = Token(Token::PLUS, "+");
                token_offset++;
                if(token_offset < data.size() && data[token_offset] == '+')
                {
                    token_offset++;
                    current_token = Token(Token::PLUSPLUS, "++");
                }
                return;
            case '*':
                current_token = Token(Token::ASTERISK, "*");
                token_offset++;
                return;
            case '/':
                current_token = Token(Token::FORWARDS_SLASH, "/");
                token_offset++;
                return;
            case '{':
                current_token = Token(Token::OPEN_BRACE, "{");
                token_offset++;
                return;
            case '}':
                current_token = Token(Token::CLOSE_BRACE, "}");
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
            case '%':
                current_token = Token(Token::MODULO, "%");
                token_offset++;
                return;
            case '!':
                current_token = Token(Token::NOT, "!");
                if(++token_offset < data.size() && data[token_offset] == '=')
                {
                    current_token = Token(Token::DOES_NOT_EQUAL, "!=");
                    token_offset++;
                }
                return;
            case '"':
            {
                ++token_offset;
                current_token = Token(Token::STRING);
                while(data[token_offset] != '"')
                    current_token.data += data[token_offset++];
                token_offset++;
                return;
            }
            case '[':
            {
                current_token = Token(Token::OPEN_BRACKET, "[");
                token_offset++;
                return;
            }
            case ']':
            {
                current_token = Token(Token::CLOSE_BRACKET, "]");
                token_offset++;
                return;
            }
            default:
                if((data[token_offset] >= '0' && data[token_offset] <= '9') ||
                   data[token_offset] == '-') //If number, it's an INT type
                {
                    current_token = Token(Token::INT);
                    while((data[token_offset] >= '0' && data[token_offset] <= '9') || data[token_offset] == '-')
                        current_token.data += data[token_offset++];
                    if(current_token.data == "-")
                    {
                        current_token.type = Token::MINUS;
                    }
                    else if(current_token.data == "--")
                    {
                        current_token.type = Token::MINUSMINUS;
                    }
                }
                else //Else ID/TYPE/Variable
                {
                    //Read in whole thing, assuming it's an ID
                    current_token = Token(Token::ID);
                    while((std::isalnum(data[token_offset]) || data[token_offset] == '_' ||
                           data[token_offset] == '.') && token_offset < data.size())
                        current_token.data += data[token_offset++];

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
                        //Or change to variable if it is
                        if(variables.find(current_token.data) != variables.end())
                        {
                            current_token.type = Token::VARIABLE;
                        }

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

bool Lexer::match(Lexer::Token::Type type)
{
    return current().type == type;
}

bool Lexer::legal_lookahead(Token::Type token)
{
    if(match(token))
        return true;

    throw SyntaxError("Unexpected token '" + current().str() + "'. Expected: '" + Token(token).str() + "'");
}

size_t Lexer::get_line_number()
{
    return line_number;
}

void Lexer::register_variable(std::string name)
{
    variables.emplace(std::move(name), 0);
}