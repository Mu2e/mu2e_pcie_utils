#include "cfoInterfaceLib/CFO_Compiler.hh"

#include <iostream>

#define TRACE_NAME "CFO_Compiler"

#define __FILENAME__ 		(__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#define __MF_SUBJECT__ 		__FILENAME__
#define __MF_DECOR__		(__MF_SUBJECT__)
#define __SHORTFILE__ 		(__builtin_strstr(&__FILE__[0], "/srcs/") ? __builtin_strstr(&__FILE__[0], "/srcs/") + 6 : __FILE__)
#define __COUT_HDR_L__ 		"[" << std::dec        << __LINE__ << "]\t"
#define __COUT_HDR_FL__ 	__SHORTFILE__ << " "   << __COUT_HDR_L__
#define __COUT_ERR__ 		TLOG(TLVL_ERROR) 
#define __cout__			std::cout << __MF_DECOR__ << __COUT_HDR_FL__

#define __SS__            	std::stringstream ss; ss << ":" << __MF_DECOR__ << ":" << __COUT_HDR_FL__
#define __SS_THROW__        { __COUT_ERR__ << "\n" << ss.str(); throw std::runtime_error(ss.str()); } //put in {}'s to prevent surprises, e.g. if ... else __SS_THROW__;
#define __E__ 				std::endl


std::deque<char> CFOLib::CFO_Compiler::processFile(std::vector<std::string> lines)
{
	TLOG(TLVL_INFO) << "CFO_Compiler::processFile BEGIN";
	lineNumber_ = 0;
	output_.clear();

	while (lineNumber_ < lines.size())
	{
		__cout__ << "Line number " << lineNumber_ << ": " << lines[lineNumber_] << __E__;
		auto line = trim(lines[lineNumber_]);
		if (!isComment(line))
		{
			readLine(line);  // Reading line
			__cout__ << "Instruction buffer: " << instructionBuffer_ << __E__;

			// Transcription into output file.
			if (!macroFlag_)
				transcribeInstruction();
			else
				transcribeMacro();
		}
		lineNumber_++;
	}

	TLOG(TLVL_INFO) << "CFO_Compiler::processFile Parsing Complete!";
	return output_;
}

/**********************************************
 *Read/Input Functions
 *********************************************/
void CFOLib::CFO_Compiler::readLine(std::string line)  // Reads one line and stores into instruction, parameter, argument and identifier
													   // Buffers.
{
	__cout__ << "Reading line instruction" << __E__;
	instructionBuffer_ = readInstruction(line);

	macroFlag_ = isMacro();
	__cout__ << "Is macro: " << std::boolalpha << macroFlag_ << __E__;

	parameterBufferString_.clear();

	if (!(isComment(line)) && !(macroFlag_))
	{
		if (std::isdigit(line[0]))  //
		{
			__cout__ << "Detected parameter, reading" << __E__;
			argumentBuffer_.clear();
			parameterBufferString_ = readInstruction(line);
			parameterBuffer_ = atoi(parameterBufferString_.c_str());
		}
		else
		{
			__cout__ << "Detected argument, reading argument and parameter" << __E__;
			argumentBuffer_ = readInstruction(line);
			parameterBufferString_ = readInstruction(line);
			parameterBuffer_ = atoi(parameterBufferString_.c_str());
		}

		if (!(isComment(line)))
		{
			__cout__ << "Reading identifier"  << __E__;
			identifierBuffer_ = readInstruction(line);
		}
		else
		{
			identifierBuffer_.clear();
		}
	}
	else if (macroFlag_)
	{
		__cout__ << "Reading macro"  << __E__;
		argumentBuffer_.clear();
		parameterBuffer_ = 0;
		identifierBuffer_.clear();

		readMacro(line);
	}
	else
	{
		__cout__ << "Comment or blank line detected, not parsing!"  << __E__;
		argumentBuffer_.clear();
		parameterBuffer_ = 0;
		identifierBuffer_.clear();
	}

	__cout__ << "Instruction: " << instructionBuffer_ << 
		" Parameter: " << parameterBuffer_  << __E__;
}

