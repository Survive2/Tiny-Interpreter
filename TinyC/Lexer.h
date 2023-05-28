#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <map>
#include <vector>

using namespace std;
enum Token
{
    Token_eof = -1,
    Token_def = -2,
    Token_extern = -3,
    Token_identifier = -4,
    Token_number = -5

};

std::string identifierStr;
static double numVal;
static int pointCount = 0;
char tmpStr = ' ';
static int getToken()
{
    static int lastCharacter = ' ';
    while (isspace(lastCharacter))
    {
        lastCharacter = getchar();
    }
    if (isalpha(lastCharacter))
    {
        identifierStr = lastCharacter;
        while (isalnum((lastCharacter = getchar())))
        {
            identifierStr += lastCharacter;
        }
        if (identifierStr == "def")
            return Token_def;
        if (identifierStr == "extern")
            return Token_extern;
        return Token_identifier;
    }
    if (isdigit(lastCharacter) || lastCharacter == '.')
    {
        std::string numStr;
        if (lastCharacter == '.' && pointCount > 1)
        {
            perror("Wrong!");
            exit(0);
        }
        do
        {
            numStr += lastCharacter;
            lastCharacter = getchar();
        } while (isdigit(lastCharacter) || lastCharacter == '.');
        numVal = strtod(numStr.c_str(), 0);
        return Token_number;
    }
    if (lastCharacter == '#')
    {
        do
        {
            lastCharacter = getchar();
        } while (lastCharacter != '\n' && lastCharacter != EOF && lastCharacter != '\r');
        if (lastCharacter != EOF)
            return getToken();
    }
    if (lastCharacter == EOF)
        return Token_eof;
    int thisCharacter = lastCharacter;
    lastCharacter = getchar();
    return thisCharacter;
}