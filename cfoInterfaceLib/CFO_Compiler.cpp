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
#define __COUT_INFO__ 		TLOG(TLVL_INFO) 
#define __COUT__			TLOG(TLVL_DEBUG) //std::cout << __MF_DECOR__ << __COUT_HDR_FL__

#define __SS__            	std::stringstream ss; ss << ":" << __MF_DECOR__ << ":" << __COUT_HDR_FL__
#define __SS_THROW__        { __COUT_ERR__ << "\n" << ss.str(); throw std::runtime_error(ss.str()); } //put in {}'s to prevent surprises, e.g. if ... else __SS_THROW__;
#define __E__ 				std::endl


//========================================================================
std::deque<char> CFOLib::CFO_Compiler::processFile(std::vector<std::string> lines)
{
	__COUT_INFO__ << "CFO_Compiler::processFile BEGIN";
	txtLineNumber_ = 0;
	binLineNumber_ = 0;
	output_.clear();

	while (txtLineNumber_ < lines.size())
	{
		__COUT__ << "Line number " << txtLineNumber_ << ": " << lines[txtLineNumber_] << __E__;
		auto line = trim(lines[txtLineNumber_]);
		if (!isComment(line))
		{
			readLine(line);  // Reading line
			__COUT__ << txtLineNumber_ << ": Instruction buffer: " << instructionBuffer_ << __E__;

			// Transcription into output file.
			if (!macroFlag_)			
				transcribeInstruction();
			else
				transcribeMacro();
		}
		else
			__COUT__ << txtLineNumber_ << ": is comment" << __E__;

		txtLineNumber_++;
	} //end main processing loop

	__COUT_INFO__ << "CFO_Compiler::processFile Parsing Complete!";
	return output_;
} //end processFile()

//========================================================================
//  *Read/Input Functions
void CFOLib::CFO_Compiler::readLine(std::string& line)  // Reads one line and stores into instruction, parameter, argument and identifier
													   // Buffers.
{
	__COUT__ << "Reading line instruction" << __E__;
	instructionBuffer_ = readInstruction(line);

	macroFlag_ = isMacro();
	__COUT__ << "Is macro: " << std::boolalpha << macroFlag_ << __E__;

	parameterBufferString_.clear();

	if (!(isComment(line)) && !(macroFlag_))
	{
		if (std::isdigit(line[0]))  //
		{
			__COUT__ << "Detected parameter, reading" << __E__;
			argumentBuffer_.clear();
			parameterBufferString_ = readInstruction(line);
			parameterBuffer_ = atoi(parameterBufferString_.c_str());
		}
		else
		{
			__COUT__ << "Detected argument, reading argument and parameter" << __E__;
			argumentBuffer_ = readInstruction(line);
			__COUT__ << "argumentBuffer_: " << argumentBuffer_;
			parameterBufferString_ = readInstruction(line);
			__COUT__ << "parameterBufferString_: " << parameterBufferString_;
			parameterBuffer_ = atoi(parameterBufferString_.c_str());
			__COUT__ << "parameterBuffer_: " << parameterBuffer_;
		}

		if (!(isComment(line)))
		{
			__COUT__ << "Reading identifier"  << __E__;
			identifierBuffer_ = readInstruction(line);
			__COUT__ << "identifierBuffer_: " << identifierBuffer_;
		}
		else
		{
			__COUT__ << "Comment, clearing identifier";
			identifierBuffer_.clear();
		}
	}
	else if (macroFlag_)
	{
		__COUT__ << "Reading macro"  << __E__;
		argumentBuffer_.clear();
		parameterBuffer_ = 0;
		identifierBuffer_.clear();

		readMacro(line);
	}
	else
	{
		__COUT__ << "Comment or blank line detected, not parsing!"  << __E__;
		argumentBuffer_.clear();
		parameterBuffer_ = 0;
		identifierBuffer_.clear();
	}

	__COUT__ << "Instruction: " << instructionBuffer_ << 
		" Parameter: '" << argumentBuffer_ << "' = " << parameterBuffer_  <<
		" " << identifierBuffer_ << __E__;
} //end readLine()

//========================================================================
// Basic Read Algorithms
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
} //end readInstruction()

//========================================================================
// macro artificial instruction forming.
void CFOLib::CFO_Compiler::feedInstruction(const std::string& instruction, 
											const std::string& argument, 
											uint64_t parameter,
										   	const std::string& identifier)
{
	instructionBuffer_ = instruction;
	argumentBuffer_ = argument;
	parameterBuffer_ = parameter;
	identifierBuffer_ = identifier;
} //end feedInstruction()

