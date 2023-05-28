#include "./Lexer.h"

static int __current_token;

/*
this map describes the precedence among the binary oprators like '+' or '*'
*/
static std::map<char, int> binaryOpcodePrecedence;

void setMap()
{
    binaryOpcodePrecedence['<'] = 10;
    binaryOpcodePrecedence['+'] = 20;
    binaryOpcodePrecedence['-'] = 20;
    binaryOpcodePrecedence['*'] = 40;
}

static int GetTokPrecedence()
{
    if (!isascii(__current_token))
        return -1;

    // Make sure it's a declared binop.
    int TokPrec = binaryOpcodePrecedence[__current_token];
    if (TokPrec <= 0)
        return -1;
    return TokPrec;
}

static int getNextToken()
{
    __current_token = getToken();
    return __current_token;
}

class ExprAST
{
public:
    virtual ~ExprAST() {}
};

// Expression class for number like 1.0
class NumberExprAST : public ExprAST
{
    double _value;

public:
    NumberExprAST(double val)
    {
        _value = val;
    }
};

// Expression class for variable like "a"
class VariableExprAST : public ExprAST
{
    std::string _name;

public:
    VariableExprAST(string &name)
    {
        _name = name;
    }
};

// Expression class for binary operator like "+"
class BinaryExprAST : public ExprAST
{
    char _operator;
    ExprAST *_left_hand_side, *_right_hand_side;

public:
    BinaryExprAST(char op, ExprAST *lhs, ExprAST *rhs)
    {
        _operator = op;
        _left_hand_side = lhs;
        _right_hand_side = rhs;
    }
};

// Expression class for function calls
class FuncCallExprAST : public ExprAST
{
    std::string _callee;
    std::vector<ExprAST *> _args;

public:
    FuncCallExprAST(string callee, std::vector<ExprAST *> &args)
    {
        _callee = callee;
        _args = args;
    }
};

/*
This class represents the "prototype" for a function,
which captures its name, and its argument names (thus implicitly the number
of arguments the function takes).
*/
class PrototypeAST
{
    std::string _name;
    std::vector<std::string> _args;

public:
    PrototypeAST(std::string name, std::vector<std::string> args)
    {
        _name = name;
        _args = args;
    }
};

// This class represents a function definition itself.
class FunctionAST
{
    PrototypeAST *_prototype;
    ExprAST *_body;

public:
    FunctionAST(PrototypeAST *prototype, ExprAST *body)
    {
        _prototype = prototype;
        _body = body;
    }
};

ExprAST *error(const char *Str)
{
    fprintf(stderr, "Error: %s\n", Str);
    return 0;
}
PrototypeAST *errorP(const char *Str)
{
    error(Str);
    return 0;
}
FunctionAST *errorF(const char *Str)
{
    error(Str);
    return 0;
}

static ExprAST *parseExpression();
/*
Parse Number Expression
*/
static ExprAST *parseNumberExpr()
{
    ExprAST *expr = new NumberExprAST(numVal);
    getNextToken();
    return expr;
}

// parse the expression like "(xxxx)"
static ExprAST *parseParenExpr()
{
    getNextToken();
    ExprAST *expr = parseExpression();
    if (!expr)
        return 0;
    if (__current_token != ')')
        return error("Expected ')'");
    getNextToken();
    return expr;
}

// parse the function call
static ExprAST *parseIdentifierExpr()
{
    std::string identifier = identifierStr;
    getNextToken();
    if (__current_token != '(')
        return new VariableExprAST(identifier);
    // if the token is '(', it means this is a function
    getNextToken();
    std::vector<ExprAST *> args;
    if (__current_token != ')')
    {
        while (1)
        {
            ExprAST *arg = parseExpression();
            if (!arg)
                return 0;
            args.push_back(arg);
            if (__current_token == ')')
                break;
            if (__current_token != ',')
                return error("expected ')' or ',' in argument list");
            getNextToken();
        }
    }
}

