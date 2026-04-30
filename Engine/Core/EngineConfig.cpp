#include "Core/EngineConfig.h"
#include "Core/Log.h"
#include "Platform/FileSystem.h"
#include "RHIConfig.h"
#include <unordered_map>
#include <variant>
#include <memory>
#include <vector>
#include <cctype>
#include <cstdlib>

namespace Evo {

// ============================================================================
// Minimal JSON value type — just enough to parse engine_config.json.
// Supports: object, string, integer, float, boolean, null.
// ============================================================================

struct JsonValue;

using JsonObject = std::unordered_map<std::string, JsonValue>;

struct JsonValue {
	enum class Kind { Null, String, Integer, Float, Boolean, Object };

	Kind kind = Kind::Null;
	std::string        sValue;
	int64_t             iValue = 0;
	double              fValue = 0.0;
	bool                bValue = false;
	std::unique_ptr<JsonObject> pObject;

	JsonValue() = default;
	explicit JsonValue(std::string s)  : kind(Kind::String),  sValue(std::move(s)) {}
	explicit JsonValue(int64_t i)      : kind(Kind::Integer), iValue(i)               {}
	explicit JsonValue(double f)       : kind(Kind::Float),   fValue(f)               {}
	explicit JsonValue(bool b)         : kind(Kind::Boolean), bValue(b)               {}
	explicit JsonValue(JsonObject obj) : kind(Kind::Object),  pObject(std::make_unique<JsonObject>(std::move(obj))) {}

	bool IsObject()  const { return kind == Kind::Object;  }
	bool IsString()  const { return kind == Kind::String;  }
	bool IsInteger() const { return kind == Kind::Integer; }
	bool IsBoolean() const { return kind == Kind::Boolean; }
};

// ============================================================================
// Tokenizer
// ============================================================================

struct JsonToken {
	enum class Type { LBrace, RBrace, Colon, Comma, String, Integer, Float, True, False, Null, Eof };

	Type type = Type::Eof;
	std::string sValue;
	int64_t      iValue = 0;
	double       fValue = 0.0;
};

class JsonTokenizer {
public:
	explicit JsonTokenizer(const std::string& source)
		: m_Source(source), m_Pos(0)
	{
		// Skip UTF-8 BOM if present
		if (m_Source.size() >= 3 &&
			static_cast<unsigned char>(m_Source[0]) == 0xEF &&
			static_cast<unsigned char>(m_Source[1]) == 0xBB &&
			static_cast<unsigned char>(m_Source[2]) == 0xBF)
		{
			m_Pos = 3;
		}
	}

	std::optional<JsonToken> Next() {
		SkipWhitespace();
		if (m_Pos >= m_Source.size())
			return JsonToken{JsonToken::Type::Eof};

		char c = Peek();

		switch (c) {
		case '{': Advance(); return JsonToken{JsonToken::Type::LBrace};
		case '}': Advance(); return JsonToken{JsonToken::Type::RBrace};
		case ':': Advance(); return JsonToken{JsonToken::Type::Colon};
		case ',': Advance(); return JsonToken{JsonToken::Type::Comma};
		case '"': return ParseString();
		case 't': case 'f': return ParseBoolean();
		case 'n': return ParseNull();
		default:
			if (c == '-' || std::isdigit(static_cast<unsigned char>(c)))
				return ParseNumber();
			SetErrorChar("unexpected character", c);
			return std::nullopt;
		}
	}

	const std::string& GetError() const { return m_Error; }

private:
	void SkipWhitespace() {
		while (m_Pos < m_Source.size()) {
			char c = m_Source[m_Pos];
			if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
				++m_Pos;
			} else if (c == '/' && m_Pos + 1 < m_Source.size()) {
				// Line comment
				if (m_Source[m_Pos + 1] == '/') {
					m_Pos += 2;
					while (m_Pos < m_Source.size() && m_Source[m_Pos] != '\n') ++m_Pos;
				} else {
					break; // not a comment
				}
			} else {
				break;
			}
		}
	}

