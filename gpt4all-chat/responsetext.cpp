#include "responsetext.h"

#include <QTextCharFormat>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QFontMetricsF>
#include <QTextTableCell>
#include <QGuiApplication>
#include <QClipboard>

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
    Php
};


static QColor defaultColor      = "#d1d5db"; // white
static QColor keywordColor      = "#2e95d3"; // blue
static QColor functionColor     = "#f22c3d"; // red
static QColor functionCallColor = "#e9950c"; // orange
static QColor commentColor      = "#808080"; // gray
static QColor stringColor       = "#00a37d"; // green
static QColor numberColor       = "#df3079"; // fuchsia
static QColor preprocessorColor = keywordColor;
static QColor typeColor = numberColor;
static QColor arrowColor = functionColor;
static QColor commandColor = functionCallColor;
static QColor variableColor = numberColor;
static QColor keyColor = functionColor;
static QColor valueColor = stringColor;
static QColor parameterColor = stringColor;
static QColor attributeNameColor = numberColor;
static QColor attributeValueColor = stringColor;
static QColor specialCharacterColor = functionColor;
static QColor doctypeColor = commentColor;

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

struct HighlightingRule {
    QRegularExpression pattern;
    QTextCharFormat format;
};

static QVector<HighlightingRule> pythonHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        QTextCharFormat defaultFormat;
        defaultFormat.setForeground(defaultColor);
        rule.pattern = QRegularExpression(".*");
        rule.format = defaultFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionCallFormat;
        functionCallFormat.setForeground(functionCallColor);
        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = functionCallFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionFormat;
        functionFormat.setForeground(functionColor);
        rule.pattern = QRegularExpression("\\bdef\\s+(\\w+)\\b");
        rule.format = functionFormat;
        highlightingRules.append(rule);

        QTextCharFormat numberFormat;
        numberFormat.setForeground(numberColor);
        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = numberFormat;
        highlightingRules.append(rule);

        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(keywordColor);
        QStringList keywordPatterns = {
            "\\bdef\\b", "\\bclass\\b", "\\bif\\b", "\\belse\\b", "\\belif\\b",
            "\\bwhile\\b", "\\bfor\\b", "\\breturn\\b", "\\bprint\\b", "\\bimport\\b",
            "\\bfrom\\b", "\\bas\\b", "\\btry\\b", "\\bexcept\\b", "\\braise\\b",
            "\\bwith\\b", "\\bfinally\\b", "\\bcontinue\\b", "\\bbreak\\b", "\\bpass\\b"
        };

        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        QTextCharFormat stringFormat;
        stringFormat.setForeground(stringColor);
        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\'.*?\'");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        QTextCharFormat commentFormat;
        commentFormat.setForeground(commentColor);
        rule.pattern = QRegularExpression("#[^\n]*");
        rule.format = commentFormat;
        highlightingRules.append(rule);

    }
    return highlightingRules;
}

static QVector<HighlightingRule> csharpHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        QTextCharFormat defaultFormat;
        defaultFormat.setForeground(defaultColor);
        rule.pattern = QRegularExpression(".*");
        rule.format = defaultFormat;
        highlightingRules.append(rule);

        // Function call highlighting
        QTextCharFormat functionCallFormat;
        functionCallFormat.setForeground(functionCallColor);
        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = functionCallFormat;
        highlightingRules.append(rule);

        // Function definition highlighting
        QTextCharFormat functionFormat;
        functionFormat.setForeground(functionColor);
        rule.pattern = QRegularExpression("\\bvoid|int|double|string|bool\\s+(\\w+)\\s*(?=\\()");
        rule.format = functionFormat;
        highlightingRules.append(rule);

        // Number highlighting
        QTextCharFormat numberFormat;
        numberFormat.setForeground(numberColor);
        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = numberFormat;
        highlightingRules.append(rule);

        // Keyword highlighting
        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(keywordColor);
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
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        // String highlighting
        QTextCharFormat stringFormat;
        stringFormat.setForeground(stringColor);
        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        // Single-line comment highlighting
        QTextCharFormat commentFormat;
        commentFormat.setForeground(commentColor);
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = commentFormat;
        highlightingRules.append(rule);

        // Multi-line comment highlighting
        rule.pattern = QRegularExpression("/\\*.*?\\*/");
        rule.format = commentFormat;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}