static ExprAST *parsePrimary()
{
    switch (__current_token)
    {
    case Token_identifier:
        return parseIdentifierExpr();
    case Token_number:
        return parseNumberExpr();
    case '(':
        return parseParenExpr();
    default:
        return error("unknown token when expecting an expression");
    }
}

static ExprAST *parseBinaryOpRhs(int ExprPrec, ExprAST *LHS);

static ExprAST *parseExpression()
{
    ExprAST *lhs = parsePrimary();
    if (!lhs)
        return 0;

    return parseBinaryOpRhs(0, lhs);
}

static ExprAST *parseBinaryOpRhs(int ExprPrec, ExprAST *LHS)
{
    // If this is a binop, find its precedence.
    while (1)
    {
        int TokPrec = GetTokPrecedence();

        // If this is a binop that binds at least as tightly as the current binop,
        // consume it, otherwise we are done.
        if (TokPrec < ExprPrec)
            return LHS;

        // Okay, we know this is a binop.
        int BinOp = __current_token;
        getNextToken(); // eat binop

        // Parse the primary expression after the binary operator.
        ExprAST *RHS = parsePrimary();
        if (!RHS)
            return 0;

        // If BinOp binds less tightly with RHS than the operator after RHS, let
        // the pending operator take RHS as its LHS.
        int NextPrec = GetTokPrecedence();
        if (TokPrec < NextPrec)
        {
            RHS = parseBinaryOpRhs(TokPrec + 1, RHS);
            if (RHS == 0)
                return 0;
        }

        // Merge LHS/RHS.
        LHS = new BinaryExprAST(BinOp, LHS, RHS);
    }
}

static PrototypeAST *parsePrototype()
{
    if (__current_token != Token_identifier)
    {
        return errorP("expected function name in prototype");
    }
    std::string funcName = identifierStr;
    getNextToken();

    if (__current_token != '(')
        return errorP("expected '(' in prototype");
    std::vector<std::string> args;
    while (getNextToken() == Token_identifier)
    {
        args.push_back(identifierStr);
    }
    if (__current_token != ')')
        return errorP("expected ')' in prototype");
    getNextToken();
    return new PrototypeAST(funcName, args);
}

static FunctionAST *parseFuncDefinition()
{
    getNextToken();
    PrototypeAST *prototype = parsePrototype();
    if (!prototype)
        return 0;
    ExprAST *expr = parseExpression();
    if (!expr)
        return 0;
    return new FunctionAST(prototype, expr);
}

static PrototypeAST *parseExtern()
{
    getNextToken();
    return parsePrototype();
}

static FunctionAST *parseTopLevelExpr()
{
    if (ExprAST *expr = parseExpression())
    {
        // Make an anonymous proto.
        PrototypeAST *prototype = new PrototypeAST("", std::vector<std::string>());
        return new FunctionAST(prototype, expr);
    }
    return 0;
}

static void HandleDefinition()
{
    if (parseFuncDefinition())
    {
        fprintf(stderr, "Parsed a function definition.\n");
    }
    else
    {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void HandleExtern()
{
    if (parseExtern())
    {
        fprintf(stderr, "Parsed an extern\n");
    }
    else
    {
        // Skip token for error recovery.
        getNextToken();
    }
}

static void HandleTopLevelExpression()
{
    // Evaluate a top-level expression into an anonymous function.
    if (parseTopLevelExpr())
    {
        fprintf(stderr, "Parsed a top-level expr\n");
    }
    else
    {
        // Skip token for error recovery.
        getNextToken();
    }
}

void MainLoop()
{
    while (1)
    {
        fprintf(stderr, "ready> ");
        switch (__current_token)
        {
        case Token_eof:
            return;
        case ';':
            getNextToken();
            break; // ignore top-level semicolons.
        case Token_def:
            HandleDefinition();
            break;
        case Token_extern:
            HandleExtern();
            break;
        default:
            HandleTopLevelExpression();
            break;
        }
    }
}
