#include "chatviewtextprocessor.h"

#include <QBrush>
#include <QChar>
#include <QClipboard>
#include <QFont>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QList>
#include <QPainter>
#include <QQuickTextDocument>
#include <QRegularExpression>
#include <QStringList>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextFrame>
#include <QTextFrameFormat>
#include <QTextTableCell>
#include <QVariant>
#include <Qt>
#include <QtGlobal>

#include <algorithm>

enum Language {
    None,
    Python,
    Cpp,
    Bash,
    TypeScript,
    Java,
    Go,
    Json,
    Csharp,
    Latex,
    Html,
    Php,
    Markdown
};

static Language stringToLanguage(const QString &language)
{
    if (language == "python")
        return Python;
    if (language == "cpp")
        return Cpp;
    if (language == "c++")
        return Cpp;
    if (language == "csharp")
        return Csharp;
    if (language == "c#")
        return Csharp;
    if (language == "c")
        return Cpp;
    if (language == "bash")
        return Bash;
    if (language == "javascript")
        return TypeScript;
    if (language == "typescript")
        return TypeScript;
    if (language == "java")
        return Java;
    if (language == "go")
        return Go;
    if (language == "golang")
        return Go;
    if (language == "json")
        return Json;
    if (language == "latex")
        return Latex;
    if (language == "html")
        return Html;
    if (language == "php")
        return Php;
    return None;
}

enum Code {
    Default,
    Keyword,
    Function,
    FunctionCall,
    Comment,
    String,
    Number,
    Header,
    Preprocessor,
    Type,
    Arrow,
    Command,
    Variable,
    Key,
    Value,
    Parameter,
    AttributeName,
    AttributeValue,
    SpecialCharacter,
    DocType
};

struct HighlightingRule {
    QRegularExpression pattern;
    Code format;
};

static QColor formatToColor(Code c, const CodeColors &colors)
{
    switch (c) {
    case Default: return colors.defaultColor;
    case Keyword: return colors.keywordColor;
    case Function: return colors.functionColor;
    case FunctionCall: return colors.functionCallColor;
    case Comment: return colors.commentColor;
    case String: return colors.stringColor;
    case Number: return colors.numberColor;
    case Header: return colors.headerColor;
    case Preprocessor: return colors.preprocessorColor;
    case Type: return colors.typeColor;
    case Arrow: return colors.arrowColor;
    case Command: return colors.commandColor;
    case Variable: return colors.variableColor;
    case Key: return colors.keyColor;
    case Value: return colors.valueColor;
    case Parameter: return colors.parameterColor;
    case AttributeName: return colors.attributeNameColor;
    case AttributeValue: return colors.attributeValueColor;
    case SpecialCharacter: return colors.specialCharacterColor;
    case DocType: return colors.doctypeColor;
    default: Q_UNREACHABLE();
    }
    return QColor();
}

static QVector<HighlightingRule> pythonHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        rule.pattern = QRegularExpression(".*");
        rule.format = Default;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = FunctionCall;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\bdef\\s+(\\w+)\\b");
        rule.format = Function;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = Number;
        highlightingRules.append(rule);

        QStringList keywordPatterns = {
            "\\bdef\\b", "\\bclass\\b", "\\bif\\b", "\\belse\\b", "\\belif\\b",
            "\\bwhile\\b", "\\bfor\\b", "\\breturn\\b", "\\bprint\\b", "\\bimport\\b",
            "\\bfrom\\b", "\\bas\\b", "\\btry\\b", "\\bexcept\\b", "\\braise\\b",
            "\\bwith\\b", "\\bfinally\\b", "\\bcontinue\\b", "\\bbreak\\b", "\\bpass\\b"
        };

        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = Keyword;
            highlightingRules.append(rule);
        }

        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = String;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\'.*?\'");
        rule.format = String;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("#[^\n]*");
        rule.format = Comment;
        highlightingRules.append(rule);

    }
    return highlightingRules;
}

