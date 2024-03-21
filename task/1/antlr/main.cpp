#include "SYsU_lang.h" // 确保这里的头文件名与您生成的词法分析器匹配
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <algorithm>

// 映射定义，将ANTLR的tokenTypeName映射到clang的格式
std::unordered_map<std::string, std::string> gTokenTypeMapping = {
  { "LeftParen", "l_paren" },
  { "RightParen", "r_paren" },
  { "RightBrace", "r_brace" },
  { "LeftBrace", "l_brace" },
  { "LeftBracket", "l_square" },
  { "RightBracket", "r_square" },
  { "Constant", "numeric_constant" },

  // 在这里继续添加其他映射
};

void
print_token(const antlr4::Token* token,
            const antlr4::CommonTokenStream& tokens,
            std::ofstream& outFile,
            const antlr4::Lexer& lexer)
{
  auto& vocabulary = lexer.getVocabulary();

  auto tokenTypeName =
    std::string(vocabulary.getSymbolicName(token->getType()));

  if (tokenTypeName.empty())
    tokenTypeName = "<UNKNOWN>"; // 处理可能的空字符串情况
  else if (gTokenTypeMapping.find(tokenTypeName) != gTokenTypeMapping.end()) {
    tokenTypeName = gTokenTypeMapping[tokenTypeName];
  }else // 默认转为小写
    std::transform(tokenTypeName.begin(), tokenTypeName.end(), tokenTypeName.begin(), ::tolower);

  // 定位文件和行号
  static std::string sCurrentFileName;
  static int sLineOffset;
  if(tokenTypeName == "lineafterpreprocessing"){
    std::stringstream commentLine(token->getText());
    char hashtag;
    int starOfLineInFile;
    commentLine >> hashtag >> starOfLineInFile >> sCurrentFileName;
    sLineOffset = starOfLineInFile - token->getLine() - 1;
    sCurrentFileName.erase(0, 1); // 去除""
    sCurrentFileName.erase(sCurrentFileName.size() - 1);
    return;
  }

  // 行首和空格标记
  static bool sStartOfLine = false;
  static bool sLeadingSpace = false;
  if(tokenTypeName == "newline"){
    sStartOfLine = true;
    return;
  }
  if(tokenTypeName == "whitespace"){
    sLeadingSpace = true;
    return;
  }

  if (token->getText() != "<EOF>")
    outFile << tokenTypeName << " '" << token->getText() << "'";
  else
    outFile << tokenTypeName << " '"
            << "'";
  if (sStartOfLine){
    outFile << "\t [StartOfLine]";
    sStartOfLine = false;
  }
  if (sLeadingSpace){
    outFile << " [LeadingSpace]";
    sLeadingSpace = false;
  }
  // location info
  outFile << " Loc=<" << sCurrentFileName
          << ":" << token->getLine() + sLineOffset
          << ":" << token->getCharPositionInLine() + 1 // 列号转换为1开始
          << ">" << std::endl;
}

int
main(int argc, char* argv[])
{
  if (argc != 3) {
    std::cout << "Usage: " << argv[0] << " <input> <output>\n";
    return -1;
  }

  std::ifstream inFile(argv[1]);
  if (!inFile) {
    std::cout << "Error: unable to open input file: " << argv[1] << '\n';
    return -2;
  }

  std::ofstream outFile(argv[2]);
  if (!outFile) {
    std::cout << "Error: unable to open output file: " << argv[2] << '\n';
    return -3;
  }

  std::cout << "程序 '" << argv[0] << std::endl;
  std::cout << "输入 '" << argv[1] << std::endl;
  std::cout << "输出 '" << argv[2] << std::endl;

  antlr4::ANTLRInputStream input(inFile);
  SYsU_lang lexer(&input);

  antlr4::CommonTokenStream tokens(&lexer);
  tokens.fill();

  for (auto&& token : tokens.getTokens()) {
    print_token(token, tokens, outFile, lexer);
  }
}