	std::optional<JsonToken> ParseString() {
		Advance(); // skip opening "
		std::string result;
		while (m_Pos < m_Source.size()) {
			char c = m_Source[m_Pos];
			if (c == '"') {
				Advance(); // skip closing "
				JsonToken tok{JsonToken::Type::String};
				tok.sValue = std::move(result);
				return tok;
			}
			if (c == '\\') {
				++m_Pos;
				if (m_Pos >= m_Source.size()) {
					SetError("unterminated escape sequence in string");
					return std::nullopt;
				}
				switch (m_Source[m_Pos]) {
				case '"':  result += '"';  break;
				case '\\': result += '\\'; break;
				case '/':  result += '/';  break;
				case 'n':  result += '\n'; break;
				case 'r':  result += '\r'; break;
				case 't':  result += '\t'; break;
				default:
					// For simplicity, just append the escaped char
					result += m_Source[m_Pos];
					break;
				}
				++m_Pos;
			} else {
				result += c;
				++m_Pos;
			}
		}
		SetError("unterminated string");
		return std::nullopt;
	}

	std::optional<JsonToken> ParseNumber() {
		size_t start = m_Pos;
		// Optional minus
		if (Peek() == '-') ++m_Pos;
		// Integer digits
		while (m_Pos < m_Source.size() && std::isdigit(static_cast<unsigned char>(m_Source[m_Pos])))
			++m_Pos;
		// Fractional part
		bool isFloat = false;
		if (m_Pos < m_Source.size() && m_Source[m_Pos] == '.') {
			isFloat = true;
			++m_Pos;
			while (m_Pos < m_Source.size() && std::isdigit(static_cast<unsigned char>(m_Source[m_Pos])))
				++m_Pos;
		}
		std::string numStr = m_Source.substr(start, m_Pos - start);

		JsonToken tok;
		if (isFloat) {
			tok.type  = JsonToken::Type::Float;
			tok.fValue = std::strtod(numStr.c_str(), nullptr);
		} else {
			tok.type  = JsonToken::Type::Integer;
			tok.iValue = std::strtoll(numStr.c_str(), nullptr, 10);
		}
		return tok;
	}

	std::optional<JsonToken> ParseBoolean() {
		if (m_Source.substr(m_Pos, 4) == "true") {
			m_Pos += 4;
			return JsonToken{JsonToken::Type::True};
		}
		if (m_Source.substr(m_Pos, 5) == "false") {
			m_Pos += 5;
			return JsonToken{JsonToken::Type::False};
		}
		SetError("invalid token (expected true/false)");
		return std::nullopt;
	}

	std::optional<JsonToken> ParseNull() {
		if (m_Source.substr(m_Pos, 4) == "null") {
			m_Pos += 4;
			return JsonToken{JsonToken::Type::Null};
		}
		SetError("invalid token (expected null)");
		return std::nullopt;
	}

	char Peek() const { return m_Source[m_Pos]; }
	void Advance()   { ++m_Pos; }

	void SetError(const std::string& msg) { m_Error = msg; }
	void SetErrorChar(const std::string& msg, char c) {
		m_Error = msg + " '" + c + "' at position " + std::to_string(m_Pos);
	}

	const std::string& m_Source;
	size_t m_Pos;
	std::string m_Error;
};

// ============================================================================
// Recursive-descent parser
// ============================================================================

class JsonParser {
public:
	explicit JsonParser(JsonTokenizer& tokenizer) : m_Tokenizer(tokenizer) {}

	std::optional<JsonValue> ParseValue(JsonToken& firstToken) {
		switch (firstToken.type) {
		case JsonToken::Type::LBrace: return ParseObject();
		case JsonToken::Type::String: {
			JsonValue val(firstToken.sValue);
			return val;
		}
		case JsonToken::Type::Integer:
			return JsonValue(firstToken.iValue);
		case JsonToken::Type::Float:
			return JsonValue(firstToken.fValue);
		case JsonToken::Type::True:
			return JsonValue(true);
		case JsonToken::Type::False:
			return JsonValue(false);
		case JsonToken::Type::Null:
			return JsonValue();
		default:
			m_Error = "unexpected token";
			return std::nullopt;
		}
	}