static QVector<HighlightingRule> csharpHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        rule.pattern = QRegularExpression(".*");
        rule.format = Default;
        highlightingRules.append(rule);

        // Function call highlighting
        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = FunctionCall;
        highlightingRules.append(rule);

        // Function definition highlighting
        rule.pattern = QRegularExpression("\\bvoid|int|double|string|bool\\s+(\\w+)\\s*(?=\\()");
        rule.format = Function;
        highlightingRules.append(rule);

        // Number highlighting
        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = Number;
        highlightingRules.append(rule);

        // Keyword highlighting
        QStringList keywordPatterns = {
            "\\bvoid\\b", "\\bint\\b", "\\bdouble\\b", "\\bstring\\b", "\\bbool\\b",
            "\\bclass\\b", "\\bif\\b", "\\belse\\b", "\\bwhile\\b", "\\bfor\\b",
            "\\breturn\\b", "\\bnew\\b", "\\bthis\\b", "\\bpublic\\b", "\\bprivate\\b",
            "\\bprotected\\b", "\\bstatic\\b", "\\btrue\\b", "\\bfalse\\b", "\\bnull\\b",
            "\\bnamespace\\b", "\\busing\\b", "\\btry\\b", "\\bcatch\\b", "\\bfinally\\b",
            "\\bthrow\\b", "\\bvar\\b"
        };

        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = Keyword;
            highlightingRules.append(rule);
        }

        // String highlighting
        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = String;
        highlightingRules.append(rule);

        // Single-line comment highlighting
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = Comment;
        highlightingRules.append(rule);

        // Multi-line comment highlighting
        rule.pattern = QRegularExpression("/\\*.*?\\*/");
        rule.format = Comment;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}

static QVector<HighlightingRule> cppHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        rule.pattern = QRegularExpression(".*");
        rule.format = Default;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = FunctionCall;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b[a-zA-Z_][a-zA-Z0-9_]*\\s+(\\w+)\\s*\\(");
        rule.format = Function;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = Number;
        highlightingRules.append(rule);

        QStringList keywordPatterns = {
            "\\bauto\\b", "\\bbool\\b", "\\bbreak\\b", "\\bcase\\b", "\\bcatch\\b",
            "\\bchar\\b", "\\bclass\\b", "\\bconst\\b", "\\bconstexpr\\b", "\\bcontinue\\b",
            "\\bdefault\\b", "\\bdelete\\b", "\\bdo\\b", "\\bdouble\\b", "\\belse\\b",
            "\\belifdef\\b", "\\belifndef\\b", "\\bembed\\b", "\\benum\\b", "\\bexplicit\\b",
            "\\bextern\\b", "\\bfalse\\b", "\\bfloat\\b", "\\bfor\\b", "\\bfriend\\b", "\\bgoto\\b",
            "\\bif\\b", "\\binline\\b", "\\bint\\b", "\\blong\\b", "\\bmutable\\b", "\\bnamespace\\b",
            "\\bnew\\b", "\\bnoexcept\\b", "\\bnullptr\\b", "\\boperator\\b", "\\boverride\\b",
            "\\bprivate\\b", "\\bprotected\\b", "\\bpublic\\b", "\\bregister\\b", "\\breinterpret_cast\\b",
            "\\breturn\\b", "\\bshort\\b", "\\bsigned\\b", "\\bsizeof\\b", "\\bstatic\\b", "\\bstatic_assert\\b",
            "\\bstatic_cast\\b", "\\bstruct\\b", "\\bswitch\\b", "\\btemplate\\b", "\\bthis\\b",
            "\\bthrow\\b", "\\btrue\\b", "\\btry\\b", "\\btypedef\\b", "\\btypeid\\b", "\\btypename\\b",
            "\\bunion\\b", "\\bunsigned\\b", "\\busing\\b", "\\bvirtual\\b", "\\bvoid\\b",
            "\\bvolatile\\b", "\\bwchar_t\\b", "\\bwhile\\b"
        };

        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = Keyword;
            highlightingRules.append(rule);
        }

        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = String;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\'.*?\'");
        rule.format = String;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = Comment;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("/\\*.*?\\*/");
        rule.format = Comment;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("#(?:include|define|undef|ifdef|ifndef|if|else|elif|endif|error|pragma)\\b.*");
        rule.format = Preprocessor;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}