//========================================================================
// Macro Argument Reading
void CFOLib::CFO_Compiler::readMacro(std::string& line)
{
	macroSetup(instructionBuffer_);

	for (int i = 0; i < macroArgCount_; i++)
		macroArgument_.push_back(readInstruction(line));
} //end readMacro()

//========================================================================
// Boolean Operators
bool CFOLib::CFO_Compiler::isComment(const std::string& line)  // Checks if line has a comment.
{
	if (line.size() == 0) return true;

	if (line[0] == '/')
	{
		if (line[1] != '/')
		{
			__SS__ << "Singled slash found; missing double slash for comment. (Line " << txtLineNumber_ << ")" << __E__;
			__SS_THROW__;
		}
		return true;
	}
		
	return false;
} // end isComment()

//========================================================================
bool CFOLib::CFO_Compiler::isMacro()
{
	macroOpcode_ = parse_macro(instructionBuffer_);
	if (macroOpcode_ == CFO_MACRO::NON_MACRO)
		return false;

	return true;
} //end isMacro()

//========================================================================
// Switch Conversions
// Turns strings into integers for switch statements.
CFOLib::CFO_Compiler::CFO_INSTR CFOLib::CFO_Compiler::parse_instruction(const std::string& instructionBuffer)
{
	if (instructionBuffer == "HEARTBEAT")
		return CFO_INSTR::HEARTBEAT;
	else if (instructionBuffer == "DATA_REQUEST")
		return CFO_INSTR::DATA_REQUEST;
	else if (instructionBuffer == "INC")
		return CFO_INSTR::INC;
	else if (instructionBuffer == "SET")
		return CFO_INSTR::SET;
	// else if (instructionBuffer == "AND")
	// {
	// 	return CFO_INSTR::AND;
	// }
	// else if (instructionBuffer == "OR")
	// {
	// 	return CFO_INSTR::OR;
	// }
	else if (instructionBuffer == "LOOP")
		return CFO_INSTR::LOOP;
	else if (instructionBuffer == "DO_LOOP")
		return CFO_INSTR::DO_LOOP;
	else if (instructionBuffer == "REPEAT")
		return CFO_INSTR::REPEAT;
	else if (instructionBuffer == "WAIT")
		return CFO_INSTR::WAIT;
	else if (instructionBuffer == "END")
		return CFO_INSTR::END;
	return CFO_INSTR::INVALID;
} //end parse_instruction()

//========================================================================
// Macro Switch
// Turns strings into integers for switch statements.
CFOLib::CFO_Compiler::CFO_MACRO CFOLib::CFO_Compiler::parse_macro(const std::string& instructionBuffer) 
{
	if (instructionBuffer == "SLICE")
		return CFO_MACRO::SLICE;
	return CFO_MACRO::NON_MACRO;
} //end parse_macro()

//========================================================================
// Macro Argument and Instruction Counts
void CFOLib::CFO_Compiler::macroSetup(const std::string& instructionBuffer)
{
	if (instructionBuffer == "SLICE")
	{
		macroArgCount_ = CFO_MACRO_SLICE_ARG_COUNT;
	}
	else
	{
		macroArgCount_ = 0;
	}
} //end macroSetup()