// Basic Read Algorithms
//************************
std::string
CFOLib::CFO_Compiler::readInstruction(std::string& line)  // reads on space separated word (with an exception for DATA REQ and DO LOOP)
{
	TLOG(TLVL_TRACE) << "readInstruction: Searching " << line << " for first = or space";
	std::string insString;

	auto breakPos = line.find_first_of(" =");
	if (breakPos != std::string::npos)
	{
		insString = line.substr(0, breakPos);
		line.erase(0, breakPos + 1);
	}
	else
	{
		insString = line;
		line = "";
	}

	insString = trim(insString);
	line = trim(line);

	TLOG(TLVL_TRACE) << "readInstruction: Returning instruction " << insString << ", remaining line: " << line;
	return insString;
}

// macro artificial instruction forming.
//*****************************
void CFOLib::CFO_Compiler::feedInstruction(std::string instruction, std::string argument, int64_t parameter,
										   std::string identifier)
{
	instructionBuffer_ = instruction;
	argumentBuffer_ = argument;
	parameterBuffer_ = parameter;
	identifierBuffer_ = identifier;
}

// Macro Argument Reading
void CFOLib::CFO_Compiler::readMacro(std::string& line)
{
	macroSetup(instructionBuffer_);

	for (int i = 0; i < macroArgCount_; i++)
	{
		macroArgument_.push_back(readInstruction(line));
	}
}

/**********************************************
                          Internal Functions
 *********************************************/