static QVector<HighlightingRule> typescriptHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        rule.pattern = QRegularExpression(".*");
        rule.format = Default;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = FunctionCall;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\bfunction\\s+(\\w+)\\b");
        rule.format = Function;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = Number;
        highlightingRules.append(rule);

        QStringList keywordPatterns = {
            "\\bfunction\\b", "\\bvar\\b", "\\blet\\b", "\\bconst\\b", "\\bif\\b", "\\belse\\b",
            "\\bfor\\b", "\\bwhile\\b", "\\breturn\\b", "\\btry\\b", "\\bcatch\\b", "\\bfinally\\b",
            "\\bthrow\\b", "\\bnew\\b", "\\bdelete\\b", "\\btypeof\\b", "\\binstanceof\\b",
            "\\bdo\\b", "\\bswitch\\b", "\\bcase\\b", "\\bbreak\\b", "\\bcontinue\\b",
            "\\bpublic\\b", "\\bprivate\\b", "\\bprotected\\b", "\\bstatic\\b", "\\breadonly\\b",
            "\\benum\\b", "\\binterface\\b", "\\bextends\\b", "\\bimplements\\b", "\\bexport\\b",
            "\\bimport\\b", "\\btype\\b", "\\bnamespace\\b", "\\babstract\\b", "\\bas\\b",
            "\\basync\\b", "\\bawait\\b", "\\bclass\\b", "\\bconstructor\\b", "\\bget\\b",
            "\\bset\\b", "\\bnull\\b", "\\bundefined\\b", "\\btrue\\b", "\\bfalse\\b"
        };

        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = Keyword;
            highlightingRules.append(rule);
        }

        QStringList typePatterns = {
            "\\bstring\\b", "\\bnumber\\b", "\\bboolean\\b", "\\bany\\b", "\\bvoid\\b",
            "\\bnever\\b", "\\bunknown\\b", "\\bObject\\b", "\\bArray\\b"
        };

        for (const QString &pattern : typePatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = Type;
            highlightingRules.append(rule);
        }

        rule.pattern = QRegularExpression("\".*?\"|'.*?'|`.*?`");
        rule.format = String;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = Comment;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("/\\*.*?\\*/");
        rule.format = Comment;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("=>");
        rule.format = Arrow;
        highlightingRules.append(rule);

    }
    return highlightingRules;
}

static QVector<HighlightingRule> javaHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        rule.pattern = QRegularExpression(".*");
        rule.format = Default;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = FunctionCall;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\bvoid\\s+(\\w+)\\b");
        rule.format = Function;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = Number;
        highlightingRules.append(rule);

        QStringList keywordPatterns = {
            "\\bpublic\\b", "\\bprivate\\b", "\\bprotected\\b", "\\bstatic\\b", "\\bfinal\\b",
            "\\bclass\\b", "\\bif\\b", "\\belse\\b", "\\bwhile\\b", "\\bfor\\b",
            "\\breturn\\b", "\\bnew\\b", "\\bimport\\b", "\\bpackage\\b", "\\btry\\b",
            "\\bcatch\\b", "\\bthrow\\b", "\\bthrows\\b", "\\bfinally\\b", "\\binterface\\b",
            "\\bextends\\b", "\\bimplements\\b", "\\bsuper\\b", "\\bthis\\b", "\\bvoid\\b",
            "\\bboolean\\b", "\\bbyte\\b", "\\bchar\\b", "\\bdouble\\b", "\\bfloat\\b",
            "\\bint\\b", "\\blong\\b", "\\bshort\\b", "\\bswitch\\b", "\\bcase\\b",
            "\\bdefault\\b", "\\bcontinue\\b", "\\bbreak\\b", "\\babstract\\b", "\\bassert\\b",
            "\\benum\\b", "\\binstanceof\\b", "\\bnative\\b", "\\bstrictfp\\b", "\\bsynchronized\\b",
            "\\btransient\\b", "\\bvolatile\\b", "\\bconst\\b", "\\bgoto\\b"
        };

        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = Keyword;
            highlightingRules.append(rule);
        }

        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = String;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\'.*?\'");
        rule.format = String;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = Comment;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("/\\*.*?\\*/");
        rule.format = Comment;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}

