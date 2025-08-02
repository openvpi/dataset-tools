// MIT License
//
// Copyright (c) 2019, Eiichiro Iwata. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef TEXTGRID_HPP_
#define TEXTGRID_HPP_

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace textgrid {

class Exception : public std::runtime_error {
  static std::string CreateWhat(const std::string& message, int lineno) {
    return message + " (line " + std::to_string(lineno) + ").";
  }

 public:
  Exception(const std::string& message, int lineno)
      : std::runtime_error(""),
        message_(message),
        lineno_(lineno),
        what_(CreateWhat(message, lineno)) {}
  ~Exception() noexcept override = default;

  const char* what() const noexcept override { return what_.c_str(); }

  std::string GetMessage() const { return message_; }
  int GetLineNumber() const noexcept { return lineno_; }

 private:
  std::string message_;
  int lineno_;
  std::string what_;
};

class LexicalError : public Exception {
 public:
  LexicalError(const std::string& message, int lineno) : Exception(message, lineno) {}
  ~LexicalError() noexcept override = default;
};

class SyntaxError : public Exception {
 public:
  SyntaxError(const std::string& message, int lineno) : Exception(message, lineno) {}
  ~SyntaxError() noexcept override = default;
};

class TextGrid;
class IntervalTier;
struct Interval;
class PointTier;
struct Point;

class TextGridVisitor {
 public:
  virtual ~TextGridVisitor() noexcept = default;

  virtual void Visit(const TextGrid& text_grid) = 0;
  virtual void Visit(const IntervalTier& interval_tier) = 0;
  virtual void Visit(const Interval& interval) = 0;
  virtual void Visit(const PointTier& point_tier) = 0;
  virtual void Visit(const Point& point) = 0;
};

typedef double Number;

struct Point {
  Point() = default;
  explicit Point(Number time, const std::string& text = "") : time(time), text(text) {}

  void Accept(TextGridVisitor& visitor) const { visitor.Visit(*this); }
  void Accept(TextGridVisitor&& visitor) const { visitor.Visit(*this); }

  Number time;
  std::string text;
};

struct Interval {
  Interval() = default;
  Interval(Number min_time, Number max_time, const std::string& text = "")
      : min_time(min_time), max_time(max_time), text(text) {}

  void Accept(TextGridVisitor& visitor) const { visitor.Visit(*this); }
  void Accept(TextGridVisitor&& visitor) const { visitor.Visit(*this); }

  Number min_time;
  Number max_time;
  std::string text;
};

class Tier {
 public:
  Tier(const std::string& name, Number min_time, Number max_time)
      : name_(name), min_time_(min_time), max_time_(max_time) {}
  virtual ~Tier() noexcept = default;

  const std::string& GetName() const& { return name_; }
  std::string GetName() && { return name_; }
  void SetName(const std::string& name) { name_ = name; }
  Number GetMinTime() const noexcept { return min_time_; }
  void SetMinTime(Number min_time) { min_time_ = min_time; }
  Number GetMaxTime() const noexcept { return max_time_; }
  void SetMaxTime(Number max_time) { max_time_ = max_time; }

  virtual size_t GetNumberOfAnnotations() const noexcept = 0;

  virtual void Accept(TextGridVisitor& visitor) const = 0;
  virtual void Accept(TextGridVisitor&& visitor) const = 0;

 protected:
  std::string name_;
  Number min_time_;
  Number max_time_;
};

class PointTier : public Tier {
 public:
  PointTier(const std::string& name, Number min_time, Number max_time)
      : Tier(name, min_time, max_time), points_() {}
  PointTier(const std::string& name, Number min_time, Number max_time,
            size_t number_of_points_to_reserve)
      : Tier(name, min_time, max_time), points_() {
    points_.reserve(number_of_points_to_reserve);
  }
  ~PointTier() noexcept override = default;

  void AppendPoint(const Point& point) { points_.push_back(point); }
  void AppendPoint(Point&& point) { points_.push_back(std::move(point)); }

  size_t GetNumberOfPoints() const noexcept { return points_.size(); }
  size_t GetNumberOfAnnotations() const noexcept override { return GetNumberOfPoints(); }
  const Point& GetPoint(size_t index) const& { return points_[index]; }
  Point GetPoint(size_t index) && { return std::move(points_[index]); }
  const std::vector<Point>& GetAllPoints() const& { return points_; }
  std::vector<Point> GetAllPoints() && { return std::move(points_); }

  void Accept(TextGridVisitor& visitor) const override { visitor.Visit(*this); }
  void Accept(TextGridVisitor&& visitor) const override { visitor.Visit(*this); }

 private:
  std::vector<Point> points_;
};

class IntervalTier : public Tier {
 public:
  IntervalTier(const std::string& name, Number min_time, Number max_time,
               const std::string& text = "")
      : Tier(name, min_time, max_time), intervals_() {
    // intervals_.push_back(Interval(min_time, max_time, text));
  }
  IntervalTier(const std::string& name, Number min_time, Number max_time,
               size_t number_of_intervals_to_reserve)
      : Tier(name, min_time, max_time), intervals_() {
    intervals_.reserve(number_of_intervals_to_reserve);
  }
  ~IntervalTier() noexcept override = default;

  void AppendInterval(const Interval& interval) { intervals_.push_back(interval); }
  void AppendInterval(Interval&& interval) { intervals_.push_back(std::move(interval)); }

  size_t GetNumberOfIntervals() const noexcept { return intervals_.size(); }
  size_t GetNumberOfAnnotations() const noexcept override { return GetNumberOfIntervals(); }
  const Interval& GetInterval(size_t index) const& { return intervals_[index]; }
  Interval GetInterval(size_t index) && { return std::move(intervals_[index]); }
  const std::vector<Interval>& GetAllIntervals() const& { return intervals_; }
  std::vector<Interval> GetAllIntervals() && { return intervals_; }

  void Accept(TextGridVisitor& visitor) const override { visitor.Visit(*this); }
  void Accept(TextGridVisitor&& visitor) const override { visitor.Visit(*this); }

 private:
  std::vector<Interval> intervals_;
};

class TextGrid {
 public:
  TextGrid() : min_time_(0), max_time_(0), tiers_() {}

  TextGrid(Number min_time, Number max_time, size_t number_of_tiers_to_reserve = 0u)
      : min_time_(min_time), max_time_(max_time), tiers_() {
    tiers_.reserve(number_of_tiers_to_reserve);
  }

  void AppendTier(const std::shared_ptr<Tier>& tier) { tiers_.push_back(tier); }
  void AppendTier(std::shared_ptr<Tier>&& tier) { tiers_.push_back(std::move(tier)); }
  template <typename TierType>
  void AppendTier(const std::string& name = "") {
    AppendTier(std::make_shared<TierType>(name, GetMinTime(), GetMaxTime()));
  }

  Number GetMinTime() const noexcept { return min_time_; }
  void SetMinTime(Number min_time) { min_time_ = min_time; }
  Number GetMaxTime() const noexcept { return max_time_; }
  void SetMaxTime(Number max_time) { max_time_ = max_time; }
  size_t GetNumberOfTiers() const noexcept { return tiers_.size(); }
  std::shared_ptr<Tier> GetTier(size_t index) const { return tiers_[index]; }
  template <typename TierType>
  std::shared_ptr<TierType> GetTierAs(size_t index) const {
    return std::dynamic_pointer_cast<TierType>(GetTier(index));
  }
  std::shared_ptr<Tier> GetTier(const std::string& name) const {
    for (const auto& tier : GetAllTiers()) {
      if (tier->GetName() == name) {
        return tier;
      }
    }
    return nullptr;
  }
  template <typename TierType>
  std::shared_ptr<TierType> GetTierAs(const std::string& name) const {
    return std::dynamic_pointer_cast<TierType>(GetTier(name));
  }
  const std::vector<std::shared_ptr<Tier>>& GetAllTiers() const& { return tiers_; }
  std::vector<std::shared_ptr<Tier>> GetAllTiers() && { return std::move(tiers_); }

  void Accept(TextGridVisitor& visitor) const { visitor.Visit(*this); }
  void Accept(TextGridVisitor&& visitor) const { visitor.Visit(*this); }

 private:
  Number min_time_;
  Number max_time_;
  std::vector<std::shared_ptr<Tier>> tiers_;
};

enum class TokenType {
  END_OF_FILE,
  NUMBER_LITERAL,
  TEXT_LITERAL,
  FLAG_LITERAL,
  COMMENT,
};

inline std::string ToString(TokenType type) {
  switch (type) {
    case TokenType::END_OF_FILE:
      return "end of file";
    case TokenType::NUMBER_LITERAL:
      return "number";
    case TokenType::TEXT_LITERAL:
      return "string";
    case TokenType::FLAG_LITERAL:
      return "flag";
    case TokenType::COMMENT:
      return "comment";
    default:
      return "unknown";
  }
}

struct Token {
  explicit Token(TokenType type, const std::string& value = "") : type(type), value(value) {}

  TokenType type;
  std::string value;
};

inline bool IsEofToken(const Token& token) { return token.type == TokenType::END_OF_FILE; }

inline bool IsNumberToken(const Token& token) { return token.type == TokenType::NUMBER_LITERAL; }

inline bool IsTextToken(const Token& token) { return token.type == TokenType::TEXT_LITERAL; }

inline bool IsFlagToken(const Token& token) { return token.type == TokenType::FLAG_LITERAL; }

inline bool IsCommentToken(const Token& token) { return token.type == TokenType::COMMENT; }

inline std::string EscapeText(const std::string& str) {
  std::string escaped_str;
  escaped_str += '"';
  for (size_t i = 0; i < str.size(); ++i) {
    if (str[i] == '"') {
      escaped_str += str[i];
      escaped_str += str[i];
    } else {
      escaped_str += str[i];
    }
  }
  escaped_str += '"';
  return escaped_str;
}

inline std::string UnescapeText(const std::string& str) {
  std::string unescaped_str;
  for (size_t i = 1; i + 1 < str.size(); ++i) {
    if (str[i] == '"' && str[i + 1] == '"') {
      unescaped_str += str[i++];
    } else {
      unescaped_str += str[i];
    }
  }
  return unescaped_str;
}

inline std::string EscapeFlag(const std::string& str) { return "<" + str + ">"; }

inline std::string UnescapeFlag(const std::string& str) {
  if (str.size() < 2) {
    return "";
  } else {
    return str.substr(1u, str.size() - 2);
  }
}

class Lexer {
 public:
  virtual ~Lexer() noexcept = default;

  virtual Token GetNextToken() = 0;
  virtual int GetLineNumber() = 0;

  virtual Token GetNextTokenExceptComment() {
    Token token = GetNextToken();
    while (IsCommentToken(token)) {
      token = GetNextToken();
    }
    return token;
  }

  virtual std::vector<Token> Tokenize() {
    std::vector<Token> tokens;
    for (Token token = GetNextToken(); !IsEofToken(token); token = GetNextToken()) {
      tokens.push_back(std::move(token));
    }
    return tokens;
  }

  virtual std::vector<Token> TokenizeExceptComment() {
    std::vector<Token> tokens;
    for (Token token = GetNextToken(); !IsEofToken(token); token = GetNextToken()) {
      if (IsCommentToken(token)) {
        continue;
      }
      tokens.push_back(std::move(token));
    }
    return tokens;
  }
};

class Evaluator : public Lexer {
 private:
  static void EvaluateInPlace(Token& token) {
    switch (token.type) {
      case TokenType::END_OF_FILE:
      case TokenType::NUMBER_LITERAL:
      case TokenType::COMMENT:
        // Nothing to do.
        break;
      case TokenType::TEXT_LITERAL:
        token.value = UnescapeText(token.value);
        break;
      case TokenType::FLAG_LITERAL:
        token.value = UnescapeFlag(token.value);
        break;
    }
  }

 public:
  explicit Evaluator(Lexer* lexer) : lexer_(lexer) {}
  explicit Evaluator(std::unique_ptr<Lexer> lexer) : lexer_(std::move(lexer)) {}

  Token GetNextToken() override {
    Token token = lexer_->GetNextToken();
    EvaluateInPlace(token);
    return token;
  }
  int GetLineNumber() override { return lexer_->GetLineNumber(); }

 private:
  std::unique_ptr<Lexer> lexer_;
};

class StreamLexer : public Lexer {
 public:
  static constexpr char EofChar() { return std::istream::traits_type::eof(); }
  static char IsEofChar(char c) { return c == EofChar(); }
  static char IsNewlineChar(char c) { return c == '\r' || c == '\n'; }

 public:
  explicit StreamLexer(std::istream& is) : is_(is), lineno_(1) {}

  Token GetNextToken() override {
    SkipSpaces();
    switch (Peek()) {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        return GetNumberToken();
      case '\"':
        return GetTextToken();
      case '<':
        return GetFlagToken();
      case EofChar():
        return Token(TokenType::END_OF_FILE);
      case '!':
        return GetLineCommentToken();
      default:
        return GetCommentToken();
    }
  }
  int GetLineNumber() override { return lineno_; }

 private:
  char Peek() { return is_.peek(); }
  char GetChar() {
    char c = is_.get();
    if (c == '\n') {
      lineno_++;
    }
    return c;
  }
  void SkipSpaces() {
    while (isspace(Peek())) {
      GetChar();
    }
  }

  Token GetNumberToken() {
    Token token(TokenType::NUMBER_LITERAL);
    while (isdigit(Peek())) {
      token.value += GetChar();
    }
    if (Peek() == '.') {
      token.value += GetChar();
    }
    while (isdigit(Peek())) {
      token.value += GetChar();
    }
    // Check whether it is "free-standing" or not.
    if (!isspace(Peek()) && !IsEofChar(Peek())) {
      const std::string message =
          "Character '" + std::string(1u, Peek()) + "' following number " + token.value;
      throw LexicalError(message, lineno_);
    }
    return token;
  }
  Token GetTextToken() {
    // Note: Currently, the text token in multiple lines is not supported.
    Token token(TokenType::TEXT_LITERAL);
    token.value += GetChar();  // '\"'
    while (!IsNewlineChar(Peek()) && !IsEofChar(Peek())) {
      if (Peek() == '\"') {
        token.value += GetChar();
        if (Peek() == '\"') {
          token.value += GetChar();
        } else {
          break;
        }
      } else {
        token.value += GetChar();
      }
    }
    if (token.value.back() != '\"') {
      throw LexicalError("Early end of text is detected while reading a string", lineno_);
    }
    // Check whether it is "free-standing" or not.
    if (!isspace(Peek()) && !IsEofChar(Peek())) {
      const std::string message =
          "Character '" + std::string(1u, Peek()) + "' following text " + token.value;
      throw LexicalError(message, lineno_);
    }
    return token;
  }
  Token GetFlagToken() {
    Token token(TokenType::FLAG_LITERAL);
    token.value += GetChar();  // '<'
    while (!IsNewlineChar(Peek()) && !IsEofChar(Peek())) {
      if (Peek() == '>') {
        token.value += GetChar();
        break;
      } else {
        token.value += GetChar();
      }
    }
    if (token.value.back() != '>') {
      throw LexicalError("No matching '>' while reading a flag", lineno_);
    }
    // Check whether it is "free-standing" or not.
    if (!isspace(Peek()) && !IsEofChar(Peek())) {
      const std::string message =
          "Character '" + std::string(1u, Peek()) + "' following flag " + token.value;
      throw LexicalError(message, lineno_);
    }
    return token;
  }
  Token GetLineCommentToken() {
    Token token(TokenType::COMMENT);
    while (!IsNewlineChar(Peek()) && !IsEofChar(Peek())) {
      token.value += GetChar();
    }
    return token;
  }
  Token GetCommentToken() {
    Token token(TokenType::COMMENT);
    while (!isspace(Peek()) && !IsEofChar(Peek())) {
      token.value += GetChar();
    }
    return token;
  }

 private:
  std::istream& is_;
  int lineno_;
};

inline std::unique_ptr<Lexer> CreateDefaultLexer(std::istream& is) {
  return std::unique_ptr<Lexer>(new Evaluator(new StreamLexer(is)));
}

class Parser {
 private:
  static bool Expect(const Token& token, TokenType type) { return token.type == type; }
  static bool Expect(const Token& token, TokenType type, const std::string& value) {
    return token.type == type && token.value == value;
  }

 public:
  explicit Parser(std::istream& is) : lexer_(CreateDefaultLexer(is)) {}
  explicit Parser(std::unique_ptr<Lexer> lexer) : lexer_(std::move(lexer)) {}

  TextGrid Parse() {
    ParseHeader();
    return ParseTextGrid();
  }
  void ResetLexer(std::unique_ptr<Lexer> new_lexer) { lexer_ = std::move(new_lexer); }

 private:
  void ParseHeaderFileType() {
    static const std::string kFileType = "ooTextFile";
    Token token = lexer_->GetNextTokenExceptComment();
    if (!Expect(token, TokenType::TEXT_LITERAL, kFileType)) {
      throw SyntaxError("Invalid TextGrid Header. File type must be \"" + kFileType +
                            "\", but it's \"" + token.value + "\"",
                        lexer_->GetLineNumber());
    }
  }
  void ParseHeaderObjectClass() {
    static const std::string kObjectClass = "TextGrid";
    Token token = lexer_->GetNextTokenExceptComment();
    if (!Expect(token, TokenType::TEXT_LITERAL, kObjectClass)) {
      throw SyntaxError("Invalid TextGrid Header. Object class must be \"" + kObjectClass +
                            "\", but it's \"" + token.value + "\"",
                        lexer_->GetLineNumber());
    }
  }
  void ParseHeader() {
    ParseHeaderFileType();
    ParseHeaderObjectClass();
  }

  Number ParseNumber() {
    Token token = lexer_->GetNextTokenExceptComment();
    if (!Expect(token, TokenType::NUMBER_LITERAL)) {
      throw SyntaxError("Found a " + ToString(token.type) + " while looking for a real number",
                        lexer_->GetLineNumber());
    }
    return atof(token.value.c_str());
  }
  std::string ParseText() {
    Token token = lexer_->GetNextTokenExceptComment();
    if (!Expect(token, TokenType::TEXT_LITERAL)) {
      throw SyntaxError("Found a " + ToString(token.type) + " while looking for a text",
                        lexer_->GetLineNumber());
    }
    return token.value;
  }
  std::string ParseFlag() {
    Token token = lexer_->GetNextTokenExceptComment();
    if (!Expect(token, TokenType::FLAG_LITERAL)) {
      throw SyntaxError("Found a " + ToString(token.type) + " while looking for a flag",
                        lexer_->GetLineNumber());
    }
    return token.value;
  }
  void ParseExistsFlag() {
    static const std::string kExistsFlag = "exists";
    const std::string flag = ParseFlag();
    if (flag != kExistsFlag) {
      throw SyntaxError("<" + flag + "> is not valid flag for TextGrid file",
                        lexer_->GetLineNumber());
    }
  }

  Interval ParseInterval() {
    Interval interval;
    interval.min_time = ParseNumber();
    interval.max_time = ParseNumber();
    interval.text = ParseText();
    return interval;
  }
  std::shared_ptr<Tier> ParseIntervalTier() {
    const std::string name = ParseText();
    const Number min_time = ParseNumber();
    const Number max_time = ParseNumber();
    const size_t number_of_intervals = ParseNumber();
    std::shared_ptr<IntervalTier> interval_tier =
        std::make_shared<IntervalTier>(name, min_time, max_time, number_of_intervals);
    for (size_t i = 0; i < number_of_intervals; ++i) {
      interval_tier->AppendInterval(ParseInterval());
    }
    return interval_tier;
  }

  Point ParsePoint() {
    Point point;
    point.time = ParseNumber();
    point.text = ParseText();
    return point;
  }
  std::shared_ptr<Tier> ParsePointTier() {
    const std::string name = ParseText();
    const Number min_time = ParseNumber();
    const Number max_time = ParseNumber();
    const size_t number_of_points = ParseNumber();
    std::shared_ptr<PointTier> point_tier =
        std::make_shared<PointTier>(name, min_time, max_time, number_of_points);
    for (size_t i = 0; i < number_of_points; ++i) {
      point_tier->AppendPoint(ParsePoint());
    }
    return point_tier;
  }
  std::shared_ptr<Tier> ParseTier() {
    static const std::string kIntervalTierClass = "IntervalTier";
    static const std::string kTextTierClass = "TextTier";
    const std::string klass = ParseText();
    if (klass == kIntervalTierClass) {
      return ParseIntervalTier();
    } else if (klass == kTextTierClass) {
      // Note: PointTier is also called TextTier. They are the same.
      return ParsePointTier();
    } else {
      throw SyntaxError("Class \"" + klass + "\" is not valid class name.",
                        lexer_->GetLineNumber());
    }
  }

  TextGrid ParseTextGrid() {
    const Number min_time = ParseNumber();
    const Number max_time = ParseNumber();
    ParseExistsFlag();
    const size_t number_of_tiers = ParseNumber();
    TextGrid grid(min_time, max_time, number_of_tiers);
    for (size_t i = 0; i < number_of_tiers; ++i) {
      grid.AppendTier(ParseTier());
    }
    return grid;
  }

 private:
  std::unique_ptr<Lexer> lexer_;
};

class TextGridInternalVisitor : public TextGridVisitor {
 public:
  enum class TraversalOrder {
    PreOrder,
    PostOrder,
  };

 public:
  explicit TextGridInternalVisitor(TraversalOrder order) : order_(order) {}
  ~TextGridInternalVisitor() noexcept override = default;

  bool PreOrder() const noexcept { return order_ == TraversalOrder::PreOrder; }
  bool PostOrder() const noexcept { return order_ == TraversalOrder::PostOrder; }

  void Visit(const TextGrid& text_grid) final {
    if (PreOrder()) {
      VisitInternally(text_grid);
    }
    for (const auto& tier : text_grid.GetAllTiers()) {
      tier->Accept(*this);
    }
    if (PostOrder()) {
      VisitInternally(text_grid);
    }
  }

  void Visit(const IntervalTier& interval_tier) final {
    if (PreOrder()) {
      VisitInternally(interval_tier);
    }
    for (const auto& interval : interval_tier.GetAllIntervals()) {
      interval.Accept(*this);
    }
    if (PostOrder()) {
      VisitInternally(interval_tier);
    }
  }
  void Visit(const Interval& interval) final { VisitInternally(interval); }

  void Visit(const PointTier& point_tier) final {
    if (PreOrder()) {
      VisitInternally(point_tier);
    }
    for (const auto& point : point_tier.GetAllPoints()) {
      point.Accept(*this);
    }
    if (PostOrder()) {
      VisitInternally(point_tier);
    }
  }
  void Visit(const Point& point) final { VisitInternally(point); }

 protected:
  virtual void VisitInternally(const TextGrid& text_grid) = 0;
  virtual void VisitInternally(const IntervalTier& interval_tier) = 0;
  virtual void VisitInternally(const Interval& interval) = 0;
  virtual void VisitInternally(const PointTier& point_tier) = 0;
  virtual void VisitInternally(const Point& point) = 0;

 private:
  TraversalOrder order_;
};

class ShortWriter : public TextGridInternalVisitor {
 public:
  explicit ShortWriter(std::ostream& os)
      : TextGridInternalVisitor(TraversalOrder::PreOrder), os_(os) {}
  ~ShortWriter() noexcept override = default;

  void VisitInternally(const TextGrid& text_grid) override {
    WriteAttribute("File type", "ooTextFile");
    WriteAttribute("Object class", "TextGrid");
    os_ << std::endl;
    WriteNumber(text_grid.GetMinTime());
    WriteNumber(text_grid.GetMaxTime());
    os_ << "<exists>" << std::endl;
    WriteNumber(text_grid.GetNumberOfTiers());
  }

  void VisitInternally(const IntervalTier& interval_tier) override {
    WriteText("IntervalTier");
    WriteText(interval_tier.GetName());
    WriteNumber(interval_tier.GetMinTime());
    WriteNumber(interval_tier.GetMaxTime());
    WriteNumber(interval_tier.GetNumberOfIntervals());
  }
  void VisitInternally(const Interval& interval) override {
    WriteNumber(interval.min_time);
    WriteNumber(interval.max_time);
    WriteText(interval.text);
  }

  void VisitInternally(const PointTier& point_tier) override {
    WriteText("TextTier");
    WriteText(point_tier.GetName());
    WriteNumber(point_tier.GetMinTime());
    WriteNumber(point_tier.GetMaxTime());
    WriteNumber(point_tier.GetNumberOfPoints());
  }
  void VisitInternally(const Point& point) override {
    WriteNumber(point.time);
    WriteText(point.text);
  }

 private:
  void WriteAttribute(const std::string& name, const std::string& value) {
    os_ << name << " = " << EscapeText(value) << std::endl;
  }
  void WriteNumber(const Number& value) { os_ << value << std::endl; }
  void WriteText(const std::string& value) { os_ << EscapeText(value) << std::endl; }

 private:
  std::ostream& os_;
};

class Writer : public TextGridInternalVisitor {
 public:
  explicit Writer(std::ostream& os)
      : TextGridInternalVisitor(TraversalOrder::PreOrder),
        os_(os),
        tier_index_(0u),
        interval_index_(0u),
        point_index_(0u) {}
  ~Writer() noexcept override = default;

  void VisitInternally(const TextGrid& text_grid) override { WriteTextGrid(text_grid); }

  void VisitInternally(const IntervalTier& interval_tier) override {
    WriteIntervalTier(interval_tier);
    tier_index_++;
    interval_index_ = 0u;
  }
  void VisitInternally(const Interval& interval) override {
    WriteInterval(interval);
    interval_index_++;
  }

  void VisitInternally(const PointTier& point_tier) override {
    WritePointTier(point_tier);
    tier_index_++;
    point_index_ = 0u;
  }
  void VisitInternally(const Point& point) override {
    WritePoint(point);
    point_index_++;
  }

 private:
  void WriteTierHeader() {
    static const std::string kTierHeaderIndent(4, ' ');
    os_ << kTierHeaderIndent << "item [" << (tier_index_ + 1) << "]:" << std::endl;
  }
  void WriteIntervalHeader() {
    static const std::string kIntervalHeaderIndent(8, ' ');
    os_ << kIntervalHeaderIndent << "intervals [" << (interval_index_ + 1) << "]:" << std::endl;
  }
  void WritePointHeader() {
    static const std::string kPointHeaderIndent(8, ' ');
    os_ << kPointHeaderIndent << "points [" << (point_index_ + 1) << "]:" << std::endl;
  }

  void WriteAttribute(const std::string& name, const Number& value, const std::string& indent) {
    os_ << indent << name << " = " << value << std::endl;
  }
  void WriteAttribute(const std::string& name, const std::string& value,
                      const std::string& indent) {
    os_ << indent << name << " = " << EscapeText(value) << std::endl;
  }

  template <typename T>
  void WriteTextGridAttribute(const std::string& name, const T& value) {
    static const std::string kTextGridIndent(0, ' ');
    WriteAttribute(name, value, kTextGridIndent);
  }
  template <typename T>
  void WriteTierAttribute(const std::string& name, const T& value) {
    static const std::string kTierIndent(8, ' ');
    WriteAttribute(name, value, kTierIndent);
  }
  template <typename T>
  void WriteIntervalAttribute(const std::string& name, const T& value) {
    static const std::string kIntervalIndent(12, ' ');
    WriteAttribute(name, value, kIntervalIndent);
  }
  template <typename T>
  void WritePointAttribute(const std::string& name, const T& value) {
    static const std::string kPointIndent(12, ' ');
    WriteAttribute(name, value, kPointIndent);
  }

  void WriteTextGrid(const TextGrid& text_grid) {
    WriteTextGridAttribute("File type", "ooTextFile");
    WriteTextGridAttribute("Object class", "TextGrid");
    os_ << std::endl;
    WriteTextGridAttribute("xmin", text_grid.GetMinTime());
    WriteTextGridAttribute("xmax", text_grid.GetMaxTime());
    os_ << "tiers? <exists>" << std::endl;
    WriteTextGridAttribute("size", text_grid.GetNumberOfTiers());
    os_ << "item []:" << std::endl;
  }

  void WriteInterval(const Interval& interval) {
    WriteIntervalHeader();
    WriteIntervalAttribute("xmin", interval.min_time);
    WriteIntervalAttribute("xmax", interval.max_time);
    WriteIntervalAttribute("text", interval.text);
  }
  void WriteIntervalTier(const IntervalTier& interval_tier) {
    WriteTierHeader();
    WriteTierAttribute("class", "IntervalTier");
    WriteTierAttribute("name", interval_tier.GetName());
    WriteTierAttribute("xmin", interval_tier.GetMinTime());
    WriteTierAttribute("xmax", interval_tier.GetMaxTime());
    WriteTierAttribute("intervals: size", interval_tier.GetNumberOfIntervals());
  }

  void WritePoint(const Point& point) {
    WritePointHeader();
    WritePointAttribute("time", point.time);
    WritePointAttribute("mark", point.text);
  }
  void WritePointTier(const PointTier& point_tier) {
    WriteTierHeader();
    WriteTierAttribute("class", "TextTier");
    WriteTierAttribute("name", point_tier.GetName());
    WriteTierAttribute("xmin", point_tier.GetMinTime());
    WriteTierAttribute("xmax", point_tier.GetMaxTime());
    WriteTierAttribute("points: size", point_tier.GetNumberOfPoints());
  }

 private:
  std::ostream& os_;
  size_t tier_index_;
  size_t interval_index_;
  size_t point_index_;
};

inline std::istream& operator>>(std::istream& is, TextGrid& text_grid) {
  text_grid = Parser(is).Parse();
  return is;
}

inline std::ostream& operator<<(std::ostream& os, const TextGrid& text_grid) {
  text_grid.Accept(Writer(os));
  return os;
}

}  // namespace textgrid

#endif  // TEXTGRID_HPP_