	std::optional<JsonValue> ParseObject() {
		JsonObject obj;
		while (true) {
			auto tok = m_Tokenizer.Next();
			if (!tok) {
				m_Error = m_Tokenizer.GetError();
				return std::nullopt;
			}
			// Empty object or end of object
			if (tok->type == JsonToken::Type::RBrace)
				return JsonValue(std::move(obj));
			// Key must be a string
			if (tok->type != JsonToken::Type::String) {
				m_Error = "expected string key in object";
				return std::nullopt;
			}
			std::string key = std::move(tok->sValue);
			// Colon
			tok = m_Tokenizer.Next();
			if (!tok || tok->type != JsonToken::Type::Colon) {
				m_Error = "expected ':' after key '" + key + "'";
				return std::nullopt;
			}
			// Value
			tok = m_Tokenizer.Next();
			if (!tok) {
				m_Error = m_Tokenizer.GetError();
				return std::nullopt;
			}
			auto val = ParseValue(*tok);
			if (!val) {
				m_Error = "failed to parse value for key '" + key + "': " + m_Error;
				return std::nullopt;
			}
			obj[std::move(key)] = std::move(*val);
			// Comma or end of object
			tok = m_Tokenizer.Next();
			if (!tok) {
				m_Error = m_Tokenizer.GetError();
				return std::nullopt;
			}
			if (tok->type == JsonToken::Type::RBrace)
				return JsonValue(std::move(obj));
			if (tok->type != JsonToken::Type::Comma) {
				m_Error = "expected ',' or '}' in object";
				return std::nullopt;
			}
		}
	}

	const std::string& GetError() const { return m_Error; }

private:
	JsonTokenizer& m_Tokenizer;
	std::string m_Error;
};

// ============================================================================
// Config extraction helpers — pull typed values from JsonObject by key.
// ============================================================================

static const JsonValue* FindMember(const JsonObject& obj, const std::string& key) {
	auto it = obj.find(key);
	return (it != obj.end()) ? &it->second : nullptr;
}

static bool GetString(const JsonObject& obj, const std::string& key, std::string& out) {
	auto* p = FindMember(obj, key);
	if (!p || !p->IsString()) return false;
	out = p->sValue;
	return true;
}

static bool GetBool(const JsonObject& obj, const std::string& key, bool& out) {
	auto* p = FindMember(obj, key);
	if (!p || !p->IsBoolean()) return false;
	out = p->bValue;
	return true;
}

static bool GetInt(const JsonObject& obj, const std::string& key, int64_t& out) {
	auto* p = FindMember(obj, key);
	if (!p || !p->IsInteger()) return false;
	out = p->iValue;
	return true;
}

// ============================================================================
// EngineConfig — singleton static members
// ============================================================================

EngineConfig EngineConfig::s_Instance;
bool         EngineConfig::s_bLoaded = false;

EngineConfig& EngineConfig::GetInstance() {
	return s_Instance;
}

bool EngineConfig::IsLoaded() {
	return s_bLoaded;
}

bool EngineConfig::LoadFromFile(const std::filesystem::path& path) {
	auto cfg = Load(path);
	if (!cfg) {
		EVO_LOG_ERROR("EngineConfig: failed to parse '{}', using defaults", path.string());
		cfg = Default();
	}
	s_Instance = std::move(*cfg);
	s_bLoaded = true;
	return true;
}

void EngineConfig::SetDefaults() {
	s_Instance = Default();
	s_bLoaded = true;
}

// ============================================================================
// EngineConfig — private parsing
// ============================================================================