static QVector<HighlightingRule> cppHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        QTextCharFormat defaultFormat;
        defaultFormat.setForeground(defaultColor);
        rule.pattern = QRegularExpression(".*");
        rule.format = defaultFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionCallFormat;
        functionCallFormat.setForeground(functionCallColor);
        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = functionCallFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionFormat;
        functionFormat.setForeground(functionColor);
        rule.pattern = QRegularExpression("\\b[a-zA-Z_][a-zA-Z0-9_]*\\s+(\\w+)\\s*\\(");
        rule.format = functionFormat;
        highlightingRules.append(rule);

        QTextCharFormat numberFormat;
        numberFormat.setForeground(numberColor);
        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = numberFormat;
        highlightingRules.append(rule);

        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(keywordColor);
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
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        QTextCharFormat stringFormat;
        stringFormat.setForeground(stringColor);
        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\'.*?\'");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        QTextCharFormat commentFormat;
        commentFormat.setForeground(commentColor);
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = commentFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("/\\*.*?\\*/");
        rule.format = commentFormat;
        highlightingRules.append(rule);

        QTextCharFormat preprocessorFormat;
        preprocessorFormat.setForeground(preprocessorColor);
        rule.pattern = QRegularExpression("#(?:include|define|undef|ifdef|ifndef|if|else|elif|endif|error|pragma)\\b.*");
        rule.format = preprocessorFormat;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}

static QVector<HighlightingRule> typescriptHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        QTextCharFormat defaultFormat;
        defaultFormat.setForeground(defaultColor);
        rule.pattern = QRegularExpression(".*");
        rule.format = defaultFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionCallFormat;
        functionCallFormat.setForeground(functionCallColor);
        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = functionCallFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionFormat;
        functionFormat.setForeground(functionColor);
        rule.pattern = QRegularExpression("\\bfunction\\s+(\\w+)\\b");
        rule.format = functionFormat;
        highlightingRules.append(rule);

        QTextCharFormat numberFormat;
        numberFormat.setForeground(numberColor);
        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = numberFormat;
        highlightingRules.append(rule);

        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(keywordColor);
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
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        QTextCharFormat typeFormat;
        typeFormat.setForeground(typeColor);
        QStringList typePatterns = {
            "\\bstring\\b", "\\bnumber\\b", "\\bboolean\\b", "\\bany\\b", "\\bvoid\\b",
            "\\bnever\\b", "\\bunknown\\b", "\\bObject\\b", "\\bArray\\b"
        };

        for (const QString &pattern : typePatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = typeFormat;
            highlightingRules.append(rule);
        }

        QTextCharFormat stringFormat;
        stringFormat.setForeground(stringColor);
        rule.pattern = QRegularExpression("\".*?\"|'.*?'|`.*?`");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        QTextCharFormat commentFormat;
        commentFormat.setForeground(commentColor);
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = commentFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("/\\*.*?\\*/");
        rule.format = commentFormat;
        highlightingRules.append(rule);

        QTextCharFormat arrowFormat;
        arrowFormat.setForeground(arrowColor);
        rule.pattern = QRegularExpression("=>");
        rule.format = arrowFormat;
        highlightingRules.append(rule);

    }
    return highlightingRules;
}