static QVector<HighlightingRule> goHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        rule.pattern = QRegularExpression(".*");
        rule.format = Default;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = FunctionCall;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\bfunc\\s+(\\w+)\\b");
        rule.format = Function;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = Number;
        highlightingRules.append(rule);

        QStringList keywordPatterns = {
            "\\bfunc\\b", "\\bpackage\\b", "\\bimport\\b", "\\bvar\\b", "\\bconst\\b",
            "\\btype\\b", "\\bstruct\\b", "\\binterface\\b", "\\bfor\\b", "\\bif\\b",
            "\\belse\\b", "\\bswitch\\b", "\\bcase\\b", "\\bdefault\\b", "\\breturn\\b",
            "\\bbreak\\b", "\\bcontinue\\b", "\\bgoto\\b", "\\bfallthrough\\b",
            "\\bdefer\\b", "\\bchan\\b", "\\bmap\\b", "\\brange\\b"
        };

        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = Keyword;
            highlightingRules.append(rule);
        }

        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = String;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("`.*?`");
        rule.format = String;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = Comment;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("/\\*.*?\\*/");
        rule.format = Comment;
        highlightingRules.append(rule);

    }
    return highlightingRules;
}

static QVector<HighlightingRule> bashHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        rule.pattern = QRegularExpression(".*");
        rule.format = Default;
        highlightingRules.append(rule);

        QStringList commandPatterns = {
            "\\b(grep|awk|sed|ls|cat|echo|rm|mkdir|cp|break|alias|eval|cd|exec|head|tail|strings|printf|touch|mv|chmod)\\b"
        };

        for (const QString &pattern : commandPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = Command;
            highlightingRules.append(rule);
        }

        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = Number;
        highlightingRules.append(rule);

        QStringList keywordPatterns = {
            "\\bif\\b", "\\bthen\\b", "\\belse\\b", "\\bfi\\b", "\\bfor\\b",
            "\\bin\\b", "\\bdo\\b", "\\bdone\\b", "\\bwhile\\b", "\\buntil\\b",
            "\\bcase\\b", "\\besac\\b", "\\bfunction\\b", "\\breturn\\b",
            "\\blocal\\b", "\\bdeclare\\b", "\\bunset\\b", "\\bexport\\b",
            "\\breadonly\\b", "\\bshift\\b", "\\bexit\\b"
        };

        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = Keyword;
            highlightingRules.append(rule);
        }

        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = String;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\'.*?\'");
        rule.format = String;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\$(\\w+|\\{[^}]+\\})");
        rule.format = Variable;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("#[^\n]*");
        rule.format = Comment;
        highlightingRules.append(rule);

    }
    return highlightingRules;
}

static QVector<HighlightingRule> latexHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        rule.pattern = QRegularExpression(".*");
        rule.format = Default;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\\\[A-Za-z]+"); // Pattern for LaTeX commands
        rule.format = Command;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("%[^\n]*"); // Pattern for LaTeX comments
        rule.format = Comment;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}

static QVector<HighlightingRule> htmlHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        rule.pattern = QRegularExpression(".*");
        rule.format = Default;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b(\\w+)\\s*=");
        rule.format = AttributeName;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\".*?\"|'.*?'");
        rule.format = AttributeValue;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("<!--.*?-->");
        rule.format = Comment;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("&[a-zA-Z0-9#]*;");
        rule.format = SpecialCharacter;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("<!DOCTYPE.*?>");
        rule.format = DocType;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}

static QVector<HighlightingRule> phpHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        rule.pattern = QRegularExpression(".*");
        rule.format = Default;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = FunctionCall;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\bfunction\\s+(\\w+)\\b");
        rule.format = Function;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = Number;
        highlightingRules.append(rule);

        QStringList keywordPatterns = {
            "\\bif\\b", "\\belse\\b", "\\belseif\\b", "\\bwhile\\b", "\\bfor\\b",
            "\\bforeach\\b", "\\breturn\\b", "\\bprint\\b", "\\binclude\\b", "\\brequire\\b",
            "\\binclude_once\\b", "\\brequire_once\\b", "\\btry\\b", "\\bcatch\\b",
            "\\bfinally\\b", "\\bcontinue\\b", "\\bbreak\\b", "\\bclass\\b", "\\bfunction\\b",
            "\\bnew\\b", "\\bthrow\\b", "\\barray\\b", "\\bpublic\\b", "\\bprivate\\b",
            "\\bprotected\\b", "\\bstatic\\b", "\\bglobal\\b", "\\bisset\\b", "\\bunset\\b",
            "\\bnull\\b", "\\btrue\\b", "\\bfalse\\b"
        };

        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = Keyword;
            highlightingRules.append(rule);
        }

        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = String;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\'.*?\'");
        rule.format = String;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = Comment;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("/\\*.*?\\*/");
        rule.format = Comment;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}