/**************** AUXILIARY ******************/
// Boolean Operators
bool CFOLib::CFO_Compiler::isComment(std::string line)  // Checks if line has a comment.
{
	if (line.size() == 0) return true;
	if (line[0] == '/')
	{
		if (line[1] != '/')
		{
			__SS__ << "Error: Missing slash for comment. (Line " << lineNumber_ << ")" << __E__;
			__SS_THROW__;
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool CFOLib::CFO_Compiler::isMacro()
{
	macroOpcode_ = parse_macro(instructionBuffer_);
	if (macroOpcode_ == CFO_MACRO::NON_MACRO)
	{
		return false;
	}
	else
	{
		return true;
	}
}

// Switch Conversions
CFOLib::CFO_Compiler::CFO_INSTR CFOLib::CFO_Compiler::parse_instruction(
	std::string instructionBuffer)  // Turns strings into integers for switch statements.
{
	if (instructionBuffer == "HEARTBEAT")
	{
		return CFO_INSTR::HEARTBEAT;
	}
	else if (instructionBuffer == "DATA_REQUEST")
	{
		return CFO_INSTR::DATA_REQUEST;
	}
	else if (instructionBuffer == "INC")
	{
		return CFO_INSTR::INC;
	}
	else if (instructionBuffer == "SET")
	{
		return CFO_INSTR::SET;
	}
	// else if (instructionBuffer == "AND")
	// {
	// 	return CFO_INSTR::AND;
	// }
	// else if (instructionBuffer == "OR")
	// {
	// 	return CFO_INSTR::OR;
	// }
	else if (instructionBuffer == "LOOP")
	{
		return CFO_INSTR::LOOP;
	}
	else if (instructionBuffer == "DO_LOOP")
	{
		return CFO_INSTR::DO_LOOP;
	}
	else if (instructionBuffer == "REPEAT")
	{
		return CFO_INSTR::REPEAT;
	}
	else if (instructionBuffer == "WAIT")
	{
		return CFO_INSTR::WAIT;
	}
	else if (instructionBuffer == "END")
	{
		return CFO_INSTR::END;
	}
	return CFO_INSTR::INVALID;
}

// Macro Switch
CFOLib::CFO_Compiler::CFO_MACRO CFOLib::CFO_Compiler::parse_macro(
	std::string instructionBuffer)  // Turns strings into integers for switch statements.
{
	if (instructionBuffer == "SLICE")
	{
		return CFO_MACRO::SLICE;
	}
	return CFO_MACRO::NON_MACRO;
}

// Macro Argument and Instruction Counts
void CFOLib::CFO_Compiler::macroSetup(std::string instructionBuffer)
{
	if (instructionBuffer == "SLICE")
	{
		macroArgCount_ = CFO_MACRO_SLICE_ARG_COUNT;
	}
	else
	{
		macroArgCount_ = 0;
	}
}

// Error Checking
void CFOLib::CFO_Compiler::errorCheck(CFO_INSTR instructionOpcode)  // Checks if there are any misplaced arguments
{
	switch (instructionOpcode)
	{
		case CFO_INSTR::HEARTBEAT:
			if (parameterBuffer_ == 0 && parameterBufferString_ != "0")
			{
				__SS__ << "ERROR: On Line " << lineNumber_ << ". HEARTBEAT has a null or invalid argument (HEARTBEAT 0 Event)"
						  << std::endl;
				__SS_THROW__;
			}
			if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_mode")
			{
				__SS__ << "ERROR: On Line" << lineNumber_ << ". did you mean \"HEARTBEAT event_mode=\"?" << std::endl;
				__SS_THROW__;
			}
			break;

		case CFO_INSTR::DATA_REQUEST:
			if (parameterBuffer_ == 0 && argumentBuffer_ != "CURRENT")
			{
				__SS__ << "ERROR: On Line " << lineNumber_ << ". DATA REQ has a null  or invalid argument (DATA REQ 0 Event)"
						  << std::endl;
				__SS_THROW__;
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "request_tag" && argumentBuffer_ != "CURRENT")
			{
				__SS__ << "ERROR: On Line" << lineNumber_
						  << ". did you mean \"DATA_REQUEST request_tag=\" or DATA_REQUEST CURRENT?" << std::endl;
				__SS_THROW__;
			}
			break;

		case CFO_INSTR::INC:
			if (identifierBuffer_.empty() == false)
			{
				__SS__ << "ERROR: On Line " << lineNumber_ << ". INC has an extraneous identifier" << std::endl;
				__SS_THROW__;
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_by" && argumentBuffer_ != "event_tag")
			{
				__SS__ << "ERROR: On Line" << lineNumber_ << ". did you mean \"INC event_by=\" or \"INC event_tag\?"
						  << std::endl;
				__SS_THROW__;
			}
			break;
		case CFO_INSTR::SET:
			if (parameterBuffer_ == 0)
			{
				__SS__ << "\nERROR: On Line " << lineNumber_ << ". SET has a null  or invalid argument (SET 0 Event)"
						  << std::endl;
				__SS_THROW__;
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_tag")
			{
				__SS__ << "ERROR: On Line" << lineNumber_ << ". did you mean \"SET event_tag=\"?" << std::endl;
				__SS_THROW__;
			}
			break;
		// case CFO_INSTR::AND:
		// 	if (parameterBuffer_ == 0)
		// 	{
		// 		__SS__ << "\nERROR: On Line " << lineNumber_ << ". AND has a null  or invalid argument (AND 0 Event)"
		// 				  << std::endl;
		// 		throw std::exception();
		// 	}
		// 	else if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_tag")
		// 	{
		// 		__SS__ << "ERROR: On Line" << lineNumber_ << ". did you mean \"AND event_tag=\"?" << std::endl;
		// 		throw std::exception();
		// 	}
		// 	break;
		// case CFO_INSTR::OR:
		// 	if (parameterBuffer_ == 0)
		// 	{
		// 		__SS__ << "\nERROR: On Line " << lineNumber_ << ". OR has a null  or invalid argument (OR 0 Event)"
		// 				  << std::endl;
		// 		throw std::exception();
		// 	}
		// 	else if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_tag")
		// 	{
		// 		__SS__ << "ERROR: On Line" << lineNumber_ << ". did you mean \"OR event_tag=\"?" << std::endl;
		// 		throw std::exception();
		// 	}
		// 	break;
		case CFO_INSTR::LOOP:
			if (parameterBuffer_ == 0)
			{
				__SS__ << "ERROR: On Line " << lineNumber_ << ". LOOP has a null  or invalid argument (Looping 0 times)"
						  << std::endl;
				__SS_THROW__;
			}
			else if (identifierBuffer_.empty() == 0 && identifierBuffer_ != "times")
			{
				__SS__ << "ERROR: On Line " << std::to_string(lineNumber_) << ". LOOP has an invalid identifier ("
						  << identifierBuffer_ << ")" << std::endl;
				__SS_THROW__;
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "count")
			{
				__SS__ << "ERROR: On Line" << lineNumber_ << ". did you mean \"LOOP count=\"?" << std::endl;
				__SS_THROW__;
			}
			break;
		case CFO_INSTR::DO_LOOP:
			if (parameterBuffer_ != 0)
			{
				__SS__ << "WARNING: On Line " << std::dec << lineNumber_ << ". DO LOOP has an extraneous argument("
						  << parameterBuffer_ << ")" << std::endl;
				__SS_THROW__;
			}
			break;
		case CFO_INSTR::WAIT:
			if (identifierBuffer_.empty() == 0 && identifierBuffer_ != "cycles" && identifierBuffer_ != "ns" &&
				identifierBuffer_ != "sec" && identifierBuffer_ != "ms")
			{
				__SS__ << "WARNING: On Line " << std::dec << lineNumber_ << ". WAIT has an invalid identifier("
						  << identifierBuffer_ << ")" << std::endl <<
						 "Options for third argument are [empty] ns ms sec" << std::endl;
				__SS_THROW__;
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "period" && argumentBuffer_ != "NEXT")
			{
				__SS__ << "ERROR: On Line" << lineNumber_ << ". did you mean \"WAIT period=\"?" << std::endl;
				__SS_THROW__;
			}

			break;
		case CFO_INSTR::REPEAT:
			if (parameterBuffer_ != 0 && identifierBuffer_.empty())
			{
				__SS__ << "WARNING: On Line " << std::dec << lineNumber_ << ". REPEAT has extraneous arguments("
						  << parameterBuffer_ << "_" << identifierBuffer_ << ")" << std::endl;
				__SS_THROW__;
			}
			break;
		case CFO_INSTR::END:
			if (parameterBuffer_ != 0 && identifierBuffer_.empty())
			{
				__SS__ << "WARNING: On Line " << std::dec << lineNumber_ << ". END has extraneous arguments("
						  << parameterBuffer_ << "_" << identifierBuffer_ << ")" << std::endl;
				__SS_THROW__;
			}
			break;
		default:
			{
				__SS__ << "Invalid opcode: " << (uint8_t)instructionOpcode << __E__;
				__SS_THROW__;
			}
			break;
	}
}

void CFOLib::CFO_Compiler::macroErrorCheck(CFO_MACRO macroInt)
{
	switch (macroInt)
	{
		case CFO_MACRO::SLICE:
			if (macroArgument_[0] != "bitposition")
			{
				__SS__<< "ERROR: On Line (" << lineNumber_ << ") " << 
					"1st parameter should be bitposition=" << __E__;
				__SS_THROW__;
			}
			else if ((atoi(macroArgument_[1].c_str()) > 48) || (atoi(macroArgument_[1].c_str()) < 1))
			{
				__SS__ << "ERROR: On Line (" << lineNumber_ << ") " << 
					"2nd parameter, integer must be between 1 and 48" << __E__;
				__SS_THROW__;
			}
			else if (macroArgument_[2] != "bitwidth")
			{
				__SS__ << "ERROR: On Line (" << lineNumber_ << ") " << 
					"3rd parameter should be bitwidth=" << __E__;
				__SS_THROW__;
			}
			else if ((atoi(macroArgument_[3].c_str()) + atoi(macroArgument_[1].c_str()) - 1 > 48) ||
					 (atoi(macroArgument_[3].c_str()) < 1))
			{
				__SS__ << "ERROR: On Line (" << lineNumber_ << ") " <<
					"4th parameter, integer must be between 1 and must add with bitposition to less than 48" << __E__;
				__SS_THROW__;
			}
			else if (macroArgument_[4] != "event_tag")
			{
				__SS__ << "ERROR: On Line (" << lineNumber_ << ") " << 
					"5th parameter should be event_tag=" << __E__;
				__SS_THROW__;
			}
			else if ((atoi(macroArgument_[5].c_str()) > 1536))
			{
				__SS__ << "ERROR: On Line (" << lineNumber_ << ") " <<
					"6th and last parameter, integer must be between 1 and 1536" << __E__;
				__SS_THROW__;
			}
			break;
		default:
			break;
	}
}

/****************TRANSCRIPTION********************/
// Transcription
void CFOLib::CFO_Compiler::transcribeInstruction()  // Outputs a byte stream based on the buffers.
{
	CFO_INSTR instructionOpcode = parse_instruction(instructionBuffer_);

	if (instructionOpcode == CFO_INSTR::INVALID)
	{
		__SS__ << "ERROR: INVALID INSTRUCTION: " << instructionBuffer_ << std::endl;
		__SS_THROW__;
	}

	int64_t parameterCalc = calcParameter(instructionOpcode);

	errorCheck(instructionOpcode);

	switch (instructionOpcode)
	{
			// Instructions with value;
		case CFO_INSTR::HEARTBEAT:
			outParameter(parameterBuffer_);
			break;

		case CFO_INSTR::DATA_REQUEST:
			if (argumentBuffer_ != "CURRENT")
				outParameter(parameterBuffer_);
			else
				outParameter(0xFFFFFFFFFFFF);
			break;

		case CFO_INSTR::INC:
			outParameter(parameterCalc);
			break;

		case CFO_INSTR::SET:
			outParameter(parameterBuffer_);
			break;

		// case CFO_INSTR::AND:
		// 	outParameter(parameterBuffer_);
		// 	break;

		// case CFO_INSTR::OR:
		// 	outParameter(parameterBuffer_);
		// 	break;

		case CFO_INSTR::LOOP:
			loopStack_.push(lineNumber_);
			outParameter(parameterBuffer_);
			break;

		case CFO_INSTR::DO_LOOP:
			outParameter(parameterCalc);
			break;

		case CFO_INSTR::REPEAT:
			outParameter(0);
			break;

		case CFO_INSTR::WAIT:
			outParameter(parameterCalc);
			break;

		case CFO_INSTR::END:
			outParameter(0);
			break;

		default:
			{
				__SS__ << "ERROR: INVALID INSTRUCTION Opcode: " << (uint8_t)instructionOpcode << " from " <<
					instructionBuffer_ << std::endl;
				__SS_THROW__;
			}
	}

	output_.push_back(0x00);
	output_.push_back(static_cast<char>(instructionOpcode));
}

void CFOLib::CFO_Compiler::transcribeMacro()
{
	switch (macroOpcode_)
	{
		case CFO_MACRO::SLICE:
		{
			// Arg 1: bitposition=
			// Arg 2: value
			// Arg 3: bitwidth=
			// Arg 4: value
			// Arg 5: event_tag=
			// Arg 6: value

			// Checking Errors in parameters
			macroErrorCheck(macroOpcode_);

			// Calculating Macro Parameters
			int64_t parameterAND;
			int64_t parameterOR;
			int bitbeginning = atoi(macroArgument_[1].c_str());  // cant be more than 48
			int bitwidth = atoi(macroArgument_[3].c_str());      // cant be more than (48 - bitbeginning)
			int64_t event_tag = atoi(macroArgument_[5].c_str());
			int64_t exponentHelper = (int64_t)(0x0000FFFFFFFFFFFF >> (48 - bitwidth));

			parameterAND = ~(exponentHelper << (bitbeginning - 1));
			parameterOR = event_tag << (bitbeginning - 1);

			// Feeding instructions
			feedInstruction("AND", "", parameterAND, "");
			transcribeInstruction();
			feedInstruction("OR", "", parameterOR, "");
			transcribeInstruction();
			break;
		}
		default:
			break;
	}
	// Clearing Macro Deque
	macroArgument_.clear();
}

// Parameter Calculations
int64_t CFOLib::CFO_Compiler::calcParameter(CFO_INSTR instructionOpcode)  // Calculates the parameter for the
																		  // instruction (if needed)
{
	int64_t loopLine;
	switch (instructionOpcode)
	{
		case CFO_INSTR::INC:
			if (parameterBuffer_ == 0 or argumentBuffer_ == "event_tag")
			{
				return 1;
			}
			else
			{
				return parameterBuffer_;
			}
			break;

		case CFO_INSTR::WAIT:
			if (parameterBuffer_ == 0 && argumentBuffer_ != "NEXT")
			{
				return 1;
			}
			else if (parameterBuffer_ == 0 && argumentBuffer_ == "NEXT")
			{
				return 0xFFFFFFFFFFFF;
			}
			else if (identifierBuffer_ == "sec")  // Wait wanted in seconds
			{
				int64_t paramTemp_sec = FPGAClock_ * parameterBuffer_;
				return paramTemp_sec;
			}
			else if (identifierBuffer_ == "ms")  // Wait wanted in milliseconds
			{
				int64_t paramTemp_ms = FPGAClock_ * parameterBuffer_ / 1000;
				return paramTemp_ms;
			}
			else if (identifierBuffer_ == "us")  // Wait wanted in microseconds
			{
				int64_t paramTemp_us = FPGAClock_ * parameterBuffer_ / 1000000;
				return paramTemp_us;
			}
			else if (identifierBuffer_ == "ns")  // Wait wanted in nanoseconds
			{
				int64_t paramTemp_ns = FPGAClock_ * parameterBuffer_ / 1000000000;

				if ((parameterBuffer_ % (paramTemp_ns) != 0))
				{
					__COUT_ERR__ << "WARNING: FPGA can only wait in units of 25ns ()" << std::endl;
				}
				return paramTemp_ns;
			}
			else
			{
				return parameterBuffer_;
			}
			break;

		case CFO_INSTR::DO_LOOP:
			if (!loopStack_.empty())
			{
				loopLine = loopStack_.top();
				loopStack_.pop();
				return (lineNumber_ - loopLine);
			}
			else
			{
				std::string errorMessage2 = "WARNING: LOOP/DO_LOOP counts don't match. (More DO_LOOP's) ";
				throw std::runtime_error(errorMessage2);
			}
			break;

		case CFO_INSTR::END:
			if (!loopStack_.empty())
			{
				std::string errorMessage3 = "WARNING: LOOP/DO_LOOP counts don't match (Less DO_LOOP's)";
				throw std::runtime_error(errorMessage3);
			}
		default:
			break;
	}
	return 0;
}

/**********************************************
                          Output Functions
 *********************************************/

void CFOLib::CFO_Compiler::outParameter(int64_t paramBuf)  // Writes the 6 byte parameter out based on a given integer.
{
	paramBuf = paramBuf & 0xFFFFFFFFFFFF;  // Enforce 6 bytes
	auto paramBufPtr = reinterpret_cast<int8_t*>(&paramBuf);
	for (int i = 0; i < 6; ++i) //int i = 5; i >= 0; i--)
	{
		output_.push_back(paramBufPtr[i]);
	}
}