static QVector<HighlightingRule> javaHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        QTextCharFormat defaultFormat;
        defaultFormat.setForeground(defaultColor);
        rule.pattern = QRegularExpression(".*");
        rule.format = defaultFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionCallFormat;
        functionCallFormat.setForeground(functionCallColor);
        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = functionCallFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionFormat;
        functionFormat.setForeground(functionColor);
        rule.pattern = QRegularExpression("\\bvoid\\s+(\\w+)\\b");
        rule.format = functionFormat;
        highlightingRules.append(rule);

        QTextCharFormat numberFormat;
        numberFormat.setForeground(numberColor);
        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = numberFormat;
        highlightingRules.append(rule);

        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(keywordColor);
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
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        QTextCharFormat stringFormat;
        stringFormat.setForeground(stringColor);
        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\'.*?\'");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        QTextCharFormat commentFormat;
        commentFormat.setForeground(commentColor);
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = commentFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("/\\*.*?\\*/");
        rule.format = commentFormat;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}

static QVector<HighlightingRule> goHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        QTextCharFormat defaultFormat;
        defaultFormat.setForeground(defaultColor);
        rule.pattern = QRegularExpression(".*");
        rule.format = defaultFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionCallFormat;
        functionCallFormat.setForeground(functionCallColor);
        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = functionCallFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionFormat;
        functionFormat.setForeground(functionColor);
        rule.pattern = QRegularExpression("\\bfunc\\s+(\\w+)\\b");
        rule.format = functionFormat;
        highlightingRules.append(rule);

        QTextCharFormat numberFormat;
        numberFormat.setForeground(numberColor);
        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = numberFormat;
        highlightingRules.append(rule);

        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(keywordColor);
        QStringList keywordPatterns = {
            "\\bfunc\\b", "\\bpackage\\b", "\\bimport\\b", "\\bvar\\b", "\\bconst\\b",
            "\\btype\\b", "\\bstruct\\b", "\\binterface\\b", "\\bfor\\b", "\\bif\\b",
            "\\belse\\b", "\\bswitch\\b", "\\bcase\\b", "\\bdefault\\b", "\\breturn\\b",
            "\\bbreak\\b", "\\bcontinue\\b", "\\bgoto\\b", "\\bfallthrough\\b",
            "\\bdefer\\b", "\\bchan\\b", "\\bmap\\b", "\\brange\\b"
        };

        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        QTextCharFormat stringFormat;
        stringFormat.setForeground(stringColor);
        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("`.*?`");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        QTextCharFormat commentFormat;
        commentFormat.setForeground(commentColor);
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = commentFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("/\\*.*?\\*/");
        rule.format = commentFormat;
        highlightingRules.append(rule);

    }
    return highlightingRules;
}

static QVector<HighlightingRule> bashHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        QTextCharFormat defaultFormat;
        defaultFormat.setForeground(defaultColor);
        rule.pattern = QRegularExpression(".*");
        rule.format = defaultFormat;
        highlightingRules.append(rule);

        QTextCharFormat commandFormat;
        commandFormat.setForeground(commandColor);
        QStringList commandPatterns = {
            "\\b(grep|awk|sed|ls|cat|echo|rm|mkdir|cp|break|alias|eval|cd|exec|head|tail|strings|printf|touch|mv|chmod)\\b"
        };

        for (const QString &pattern : commandPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = commandFormat;
            highlightingRules.append(rule);
        }

        QTextCharFormat numberFormat;
        numberFormat.setForeground(numberColor);
        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = numberFormat;
        highlightingRules.append(rule);

        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(keywordColor);
        QStringList keywordPatterns = {
            "\\bif\\b", "\\bthen\\b", "\\belse\\b", "\\bfi\\b", "\\bfor\\b",
            "\\bin\\b", "\\bdo\\b", "\\bdone\\b", "\\bwhile\\b", "\\buntil\\b",
            "\\bcase\\b", "\\besac\\b", "\\bfunction\\b", "\\breturn\\b",
            "\\blocal\\b", "\\bdeclare\\b", "\\bunset\\b", "\\bexport\\b",
            "\\breadonly\\b", "\\bshift\\b", "\\bexit\\b"
        };

        for (const QString &pattern : keywordPatterns) {
            rule.pattern = QRegularExpression(pattern);
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        QTextCharFormat stringFormat;
        stringFormat.setForeground(stringColor);
        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\'.*?\'");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        QTextCharFormat variableFormat;
        variableFormat.setForeground(variableColor);
        rule.pattern = QRegularExpression("\\$(\\w+|\\{[^}]+\\})");
        rule.format = variableFormat;
        highlightingRules.append(rule);

        QTextCharFormat commentFormat;
        commentFormat.setForeground(commentColor);
        rule.pattern = QRegularExpression("#[^\n]*");
        rule.format = commentFormat;
        highlightingRules.append(rule);

    }
    return highlightingRules;
}