static QVector<HighlightingRule> jsonHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        rule.pattern = QRegularExpression(".*");
        rule.format = Default;
        highlightingRules.append(rule);

        // Key string rule
        rule.pattern = QRegularExpression("\".*?\":");  // keys are typically in the "key": format
        rule.format = Key;
        highlightingRules.append(rule);

        // Value string rule
        rule.pattern = QRegularExpression(":\\s*(\".*?\")");  // values are typically in the : "value" format
        rule.format = Value;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}

SyntaxHighlighter::SyntaxHighlighter(QObject *parent)
    : QSyntaxHighlighter(parent)
{
}

SyntaxHighlighter::~SyntaxHighlighter()
{
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    QTextBlock block = this->currentBlock();

    // Search the first block of the frame we're in for the code to use for highlighting
    int userState = block.userState();
    if (QTextFrame *frame = block.document()->frameAt(block.position())) {
        QTextBlock firstBlock = frame->begin().currentBlock();
        if (firstBlock.isValid())
            userState = firstBlock.userState();
    }

    QVector<HighlightingRule> rules;
    switch (userState) {
    case Python:
        rules = pythonHighlightingRules(); break;
    case Cpp:
        rules = cppHighlightingRules(); break;
    case Csharp:
        rules = csharpHighlightingRules(); break;
    case Bash:
        rules = bashHighlightingRules(); break;
    case TypeScript:
        rules = typescriptHighlightingRules(); break;
    case Java:
        rules = javaHighlightingRules(); break;
    case Go:
        rules = goHighlightingRules(); break;
    case Json:
        rules = jsonHighlightingRules(); break;
    case Latex:
        rules = latexHighlightingRules(); break;
    case Html:
        rules = htmlHighlightingRules(); break;
    case Php:
        rules = phpHighlightingRules(); break;
    default: break;
    }

    for (const HighlightingRule &rule : std::as_const(rules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            int startIndex = match.capturedStart();
            int length = match.capturedLength();
            QTextCharFormat format;
            format.setForeground(formatToColor(rule.format, m_codeColors));
            setFormat(startIndex, length, format);
        }
    }
}

// TODO (Adam) This class replaces characters in the text in order to provide markup and syntax highlighting
// which destroys the original text in favor of the replaced text. This is a problem when we select
// text and then the user tries to 'copy' the text: the original text should be placed in the clipboard
// not the replaced text. A possible solution is to have this class keep a mapping of the original
// indices and the replacement indices and then use the original text that is stored in memory in the
// chat class to populate the clipboard.
ChatViewTextProcessor::ChatViewTextProcessor(QObject *parent)
    : QObject{parent}
    , m_quickTextDocument(nullptr)
    , m_syntaxHighlighter(new SyntaxHighlighter(this))
    , m_shouldProcessText(true)
    , m_fontPixelSize(QGuiApplication::font().pointSizeF())
{
}

QQuickTextDocument* ChatViewTextProcessor::textDocument() const
{
    return m_quickTextDocument;
}

void ChatViewTextProcessor::setTextDocument(QQuickTextDocument* quickTextDocument)
{
    m_quickTextDocument = quickTextDocument;
    m_syntaxHighlighter->setDocument(m_quickTextDocument->textDocument());
    handleTextChanged();
}

void ChatViewTextProcessor::setValue(const QString &value)
{
    m_quickTextDocument->textDocument()->setPlainText(value);
    handleTextChanged();
}

