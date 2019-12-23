#ifndef CFOINTERFACELIB_MU2ECOMPILER_HH
#define CFOINTERFACELIB_MU2ECOMPILER_HH

#include <deque>
#include <vector>
#include <fstream>
#include <memory>
#include <sstream>
#include <stack>
#include <string>
#include <algorithm>
#include <cctype>
#include <locale>

#include "tracemf.h"

/// <summary>
/// The CFO Namespace
/// </summary>
namespace CFOLib {

/// <summary>
/// The CFO Compiler class
/// </summary>
class CFO_Compiler
{
public:
	/*************
     DIRECTIVES
  ************/
	/// <summary>
	/// Instruction name
	/// </summary>
	enum class CFO_INSTR : uint8_t
	{
		NOOP = 0,
		START = 1,
		DATA_REQUEST = 2,
		INC = 3,
		SET = 4,
		AND = 5,
		OR = 6,
		LOOP = 7,
		DO_LOOP = 8,
		REPEAT = 9,
		WAIT = 10,
		END = 11,
		INVALID = 0xFF,
	};

	/// <summary>
	/// Macro names
	/// </summary>
	enum class CFO_MACRO : int
	{
		SLICE = 1,
		NON_MACRO = 0,
	};

	/// <summary>
	/// Number of arguments for each macro
	/// </summary>
	const int CFO_MACRO_SLICE_ARG_COUNT = 6;

public:
	/// <summary>
	/// Instantiate the CFO Compiler class
	/// </summary>
	/// <param name="clockSpeed">Clock to use for calculating delays</param>
	explicit CFO_Compiler(int clockSpeed = 40000000)
		: FPGAClock_(clockSpeed){};
	/// <summary>
	/// Default destructor
	/// </summary>
	virtual ~CFO_Compiler() = default;

	/// <summary>
	/// Process an input file and create a byte block for sending to the CFO
	/// </summary>
	/// <param name="lines">Lines from input file</param>
	/// <returns>Array of bytes</returns>
	std::deque<char> processFile(std::vector<std::string> lines);

private:
	// For changing/adding new instructions check README file that comes with this source.
	/***********************
   ** Function Prototypes
   **********************/
	void readLine(std::string line);
	void transcribeIns();
	void transcribeMacro();
	void errorCheck(CFO_INSTR);
	int64_t calcParameter(CFO_INSTR);
	std::string readInstruction(std::string& line);
	void readMacro(std::string& line);
	void feedInstruction(std::string, std::string, int64_t, std::string);
	void macroSetup(std::string);
	CFO_INSTR parse_instruction(std::string);
	CFO_MACRO parse_macro(std::string);
	void outParameter(int64_t);
	bool isComment(std::string line);
	bool isMacro();
	void macroErrorCheck(CFO_MACRO);

	std::stack<int64_t> loopStack_;
	std::deque<std::string> macroArgument_;
	std::string instructionBuffer_;
	std::string argumentBuffer_;
	std::string identifierBuffer_;
	int64_t parameterBuffer_;
	int64_t FPGAClock_;
	int macroArgCount_;
	CFO_MACRO macroOpcode_;
	bool macroFlag_;

	size_t lineNumber_;
	std::deque<char> output_;

	//https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
	// trim from start (in place)
	static inline void ltrim(std::string& s)
	{
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
					return !std::isspace(ch);
				}));
	}

	// trim from end (in place)
	static inline void rtrim(std::string& s)
	{
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
					return !std::isspace(ch);
				})
					.base(),
				s.end());
	}

	// trim from both ends (copying)
	static inline std::string trim(std::string s)
	{
		ltrim(s);
		rtrim(s);
		return s;
	}
};

}  // namespace CFOLib

#endif  // CFOINTERFACELIB_MU2ECOMPILER_HH