static QVector<HighlightingRule> latexHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        QTextCharFormat defaultFormat;
        defaultFormat.setForeground(defaultColor);
        rule.pattern = QRegularExpression(".*");
        rule.format = defaultFormat;
        highlightingRules.append(rule);

        QTextCharFormat commandFormat;
        commandFormat.setForeground(commandColor); // commandColor needs to be set to your liking
        rule.pattern = QRegularExpression("\\\\[A-Za-z]+"); // Pattern for LaTeX commands
        rule.format = commandFormat;
        highlightingRules.append(rule);

        QTextCharFormat commentFormat;
        commentFormat.setForeground(commentColor); // commentColor needs to be set to your liking
        rule.pattern = QRegularExpression("%[^\n]*"); // Pattern for LaTeX comments
        rule.format = commentFormat;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}

static QVector<HighlightingRule> htmlHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        QTextCharFormat defaultFormat;
        defaultFormat.setForeground(defaultColor);
        rule.pattern = QRegularExpression(".*");
        rule.format = defaultFormat;
        highlightingRules.append(rule);

        QTextCharFormat attributeNameFormat;
        attributeNameFormat.setForeground(attributeNameColor);
        rule.pattern = QRegularExpression("\\b(\\w+)\\s*=");
        rule.format = attributeNameFormat;
        highlightingRules.append(rule);

        QTextCharFormat attributeValueFormat;
        attributeValueFormat.setForeground(attributeValueColor);
        rule.pattern = QRegularExpression("\".*?\"|'.*?'");
        rule.format = attributeValueFormat;
        highlightingRules.append(rule);

        QTextCharFormat commentFormat;
        commentFormat.setForeground(commentColor);
        rule.pattern = QRegularExpression("<!--.*?-->");
        rule.format = commentFormat;
        highlightingRules.append(rule);

        QTextCharFormat specialCharacterFormat;
        specialCharacterFormat.setForeground(specialCharacterColor);
        rule.pattern = QRegularExpression("&[a-zA-Z0-9#]*;");
        rule.format = specialCharacterFormat;
        highlightingRules.append(rule);

        QTextCharFormat doctypeFormat;
        doctypeFormat.setForeground(doctypeColor);
        rule.pattern = QRegularExpression("<!DOCTYPE.*?>");
        rule.format = doctypeFormat;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}

static QVector<HighlightingRule> phpHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        QTextCharFormat defaultFormat;
        defaultFormat.setForeground(defaultColor);
        rule.pattern = QRegularExpression(".*");
        rule.format = defaultFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionCallFormat;
        functionCallFormat.setForeground(functionCallColor);
        rule.pattern = QRegularExpression("\\b(\\w+)\\s*(?=\\()");
        rule.format = functionCallFormat;
        highlightingRules.append(rule);

        QTextCharFormat functionFormat;
        functionFormat.setForeground(functionColor);
        rule.pattern = QRegularExpression("\\bfunction\\s+(\\w+)\\b");
        rule.format = functionFormat;
        highlightingRules.append(rule);

        QTextCharFormat numberFormat;
        numberFormat.setForeground(numberColor);
        rule.pattern = QRegularExpression("\\b[0-9]*\\.?[0-9]+\\b");
        rule.format = numberFormat;
        highlightingRules.append(rule);

        QTextCharFormat keywordFormat;
        keywordFormat.setForeground(keywordColor);
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
            rule.format = keywordFormat;
            highlightingRules.append(rule);
        }

        QTextCharFormat stringFormat;
        stringFormat.setForeground(stringColor);
        rule.pattern = QRegularExpression("\".*?\"");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("\'.*?\'");
        rule.format = stringFormat;
        highlightingRules.append(rule);

        QTextCharFormat commentFormat;
        commentFormat.setForeground(commentColor);
        rule.pattern = QRegularExpression("//[^\n]*");
        rule.format = commentFormat;
        highlightingRules.append(rule);

        rule.pattern = QRegularExpression("/\\*.*?\\*/");
        rule.format = commentFormat;
        highlightingRules.append(rule);
    }
    return highlightingRules;
}