bool ChatViewTextProcessor::tryCopyAtPosition(int position) const
{
    for (const auto &copy : m_copies) {
        if (position >= copy.startPos && position <= copy.endPos) {
            QClipboard *clipboard = QGuiApplication::clipboard();
            clipboard->setText(copy.text);
            return true;
        }
    }
    return false;
}

bool ChatViewTextProcessor::shouldProcessText() const
{
    return m_shouldProcessText;
}

void ChatViewTextProcessor::setShouldProcessText(bool b)
{
    if (m_shouldProcessText == b)
        return;
    m_shouldProcessText = b;
    emit shouldProcessTextChanged();
    handleTextChanged();
}

qreal ChatViewTextProcessor::fontPixelSize() const
{
    return m_fontPixelSize;
}

void ChatViewTextProcessor::setFontPixelSize(qreal sz)
{
    if (m_fontPixelSize == sz)
        return;
    m_fontPixelSize = sz;
    emit fontPixelSizeChanged();
    handleTextChanged();
}

CodeColors ChatViewTextProcessor::codeColors() const
{
    return m_syntaxHighlighter->codeColors();
}

void ChatViewTextProcessor::setCodeColors(const CodeColors &colors)
{
    m_syntaxHighlighter->setCodeColors(colors);
    emit codeColorsChanged();
}

void traverseDocument(QTextDocument *doc, QTextFrame *frame)
{
    QTextFrame *rootFrame = frame ? frame : doc->rootFrame();
    QTextFrame::iterator rootIt;

    if (!frame)
        qDebug() << "Begin traverse";

    for (rootIt = rootFrame->begin(); !rootIt.atEnd(); ++rootIt) {
        QTextFrame *childFrame = rootIt.currentFrame();
        QTextBlock childBlock = rootIt.currentBlock();

        if (childFrame) {
            qDebug() << "Frame from" << childFrame->firstPosition() << "to" << childFrame->lastPosition();
            traverseDocument(doc, childFrame);
        } else if (childBlock.isValid()) {
            qDebug() << QString("    Block %1 position:").arg(childBlock.userState()) << childBlock.position();
            qDebug() << QString("    Block %1 text:").arg(childBlock.userState()) << childBlock.text();

            // Iterate over lines within the block
            for (QTextBlock::iterator blockIt = childBlock.begin(); !(blockIt.atEnd()); ++blockIt) {
                QTextFragment fragment = blockIt.fragment();
                if (fragment.isValid()) {
                    qDebug() << "    Fragment text:" << fragment.text();
                }
            }
        }
    }

    if (!frame)
        qDebug() << "End traverse";
}

void ChatViewTextProcessor::handleTextChanged()
{
    if (!m_quickTextDocument || !m_shouldProcessText)
        return;

    // Force full layout of the text document to work around a bug in Qt
    // TODO(jared): report the Qt bug and link to the report here
    QTextDocument* doc = m_quickTextDocument->textDocument();
    (void)doc->documentLayout()->documentSize();

    handleCodeBlocks();
    handleMarkdown();

    // We insert an invisible char at the end to make sure the document goes back to the default
    // text format
    QTextCursor cursor(doc);
    QString invisibleCharacter = QString(QChar(0xFEFF));
    cursor.insertText(invisibleCharacter, QTextCharFormat());
}