//========================================================================
// Error Checking
void CFOLib::CFO_Compiler::errorCheck(CFO_INSTR instructionOpcode)  // Checks if there are any misplaced arguments
{
	switch (instructionOpcode)
	{
		case CFO_INSTR::HEARTBEAT:
			if (parameterBuffer_ == 0 && parameterBufferString_ != "0")
			{
				__SS__ << "On Line " << txtLineNumber_ << ". HEARTBEAT has a null or invalid argument (HEARTBEAT 0 Event)"
						  << std::endl;
				__SS_THROW__;
			}
			if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_mode")
			{
				__SS__ << "On Line" << txtLineNumber_ << ". did you mean \"HEARTBEAT event_mode=\"?" << std::endl;
				__SS_THROW__;
			}
			break;

		case CFO_INSTR::DATA_REQUEST:
			if (parameterBuffer_ == 0 && argumentBuffer_ != "CURRENT")
			{
				__SS__ << "On Line " << txtLineNumber_ << ". DATA REQ has a null  or invalid argument (DATA REQ 0 Event)"
						  << std::endl;
				__SS_THROW__;
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "request_tag" && argumentBuffer_ != "CURRENT")
			{
				__SS__ << "On Line" << txtLineNumber_
						  << ". did you mean \"DATA_REQUEST request_tag=\" or DATA_REQUEST CURRENT?" << std::endl;
				__SS_THROW__;
			}
			break;

		case CFO_INSTR::INC:
			if (identifierBuffer_.empty() == false)
			{
				__SS__ << "On Line " << txtLineNumber_ << ". INC has an extraneous identifier" << std::endl;
				__SS_THROW__;
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_by" && argumentBuffer_ != "event_tag")
			{
				__SS__ << "On Line" << txtLineNumber_ << ". did you mean \"INC event_by=\" or \"INC event_tag\?"
						  << std::endl;
				__SS_THROW__;
			}
			break;
		case CFO_INSTR::SET:
			if (parameterBuffer_ == 0)
			{
				__SS__ << "On Line " << txtLineNumber_ << ". SET has a null  or invalid argument (SET 0 Event)"
						  << std::endl;
				__SS_THROW__;
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_tag")
			{
				__SS__ << "On Line" << txtLineNumber_ << ". did you mean \"SET event_tag=\"?" << std::endl;
				__SS_THROW__;
			}
			break;
		// case CFO_INSTR::AND:
		// 	if (parameterBuffer_ == 0)
		// 	{
		// 		__SS__ << "On Line " << txtLineNumber_ << ". AND has a null  or invalid argument (AND 0 Event)"
		// 				  << std::endl;
		// 		throw std::exception();
		// 	}
		// 	else if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_tag")
		// 	{
		// 		__SS__ << "On Line" << txtLineNumber_ << ". did you mean \"AND event_tag=\"?" << std::endl;
		// 		throw std::exception();
		// 	}
		// 	break;
		// case CFO_INSTR::OR:
		// 	if (parameterBuffer_ == 0)
		// 	{
		// 		__SS__ << "On Line " << txtLineNumber_ << ". OR has a null  or invalid argument (OR 0 Event)"
		// 				  << std::endl;
		// 		throw std::exception();
		// 	}
		// 	else if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_tag")
		// 	{
		// 		__SS__ << "On Line" << txtLineNumber_ << ". did you mean \"OR event_tag=\"?" << std::endl;
		// 		throw std::exception();
		// 	}
		// 	break;
		case CFO_INSTR::LOOP:
			if (parameterBuffer_ == 0)
			{
				__SS__ << "On Line " << txtLineNumber_ << ". LOOP has a null  or invalid argument (Looping 0 times)"
						  << std::endl;
				__SS_THROW__;
			}
			else if (identifierBuffer_.empty() == 0 && identifierBuffer_ != "times")
			{
				__SS__ << "On Line " << txtLineNumber_ << ". LOOP has an invalid identifier ("
						  << identifierBuffer_ << ")" << std::endl;
				__SS_THROW__;
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "count")
			{
				__SS__ << "On Line" << txtLineNumber_ << ". did you mean \"LOOP count=\"?" << std::endl;
				__SS_THROW__;
			}
			break;
		case CFO_INSTR::DO_LOOP:
			if (parameterBuffer_ != 0)
			{
				__SS__ << "On Line " << txtLineNumber_ << ". DO LOOP has an extraneous argument("
						  << parameterBuffer_ << ")" << std::endl;
				__SS_THROW__;
			}
			break;
		case CFO_INSTR::WAIT:
			if (identifierBuffer_.empty() == 0 && identifierBuffer_ != "clocks" && identifierBuffer_ != "ns" &&
				identifierBuffer_ != "sec" && identifierBuffer_ != "ms" && identifierBuffer_ != "us")
			{
				__SS__ << "On Line " << txtLineNumber_ << ". WAIT has an invalid identifier("
						  << identifierBuffer_ << ")" << std::endl <<
						 "Options for third argument are [empty] ns ms sec" << std::endl;
				__SS_THROW__;
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "period" && argumentBuffer_ != "NEXT")
			{
				__SS__ << "On Line" << txtLineNumber_ << ". did you mean \"WAIT period=\"?" << std::endl;
				__SS_THROW__;
			}

			break;
		case CFO_INSTR::REPEAT:
			if (parameterBuffer_ != 0 && identifierBuffer_.empty())
			{
				__SS__ << "On Line " << txtLineNumber_ << ". REPEAT has extraneous arguments("
						  << parameterBuffer_ << "_" << identifierBuffer_ << ")" << std::endl;
				__SS_THROW__;
			}
			break;
		case CFO_INSTR::END:
			if (parameterBuffer_ != 0 && identifierBuffer_.empty())
			{
				__SS__ << "On Line " << txtLineNumber_ << ". END has extraneous arguments("
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
} //end errorCheck()

//========================================================================
void CFOLib::CFO_Compiler::macroErrorCheck(CFO_MACRO macroInt)
{
	switch (macroInt)
	{
		case CFO_MACRO::SLICE:
			if (macroArgument_[0] != "bitposition")
			{
				__SS__<< "On Line (" << txtLineNumber_ << ") " << 
					"1st parameter should be bitposition=" << __E__;
				__SS_THROW__;
			}
			else if ((atoi(macroArgument_[1].c_str()) > 48) || (atoi(macroArgument_[1].c_str()) < 1))
			{
				__SS__ << "On Line (" << txtLineNumber_ << ") " << 
					"2nd parameter, integer must be between 1 and 48" << __E__;
				__SS_THROW__;
			}
			else if (macroArgument_[2] != "bitwidth")
			{
				__SS__ << "On Line (" << txtLineNumber_ << ") " << 
					"3rd parameter should be bitwidth=" << __E__;
				__SS_THROW__;
			}
			else if ((atoi(macroArgument_[3].c_str()) + atoi(macroArgument_[1].c_str()) - 1 > 48) ||
					 (atoi(macroArgument_[3].c_str()) < 1))
			{
				__SS__ << "On Line (" << txtLineNumber_ << ") " <<
					"4th parameter, integer must be between 1 and must add with bitposition to less than 48" << __E__;
				__SS_THROW__;
			}
			else if (macroArgument_[4] != "event_tag")
			{
				__SS__ << "On Line (" << txtLineNumber_ << ") " << 
					"5th parameter should be event_tag=" << __E__;
				__SS_THROW__;
			}
			else if ((atoi(macroArgument_[5].c_str()) > 1536))
			{
				__SS__ << "On Line (" << txtLineNumber_ << ") " <<
					"6th and last parameter, integer must be between 1 and 1536" << __E__;
				__SS_THROW__;
			}
			break;
		default:
			break;
	}
} //end macroErrorCheck()

//========================================================================
void CFOLib::CFO_Compiler::outParameter(uint64_t paramBuf)  // Writes the 6 byte parameter out based on a given integer.
{
	paramBuf &= 0xFFFFFFFFFFFF;  // Enforce 6 bytes
	auto paramBufPtr = reinterpret_cast<int8_t*>(&paramBuf);
	for (int i = 0; i < 6; ++i)
	{
		output_.push_back(paramBufPtr[i]);
		__COUT__ << "[" << output_.size() << "] ==> output_ 0x" << std::hex << std::setfill('0') << std::setprecision(2) << (uint16_t)output_.back() << __E__;
	}
} //end outParameter()

//========================================================================
// Transcription
void CFOLib::CFO_Compiler::transcribeInstruction()  // Outputs a byte stream based on the buffers.
{
	//calculate line numbers that match this hexdump call (all usage of bin is relative/differences):
	//		hexdump -e '"%08_ax | " 1/8 "%016x "' -e '"\n"'  CFOCommands.bin | cat -n
	binLineNumber_ = 1 + output_.size()/8;
	__COUT__ << "binLineNumber_: " << (int)binLineNumber_ << __E__;

	CFO_INSTR instructionOpcode = parse_instruction(instructionBuffer_);

	if (instructionOpcode == CFO_INSTR::INVALID)
	{
		__SS__ << "ERROR: INVALID INSTRUCTION: " << instructionBuffer_ << std::endl;
		__SS_THROW__;
	}

	int64_t parameterCalc = calcParameter(instructionOpcode);
	__COUT__ << "instructionOpcode: " << (int)instructionOpcode << __E__;
	__COUT__ << "parameterCalc: " << parameterCalc << __E__;

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
			__COUT__ << "loopStack_ = " << binLineNumber_ << " at " << loopStack_.size() << " w/parameterBuffer_=" << parameterBuffer_;
			loopStack_.push(binLineNumber_);
			outParameter(parameterBuffer_);
			break;

		case CFO_INSTR::DO_LOOP:
			__COUT__ << "DO_LOOP loopStack_ " << loopStack_.size() << " w/parameterCalc=" << parameterCalc;
			outParameter(parameterCalc);
			break;

		case CFO_INSTR::REPEAT:
			outParameter(0);
			break;

		case CFO_INSTR::WAIT:
			__COUT__ << "parameterCalc = " << std::dec << parameterCalc << " clocks"
				<< std::hex << " = 0x" << parameterCalc << " clocks" << __E__;
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
	__COUT__ << "[" << output_.size() << "] ==> output_ 0x" << std::hex << std::setfill('0') << std::setprecision(2) << (uint16_t)output_.back() << __E__;
	output_.push_back(static_cast<char>(instructionOpcode));
	__COUT__ << "[" << output_.size() << "] ==> output_ 0x" << std::hex << std::setfill('0') << std::setprecision(2) << (uint16_t)output_.back() << __E__;

} //end transcribeInstruction()

//========================================================================
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
} //end transcribeMacro()

//========================================================================
// Parameter Calculations
uint64_t CFOLib::CFO_Compiler::calcParameter(CFO_INSTR instructionOpcode)  // Calculates the parameter for the
																		  // instruction (if needed)
{
	__COUT__ << "calcParameter... Instruction: " << instructionBuffer_ << 
		", Parameter: '" << argumentBuffer_ << "' = " << parameterBuffer_  <<
		" [" << identifierBuffer_ << "]" << __E__;

	uint64_t loopLine;
	switch (instructionOpcode)
	{
		case CFO_INSTR::INC:
			if (parameterBuffer_ == 0 || argumentBuffer_ == "event_tag")
			{
				return 1;
			}
			else
			{
				return parameterBuffer_;
			}
			break;

		case CFO_INSTR::WAIT:
			__COUT__ << "FPGAClock_ = " << FPGAClock_ << " ns" << __E__;
			if (parameterBuffer_ == 0 && argumentBuffer_ != "NEXT")
				return 1;
			else if (parameterBuffer_ == 0 && argumentBuffer_ == "NEXT")
				return -1;
			else if (identifierBuffer_ == "sec")  // Wait wanted in seconds
				return parameterBuffer_ * 1e9 / FPGAClock_;
			else if (identifierBuffer_ == "ms")  // Wait wanted in milliseconds
				return parameterBuffer_ * 1e6 / FPGAClock_;
			else if (identifierBuffer_ == "us")  // Wait wanted in microseconds
				return parameterBuffer_ * 1e3 / FPGAClock_;
			else if (identifierBuffer_ == "ns")  // Wait wanted in nanoseconds
			{
				if ((parameterBuffer_ % FPGAClock_) != 0)
				{
					__SS__ << "FPGA can only wait in multiples of " <<
						FPGAClock_ << " ns: " << parameterBuffer_ << " has remainder " <<
						(parameterBuffer_ % FPGAClock_) << __E__;
					__SS_THROW__;
				}
				return parameterBuffer_ / FPGAClock_;
			}
			else if (identifierBuffer_ == "clocks")  // Wait wanted in FPGA clocks
			{
				return parameterBuffer_;
			}
			else
			{
				__SS__ << "WAIT command is missing unit type after parameter. Command found as" <<
					"... Parameter: '" << argumentBuffer_ << "' = " << parameterBuffer_  <<
					" [" << identifierBuffer_ << "]. Accepted unit types are clocks, ns, us, ms, sec." << __E__; 
				__SS_THROW__;
			}
			break;

		case CFO_INSTR::DO_LOOP:
			if (!loopStack_.empty())
			{
				loopLine = loopStack_.top();
				__COUT__ << "DO_LOOP from [" << binLineNumber_ << 
					"], loop line popped = " << loopLine <<
					", must go back	" << binLineNumber_ - loopLine << " lines.";
				loopStack_.pop();
				return (binLineNumber_ - loopLine);
			}
			else
			{
				__SS__ << "LOOP/DO_LOOP counts don't match. (More DO_LOOP's)";
				__SS_THROW__;
			}
			break;

		case CFO_INSTR::END:
			if (!loopStack_.empty())
			{
				__SS__ << "LOOP/DO_LOOP counts don't match (Less DO_LOOP's)";
				__SS_THROW__;
			}
		default:
			break;
	}
	return 0;
} //end calcParameter()