static QVector<HighlightingRule> jsonHighlightingRules()
{
    static QVector<HighlightingRule> highlightingRules;
    if (highlightingRules.isEmpty()) {

        HighlightingRule rule;

        QTextCharFormat defaultFormat;
        defaultFormat.setForeground(defaultColor);
        rule.pattern = QRegularExpression(".*");
        rule.format = defaultFormat;
        highlightingRules.append(rule);

        // Key string rule
        QTextCharFormat keyFormat;
        keyFormat.setForeground(keyColor);  // Assuming keyColor is defined
        rule.pattern = QRegularExpression("\".*?\":");  // keys are typically in the "key": format
        rule.format = keyFormat;
        highlightingRules.append(rule);

        // Value string rule
        QTextCharFormat valueFormat;
        valueFormat.setForeground(valueColor);  // Assuming valueColor is defined
        rule.pattern = QRegularExpression(":\\s*(\".*?\")");  // values are typically in the : "value" format
        rule.format = valueFormat;
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

    QVector<HighlightingRule> rules;
    if (block.userState() == Python)
        rules = pythonHighlightingRules();
    else if (block.userState() == Cpp)
        rules = cppHighlightingRules();
    else if (block.userState() == Csharp)
        rules = csharpHighlightingRules();
    else if (block.userState() == Bash)
        rules = bashHighlightingRules();
    else if (block.userState() == TypeScript)
        rules = typescriptHighlightingRules();
    else if (block.userState() == Java)
        rules = javaHighlightingRules();
    else if (block.userState() == Go)
        rules = javaHighlightingRules();
    else if (block.userState() == Json)
        rules = jsonHighlightingRules();
    else if (block.userState() == Latex)
        rules = latexHighlightingRules();
    else if (block.userState() == Html)
        rules = htmlHighlightingRules();
    else if (block.userState() == Php)
        rules = phpHighlightingRules();

    for (const HighlightingRule &rule : qAsConst(rules)) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            int startIndex = match.capturedStart();
            int length = match.capturedLength();
            setFormat(startIndex, length, rule.format);
        }
    }
}

ResponseText::ResponseText(QObject *parent)
    : QObject{parent}
    , m_textDocument(nullptr)
    , m_syntaxHighlighter(new SyntaxHighlighter(this))
    , m_isProcessingText(false)
{
}

QQuickTextDocument* ResponseText::textDocument() const
{
    return m_textDocument;
}

void ResponseText::setTextDocument(QQuickTextDocument* textDocument)
{
    if (m_textDocument)
        disconnect(m_textDocument->textDocument(), &QTextDocument::contentsChanged, this, &ResponseText::handleTextChanged);

    m_textDocument = textDocument;
    m_syntaxHighlighter->setDocument(m_textDocument->textDocument());
    connect(m_textDocument->textDocument(), &QTextDocument::contentsChanged, this, &ResponseText::handleTextChanged);
    handleTextChanged();
}

QString ResponseText::getLinkAtPosition(int position) const
{
    int i = 0;
    for (const auto &link : m_links) {
        if (position >= link.startPos && position < link.endPos)
            return link.href;
    }
    return QString();
}