void ChatViewTextProcessor::handleCodeBlocks()
{
    QTextDocument* doc = m_quickTextDocument->textDocument();
    QTextCursor cursor(doc);

    QTextCharFormat textFormat;
    textFormat.setFontFamilies(QStringList() << "Monospace");
    textFormat.setForeground(QColor("white"));

    QTextFrameFormat frameFormatBase;
    frameFormatBase.setBackground(codeColors().backgroundColor);

    QTextTableFormat tableFormat;
    tableFormat.setMargin(0);
    tableFormat.setPadding(0);
    tableFormat.setBorder(0);
    tableFormat.setBorderCollapse(true);
    QList<QTextLength> constraints;
    constraints << QTextLength(QTextLength::PercentageLength, 100);
    tableFormat.setColumnWidthConstraints(constraints);

    QTextTableFormat headerTableFormat;
    headerTableFormat.setBackground(codeColors().headerColor);
    headerTableFormat.setPadding(0);
    headerTableFormat.setBorder(0);
    headerTableFormat.setBorderCollapse(true);
    headerTableFormat.setTopMargin(10);
    headerTableFormat.setBottomMargin(10);
    headerTableFormat.setLeftMargin(15);
    headerTableFormat.setRightMargin(15);
    QList<QTextLength> headerConstraints;
    headerConstraints << QTextLength(QTextLength::PercentageLength, 80);
    headerConstraints << QTextLength(QTextLength::PercentageLength, 20);
    headerTableFormat.setColumnWidthConstraints(headerConstraints);

    QTextTableFormat codeBlockTableFormat;
    codeBlockTableFormat.setBackground(codeColors().backgroundColor);
    codeBlockTableFormat.setPadding(0);
    codeBlockTableFormat.setBorder(0);
    codeBlockTableFormat.setBorderCollapse(true);
    codeBlockTableFormat.setTopMargin(15);
    codeBlockTableFormat.setBottomMargin(15);
    codeBlockTableFormat.setLeftMargin(15);
    codeBlockTableFormat.setRightMargin(15);
    codeBlockTableFormat.setColumnWidthConstraints(constraints);

    QTextImageFormat copyImageFormat;
    copyImageFormat.setWidth(24);
    copyImageFormat.setHeight(24);
    copyImageFormat.setName("qrc:/gpt4all/icons/copy.svg");

    // Regex for code blocks
    static const QRegularExpression reCode("```(.*?)(```|$)", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator iCode = reCode.globalMatch(doc->toPlainText());

    QList<QRegularExpressionMatch> matchesCode;
    while (iCode.hasNext())
        matchesCode.append(iCode.next());

    QVector<CodeCopy> newCopies;
    QVector<QTextFrame*> frames;

    for(int index = matchesCode.count() - 1; index >= 0; --index) {
        cursor.setPosition(matchesCode[index].capturedStart());
        cursor.setPosition(matchesCode[index].capturedEnd(), QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        QTextFrameFormat frameFormat = frameFormatBase;
        QString capturedText = matchesCode[index].captured(1);
        QString codeLanguage;

        QStringList lines = capturedText.split('\n');
        if (lines.last().isEmpty()) {
            lines.removeLast();
        }

        if (lines.count() >= 2) {
            const auto &firstWord = lines.first();
            if (firstWord == "python"
                || firstWord == "cpp"
                || firstWord == "c++"
                || firstWord == "csharp"
                || firstWord == "c#"
                || firstWord == "c"
                || firstWord == "bash"
                || firstWord == "javascript"
                || firstWord == "typescript"
                || firstWord == "java"
                || firstWord == "go"
                || firstWord == "golang"
                || firstWord == "json"
                || firstWord == "latex"
                || firstWord == "html"
                || firstWord == "php") {
                codeLanguage = firstWord;
            }
            lines.removeFirst();
        }

        QTextFrame *mainFrame = cursor.currentFrame();
        cursor.setCharFormat(textFormat);

        cursor.insertFrame(frameFormat);
        QTextTable *table = cursor.insertTable(codeLanguage.isEmpty() ? 1 : 2, 1, tableFormat);

        if (!codeLanguage.isEmpty()) {
            QTextTableCell headerCell = table->cellAt(0, 0);
            QTextCursor headerCellCursor = headerCell.firstCursorPosition();
            QTextTable *headerTable = headerCellCursor.insertTable(1, 2, headerTableFormat);
            QTextTableCell header = headerTable->cellAt(0, 0);
            QTextCursor headerCursor = header.firstCursorPosition();
            headerCursor.insertText(codeLanguage);
            QTextTableCell copy = headerTable->cellAt(0, 1);
            QTextCursor copyCursor = copy.firstCursorPosition();
            CodeCopy newCopy;
            newCopy.text = lines.join("\n");
            newCopy.startPos = copyCursor.position();
            newCopy.endPos = newCopy.startPos + 1;
            newCopies.append(newCopy);
// FIXME: There are two reasons this is commented out. Odd drawing behavior is seen when this is added
// and one selects with the mouse the code language in a code block. The other reason is the code that
// tries to do a hit test for the image is just very broken and buggy and does not always work. So I'm
// disabling this code and included functionality for v3.0.0 until I can figure out how to make this much
// less buggy
#if 0
//            QTextBlockFormat blockFormat;
//            blockFormat.setAlignment(Qt::AlignRight);
//            copyCursor.setBlockFormat(blockFormat);
//            copyCursor.insertImage(copyImageFormat, QTextFrameFormat::FloatRight);
#endif
        }

        QTextTableCell codeCell = table->cellAt(codeLanguage.isEmpty() ? 0 : 1, 0);
        QTextCursor codeCellCursor = codeCell.firstCursorPosition();
        QTextTable *codeTable = codeCellCursor.insertTable(1, 1, codeBlockTableFormat);
        QTextTableCell code = codeTable->cellAt(0, 0);

        QTextCharFormat codeBlockCharFormat;
        codeBlockCharFormat.setForeground(codeColors().defaultColor);

        QFont monospaceFont("Courier");
        monospaceFont.setPointSize(m_fontPixelSize);
        if (monospaceFont.family() != "Courier") {
            monospaceFont.setFamily("Monospace"); // Fallback if Courier isn't available
        }

        QTextCursor codeCursor = code.firstCursorPosition();
        codeBlockCharFormat.setFont(monospaceFont); // Update the font for the codeblock
        codeCursor.setCharFormat(codeBlockCharFormat);

        codeCursor.block().setUserState(stringToLanguage(codeLanguage));
        codeCursor.insertText(lines.join('\n'));

        cursor = mainFrame->lastCursorPosition();
        cursor.setCharFormat(QTextCharFormat());
    }

    m_copies = newCopies;
}

void replaceAndInsertMarkdown(int startIndex, int endIndex, QTextDocument *doc)
{
    QTextCursor cursor(doc);
    cursor.setPosition(startIndex);
    cursor.setPosition(endIndex, QTextCursor::KeepAnchor);
    QTextDocumentFragment fragment(cursor);
    const QString plainText = fragment.toPlainText();
    cursor.removeSelectedText();
    QTextDocument::MarkdownFeatures features = static_cast<QTextDocument::MarkdownFeatures>(
        QTextDocument::MarkdownNoHTML | QTextDocument::MarkdownDialectGitHub);
    cursor.insertMarkdown(plainText, features);
    cursor.block().setUserState(Markdown);
}

void ChatViewTextProcessor::handleMarkdown()
{
    QTextDocument* doc = m_quickTextDocument->textDocument();
    QTextCursor cursor(doc);

    QVector<QPair<int, int>> codeBlockPositions;

    QTextFrame *rootFrame = doc->rootFrame();
    QTextFrame::iterator rootIt;

    bool hasAlreadyProcessedMarkdown = false;
    for (rootIt = rootFrame->begin(); !rootIt.atEnd(); ++rootIt) {
        QTextFrame *childFrame = rootIt.currentFrame();
        QTextBlock childBlock = rootIt.currentBlock();
        if (childFrame) {
            codeBlockPositions.append(qMakePair(childFrame->firstPosition()-1, childFrame->lastPosition()+1));

            for (QTextFrame::iterator frameIt = childFrame->begin(); !frameIt.atEnd(); ++frameIt) {
                QTextBlock block = frameIt.currentBlock();
                if (block.isValid() && block.userState() == Markdown)
                    hasAlreadyProcessedMarkdown = true;
            }
        } else if (childBlock.isValid() && childBlock.userState() == Markdown)
            hasAlreadyProcessedMarkdown = true;
    }


    if (!hasAlreadyProcessedMarkdown) {
        std::sort(codeBlockPositions.begin(), codeBlockPositions.end(), [](const QPair<int, int> &a, const QPair<int, int> &b) {
            return a.first > b.first;
        });

        int lastIndex = doc->characterCount() - 1;
        for (const auto &pos : codeBlockPositions) {
            int nonCodeStart = pos.second;
            int nonCodeEnd = lastIndex;
            if (nonCodeEnd > nonCodeStart) {
                replaceAndInsertMarkdown(nonCodeStart, nonCodeEnd, doc);
            }
            lastIndex = pos.first;
        }

        if (lastIndex > 0)
            replaceAndInsertMarkdown(0, lastIndex, doc);
    }
}
