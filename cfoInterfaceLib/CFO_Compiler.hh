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
		HEARTBEAT = 1,
		DATA_REQUEST = 2,
		SET = 3,
		INC = 4,
		WAIT = 5,
		LOOP = 6,
		DO_LOOP = 7,
		REPEAT = 8,
		END = 9,
		// AND = 5,
		// OR = 6,
		INVALID = 0xFF,
	};

	/// <summary>
	/// Macro names
	/// </summary>
	enum class CFO_MACRO : uint8_t
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
	explicit CFO_Compiler(uint64_t clockSpeed = 40000000)
		: FPGAClock_(1e9/clockSpeed){};
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
	void 				readLine					(std::string& line);
	void 				transcribeInstruction		(void);
	void 				transcribeMacro				(void);
	void 				errorCheck					(CFO_INSTR);
	uint64_t			calcParameter				(CFO_INSTR);
	std::string 		readInstruction				(std::string& line);
	void 				readMacro					(std::string& line);
	void 				feedInstruction				(const std::string& instruction, const std::string& argument, uint64_t parameter, const std::string& identifier);
	void 				macroSetup					(const std::string& instructionBuffer);
	CFO_INSTR 			parse_instruction			(const std::string& instructionBuffer);
	CFO_MACRO 			parse_macro					(const std::string& instructionBuffer);
	void 				outParameter				(uint64_t);
	bool 				isComment					(const std::string& line);
	bool 				isMacro						(void);
	void 				macroErrorCheck				(CFO_MACRO);

	std::stack<uint64_t> loopStack_;
	std::deque<std::string> macroArgument_;
	std::string instructionBuffer_;
	std::string argumentBuffer_;
	std::string identifierBuffer_;
	std::string parameterBufferString_;
	uint64_t parameterBuffer_;
	uint64_t FPGAClock_; //period of FPGA clock in ns
	int macroArgCount_;
	CFO_MACRO macroOpcode_;
	bool macroFlag_;

	size_t txtLineNumber_, binLineNumber_;
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