bool ResponseText::tryCopyAtPosition(int position) const
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

void ResponseText::handleTextChanged()
{
    if (!m_textDocument || m_isProcessingText)
        return;

    m_isProcessingText = true;
    handleCodeBlocks();
    handleContextLinks();
    // We insert an invisible char at the end to make sure the document goes back to the default
    // text format
    QTextDocument* doc = m_textDocument->textDocument();
    QTextCursor cursor(doc);
    QString invisibleCharacter = QString(QChar(0xFEFF));
    cursor.insertText(invisibleCharacter, QTextCharFormat());
    m_isProcessingText = false;
}

void ResponseText::handleContextLinks()
{
    QTextDocument* doc = m_textDocument->textDocument();
    QTextCursor cursor(doc);
    QTextCharFormat linkFormat;
    linkFormat.setForeground(m_linkColor);
    linkFormat.setFontUnderline(true);

    // Regex for context links
    static const QRegularExpression reLink("\\[Context\\]\\((context://\\d+)\\)");
    QRegularExpressionMatchIterator iLink = reLink.globalMatch(doc->toPlainText());

    QList<QRegularExpressionMatch> matchesLink;
    while (iLink.hasNext())
        matchesLink.append(iLink.next());

    QVector<ContextLink> newLinks;

    // Calculate new positions and store them in newLinks
    int positionOffset = 0;
    for(const auto &match : matchesLink) {
        ContextLink newLink;
        newLink.href = match.captured(1);
        newLink.text = "Context";
        newLink.startPos = match.capturedStart() - positionOffset;
        newLink.endPos = newLink.startPos + newLink.text.length();
        newLinks.append(newLink);
        positionOffset += match.capturedLength() - newLink.text.length();
    }

    // Replace the context links with the word "Context" in reverse order
    for(int index = matchesLink.count() - 1; index >= 0; --index) {
        cursor.setPosition(matchesLink.at(index).capturedStart());
        cursor.setPosition(matchesLink.at(index).capturedEnd(), QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
        cursor.setCharFormat(linkFormat);
        cursor.insertText(newLinks.at(index).text);
        cursor.setCharFormat(QTextCharFormat());
    }

    m_links = newLinks;
}

void ResponseText::handleCodeBlocks()
{
    QTextDocument* doc = m_textDocument->textDocument();
    QTextCursor cursor(doc);

    QTextCharFormat textFormat;
    textFormat.setFontFamilies(QStringList() << "Monospace");
    textFormat.setForeground(QColor("white"));

    QTextFrameFormat frameFormatBase;
    frameFormatBase.setBackground(QColor("black"));

    QTextTableFormat tableFormat;
    tableFormat.setMargin(0);
    tableFormat.setPadding(0);
    tableFormat.setBorder(0);
    tableFormat.setBorderCollapse(true);
    QList<QTextLength> constraints;
    constraints << QTextLength(QTextLength::PercentageLength, 100);
    tableFormat.setColumnWidthConstraints(constraints);

    QTextTableFormat headerTableFormat;
    headerTableFormat.setBackground(m_headerColor);
    headerTableFormat.setPadding(0);
    headerTableFormat.setBorder(0);
    headerTableFormat.setBorderCollapse(true);
    headerTableFormat.setTopMargin(15);
    headerTableFormat.setBottomMargin(15);
    headerTableFormat.setLeftMargin(30);
    headerTableFormat.setRightMargin(30);
    QList<QTextLength> headerConstraints;
    headerConstraints << QTextLength(QTextLength::PercentageLength, 80);
    headerConstraints << QTextLength(QTextLength::PercentageLength, 20);
    headerTableFormat.setColumnWidthConstraints(headerConstraints);

    QTextTableFormat codeBlockTableFormat;
    codeBlockTableFormat.setBackground(QColor("black"));
    codeBlockTableFormat.setPadding(0);
    codeBlockTableFormat.setBorder(0);
    codeBlockTableFormat.setBorderCollapse(true);
    codeBlockTableFormat.setTopMargin(30);
    codeBlockTableFormat.setBottomMargin(30);
    codeBlockTableFormat.setLeftMargin(30);
    codeBlockTableFormat.setRightMargin(30);
    codeBlockTableFormat.setColumnWidthConstraints(constraints);

    QTextImageFormat copyImageFormat;
    copyImageFormat.setWidth(30);
    copyImageFormat.setHeight(30);
    copyImageFormat.setName("qrc:/gpt4all/icons/copy.svg");

    // Regex for code blocks
    static const QRegularExpression reCode("```(.*?)(```|$)", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatchIterator iCode = reCode.globalMatch(doc->toPlainText());

    static const QRegularExpression reWhitespace("^\\s*(\\w+)");

    QList<QRegularExpressionMatch> matchesCode;
    while (iCode.hasNext())
        matchesCode.append(iCode.next());

    QVector<CodeCopy> newCopies;

    for(int index = matchesCode.count() - 1; index >= 0; --index) {
        cursor.setPosition(matchesCode[index].capturedStart());
        cursor.setPosition(matchesCode[index].capturedEnd(), QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        QTextFrameFormat frameFormat = frameFormatBase;
        QString capturedText = matchesCode[index].captured(1);
        QString codeLanguage;

        QRegularExpressionMatch match = reWhitespace.match(capturedText);
        if (match.hasMatch()) {
            const QString firstWord = match.captured(1).trimmed();
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
                capturedText.remove(0, match.captured(0).length());
            }
        }

        const QStringList lines = capturedText.split('\n');

        QTextFrame *mainFrame = cursor.currentFrame();
        cursor.setCharFormat(textFormat);

        QTextFrame *frame = cursor.insertFrame(frameFormat);
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
            int startPos = copyCursor.position();
            CodeCopy newCopy;
            newCopy.text = lines.join("\n");
            newCopy.startPos = copyCursor.position();
            newCopy.endPos = newCopy.startPos + 1;
            newCopies.append(newCopy);
            QTextBlockFormat blockFormat;
            blockFormat.setAlignment(Qt::AlignRight);
            copyCursor.setBlockFormat(blockFormat);
            copyCursor.insertImage(copyImageFormat, QTextFrameFormat::FloatRight);
        }

        QTextTableCell codeCell = table->cellAt(codeLanguage.isEmpty() ? 0 : 1, 0);
        QTextCursor codeCellCursor = codeCell.firstCursorPosition();
        QTextTable *codeTable = codeCellCursor.insertTable(1, 1, codeBlockTableFormat);
        QTextTableCell code = codeTable->cellAt(0, 0);

        QTextCharFormat codeBlockCharFormat;
        QFont monospaceFont("Courier");
        monospaceFont.setPointSize(QGuiApplication::font().pointSize() + 2);
        if (monospaceFont.family() != "Courier") {
            monospaceFont.setFamily("Monospace"); // Fallback if Courier isn't available
        }

        QTextCursor codeCursor = code.firstCursorPosition();
        codeBlockCharFormat.setFont(monospaceFont); // Update the font for the codeblock
        codeCursor.setCharFormat(codeBlockCharFormat);

        if (!codeLanguage.isEmpty()) {
            codeCursor.block().setUserState(stringToLanguage(codeLanguage));
            for (const QString &line : lines) {
                codeCursor.insertText(line);
                codeCursor.insertBlock();
                codeCursor.block().setUserState(stringToLanguage(codeLanguage));
            }
        } else {
            codeCursor.insertText(lines.join("\n"));
        }
        if (codeCursor.position() > 0) {
            codeCursor.setPosition(codeCursor.position() - 1);
            codeCursor.deleteChar();
        }

        cursor = mainFrame->lastCursorPosition();
        cursor.setCharFormat(QTextCharFormat());
    }

    m_copies = newCopies;
}