std::optional<EngineConfig> EngineConfig::Load(const std::filesystem::path& path) {
	auto text = FileSystem::ReadText(path);
	if (!text) {
		EVO_LOG_WARN("EngineConfig: file not found at '{}'", path.string());
		return Default();
	}
	return Parse(*text);
}

std::optional<EngineConfig> EngineConfig::Parse(const std::string& json) {
	JsonTokenizer tokenizer(json);
	auto first = tokenizer.Next();
	if (!first) {
		EVO_LOG_ERROR("EngineConfig: tokenizer error: {}", tokenizer.GetError());
		return std::nullopt;
	}
	if (first->type != JsonToken::Type::LBrace) {
		EVO_LOG_ERROR("EngineConfig: expected top-level object, got token type {}", (int)first->type);
		return std::nullopt;
	}

	JsonParser parser(tokenizer);
	auto rootVal = parser.ParseObject();
	if (!rootVal) {
		EVO_LOG_ERROR("EngineConfig: parse error: {}", parser.GetError());
		return std::nullopt;
	}
	if (!rootVal->IsObject()) {
		EVO_LOG_ERROR("EngineConfig: root value is not an object");
		return std::nullopt;
	}

	EngineConfig cfg = Default();

	// --- "rhi" section ---
	auto* pRHI = FindMember(*rootVal->pObject, "rhi");
	if (pRHI && pRHI->IsObject()) {
		GetString(*pRHI->pObject, "backend", cfg.rhi.backend);
		GetBool(*pRHI->pObject, "enable_debug_layer", cfg.rhi.bEnableDebugLayer);
		GetBool(*pRHI->pObject, "enable_gpu_based_validation", cfg.rhi.bEnableGPUBasedValidation);
	}

	// --- "window" section ---
	auto* pWin = FindMember(*rootVal->pObject, "window");
	if (pWin && pWin->IsObject()) {
		GetString(*pWin->pObject, "title", cfg.window.sTitle);
		int64_t iWidth = 0, iHeight = 0;
		if (GetInt(*pWin->pObject, "width", iWidth))   cfg.window.uWidth  = static_cast<uint32>(iWidth);
		if (GetInt(*pWin->pObject, "height", iHeight)) cfg.window.uHeight = static_cast<uint32>(iHeight);
		GetBool(*pWin->pObject, "vsync", cfg.window.bVsync);
	}

	EVO_LOG_INFO("EngineConfig loaded: backend={}, debug_layer={}, gbv={}",
		cfg.rhi.backend, cfg.rhi.bEnableDebugLayer, cfg.rhi.bEnableGPUBasedValidation);

	return cfg;
}

EngineConfig EngineConfig::Default() {
	EngineConfig cfg;
	cfg.rhi.backend                 = "dx12";
	cfg.rhi.bEnableDebugLayer       = true;
	cfg.rhi.bEnableGPUBasedValidation = false;
	cfg.window.sTitle                = "EvoEngine";
	cfg.window.uWidth                = 1280;
	cfg.window.uHeight               = 720;
	cfg.window.bVsync                = true;
	return cfg;
}

// ============================================================================
// EngineConfig — helpers
// ============================================================================

RHIBackendType EngineConfig::ResolveBackend() const {
	if (rhi.backend == "vulkan") {
#if EVO_RHI_VULKAN
		return RHIBackendType::Vulkan;
#else
		EVO_LOG_WARN("Vulkan requested in config but not compiled in, falling back to DX12");
		return RHIBackendType::DX12;
#endif
	}
	if (rhi.backend != "dx12") {
		EVO_LOG_WARN("Unknown backend '{}', falling back to DX12", rhi.backend);
	}
#if !EVO_RHI_DX12 && EVO_RHI_VULKAN
	if (rhi.backend != "vulkan") {
		EVO_LOG_WARN("DX12 requested but not compiled in, falling back to Vulkan");
		return RHIBackendType::Vulkan;
	}
#endif
	return RHIBackendType::DX12;
}

} // namespace Evo

