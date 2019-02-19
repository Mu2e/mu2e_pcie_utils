#include "cfoInterfaceLib/CFO_Compiler.hh"

#include <iostream>

std::deque<char> CFOLib::CFO_Compiler::processFile(CFO_Source_File const& file)
{
	inFile_ = file;
	lineNumber_ = 1;
	output_.clear();
	readSpace();
	while (!(inFile_.eof())) {
		// cout << "\nlineNumber: " << lineNumber; //test
		if (!(isComment()) && inFile_.peek() != '\n') {
			readLine();  // Reading line
			//  cout << ". " << instructionBuffer; //test
			// Transcription into output file.
			if (!macroFlag_)
				transcribeIns();
			else
				transcribeMacro();
		}
		newLine();
		readSpace();
		lineNumber_++;
	}
	inFile_.close();
	std::cout << "Parsing Complete!" << std::endl;
	return output_;
}

/**********************************************
 *Read/Input Functions
 *********************************************/
void CFOLib::CFO_Compiler::readLine()  // Reads one line and stores into instruction, parameter, argument and identifier
									   // Buffers.
{
	instructionBuffer_ = readInstruction();
	readSpace();
	macroFlag_ = isMacro();

	if (!(isComment()) && !(macroFlag_)) {
		if (std::isdigit(inFile_.peek()))  //
		{
			argumentBuffer_.clear();
			parameterBuffer_ = atoi(readInstruction().c_str());
			readSpace();
		}
		else
		{
			argumentBuffer_ = readInstruction();
			readSpace();
			parameterBuffer_ = atoi(readInstruction().c_str());
			readSpace();
		}

		if (!(isComment())) {
			identifierBuffer_ = readInstruction();
		}
		else
		{
			identifierBuffer_.clear();
		}
	}
	else if (macroFlag_)
	{
		argumentBuffer_.clear();
		parameterBuffer_ = 0;
		identifierBuffer_.clear();

		readMacro();
	}
	else
	{
		argumentBuffer_.clear();
		parameterBuffer_ = 0;
		identifierBuffer_.clear();
	}
}

// Basic Read Algorithms
//************************
void CFOLib::CFO_Compiler::readSpace()
{  // Read white space.
	while ((inFile_.peek() == ' ' || inFile_.peek() == '\t') && !(inFile_.eof())) {
		inFile_.get();
	}
}

void CFOLib::CFO_Compiler::newLine()  // Reads until next line or until end of file.
{
	while (inFile_.peek() != '\n' && !inFile_.eof()) {
		inFile_.get();
	}
	inFile_.get();
}

std::string
CFOLib::CFO_Compiler::readInstruction()  // reads on space separated word (with an exception for DATA REQ and DO LOOP)
{
	char insChar;
	std::string insString;

	while (inFile_.peek() != ' ' && inFile_.peek() != '\n' && inFile_.peek() != '=' && !inFile_.eof()) {
		insChar = inFile_.get();
		insString += insChar;
	}
	if (inFile_.peek() == '=') {
		inFile_.get();
	}
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
void CFOLib::CFO_Compiler::readMacro()
{
	macroSetup(instructionBuffer_);

	for (int i = 0; i < macroArgCount_; i++) {
		macroArgument_.push_back(readInstruction());
		readSpace();
	}
}

/**********************************************
                          Internal Functions
 *********************************************/
/**************** AUXILIARY ******************/
// Boolean Operators
bool CFOLib::CFO_Compiler::isComment()  // Checks if line has a comment.
{
	if (inFile_.peek() == '/') {
		inFile_.get();
		if (inFile_.get() != '/') {
			std::string errorMessage = "Error: Missing slash for comment. (Line " + lineNumber_ + std::string(")");
			throw std::runtime_error(errorMessage);
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
	if (macroOpcode_ == CFO_MACRO::NON_MACRO) {
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
	if (instructionBuffer == "START") {
		return CFO_INSTR::START;
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
	else if (instructionBuffer == "AND")
	{
		return CFO_INSTR::AND;
	}
	else if (instructionBuffer == "OR")
	{
		return CFO_INSTR::OR;
	}
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
	if (instructionBuffer == "SLICE") {
		return CFO_MACRO::SLICE;
	}
	return CFO_MACRO::NON_MACRO;
}

// Macro Argument and Instruction Counts
void CFOLib::CFO_Compiler::macroSetup(std::string instructionBuffer)
{
	if (instructionBuffer == "SLICE") {
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
		case CFO_INSTR::START:
			if (parameterBuffer_ == 0) {
				std::cout << "ERROR: On Line " << lineNumber_ << ". START has a null or invalid argument (START 0 Event)"
						  << std::endl;
				throw std::exception();
			}
			if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_mode") {
				std::cout << "ERROR: On Line" << lineNumber_ << ". did you mean \"START event_mode=\"?" << std::endl;
				throw std::exception();
			}
			break;

		case CFO_INSTR::DATA_REQUEST:
			if (parameterBuffer_ == 0 && argumentBuffer_ != "CURRENT") {
				std::cout << "ERROR: On Line " << lineNumber_ << ". DATA REQ has a null  or invalid argument (DATA REQ 0 Event)"
						  << std::endl;
				throw std::exception();
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "request_tag" && argumentBuffer_ != "CURRENT")
			{
				std::cout << "ERROR: On Line" << lineNumber_
						  << ". did you mean \"DATA_REQUEST request_tag=\" or DATA_REQUEST CURRENT?" << std::endl;
				throw std::exception();
			}
			break;

		case CFO_INSTR::INC:
			if (identifierBuffer_.empty() == false) {
				std::cout << "WARNING: On Line " << lineNumber_ << ". INC has an extraneous identifier" << std::endl;
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_by" && argumentBuffer_ != "event_tag")
			{
				std::cout << "ERROR: On Line" << lineNumber_ << ". did you mean \"INC event_by=\" or \"INC event_tag\?"
						  << std::endl;
				throw std::exception();
			}
			break;
		case CFO_INSTR::SET:
			if (parameterBuffer_ == 0) {
				std::cout << "\nERROR: On Line " << lineNumber_ << ". SET has a null  or invalid argument (SET 0 Event)"
						  << std::endl;
				throw std::exception();
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_tag")
			{
				std::cout << "ERROR: On Line" << lineNumber_ << ". did you mean \"SET event_tag=\"?" << std::endl;
				throw std::exception();
			}
			break;
		case CFO_INSTR::AND:
			if (parameterBuffer_ == 0) {
				std::cout << "\nERROR: On Line " << lineNumber_ << ". AND has a null  or invalid argument (AND 0 Event)"
						  << std::endl;
				throw std::exception();
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_tag")
			{
				std::cout << "ERROR: On Line" << lineNumber_ << ". did you mean \"AND event_tag=\"?" << std::endl;
				throw std::exception();
			}
			break;
		case CFO_INSTR::OR:
			if (parameterBuffer_ == 0) {
				std::cout << "\nERROR: On Line " << lineNumber_ << ". OR has a null  or invalid argument (OR 0 Event)"
						  << std::endl;
				throw std::exception();
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "event_tag")
			{
				std::cout << "ERROR: On Line" << lineNumber_ << ". did you mean \"OR event_tag=\"?" << std::endl;
				throw std::exception();
			}
			break;
		case CFO_INSTR::LOOP:
			if (parameterBuffer_ == 0) {
				std::cout << "ERROR: On Line " << lineNumber_ << ". LOOP has a null  or invalid argument (Looping 0 times)"
						  << std::endl;
				throw std::exception();
			}
			else if (identifierBuffer_.empty() == 0 && identifierBuffer_ != "times")
			{
				std::cout << "ERROR: On Line " << std::to_string(lineNumber_) << ". LOOP has an invalid identifier ("
						  << identifierBuffer_ << ")" << std::endl;
				throw std::runtime_error("LOOP runtime_error thrown");
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "count")
			{
				std::cout << "ERROR: On Line" << lineNumber_ << ". did you mean \"LOOP count=\"?" << std::endl;
				throw std::exception();
			}
			break;
		case CFO_INSTR::DO_LOOP:
			if (parameterBuffer_ != 0) {
				std::cout << "WARNING: On Line " << std::dec << lineNumber_ << ". DO LOOP has an extraneous argument("
						  << parameterBuffer_ << ")" << std::endl;
			}
			break;
		case CFO_INSTR::WAIT:
			if (identifierBuffer_.empty() == 0 && identifierBuffer_ != "cycles" && identifierBuffer_ != "ns" &&
				identifierBuffer_ != "sec" && identifierBuffer_ != "ms") {
				std::cout << "WARNING: On Line " << std::dec << lineNumber_ << ". WAIT has an invalid identifier("
						  << identifierBuffer_ << ")" << std::endl;
				std::cout << "Options for third argument are [empty] ns ms sec" << std::endl;
			}
			else if (argumentBuffer_.empty() == false && argumentBuffer_ != "period" && argumentBuffer_ != "NEXT")
			{
				std::cout << "ERROR: On Line" << lineNumber_ << ". did you mean \"WAIT period=\"?" << std::endl;
				throw std::exception();
			}

			break;
		case CFO_INSTR::REPEAT:
			if (parameterBuffer_ != 0 && identifierBuffer_.empty() == 1) {
				std::cout << "WARNING: On Line " << std::dec << lineNumber_ << ". REPEAT has extraneous arguments("
						  << parameterBuffer_ << "_" << identifierBuffer_ << ")" << std::endl;
			}
			break;
		case CFO_INSTR::END:
			if (parameterBuffer_ != 0 && identifierBuffer_.empty() == 1) {
				std::cout << "WARNING: On Line " << std::dec << lineNumber_ << ". END has extraneous arguments("
						  << parameterBuffer_ << "_" << identifierBuffer_ << ")" << std::endl;
			}
			break;
		default:
			break;
	}
}

void CFOLib::CFO_Compiler::macroErrorCheck(CFO_MACRO macroInt)
{
	switch (macroInt)
	{
		case CFO_MACRO::SLICE:
			if (macroArgument_[0] != "bitposition") {
				std::cout << "ERROR: On Line (" << lineNumber_ << ")";
				throw std::runtime_error("1st parameter should be bitposition=");
			}
			else if ((atoi(macroArgument_[1].c_str()) > 48) || (atoi(macroArgument_[1].c_str()) < 1))
			{
				std::cout << "ERROR: On Line (" << lineNumber_ << ")";
				throw std::runtime_error("2nd parameter, integer must be between 1 and 48");
			}
			else if (macroArgument_[2] != "bitwidth")
			{
				std::cout << "ERROR: On Line (" << lineNumber_ << ")";
				throw std::runtime_error("3rd parameter should be bitwidth=");
			}
			else if ((atoi(macroArgument_[3].c_str()) + atoi(macroArgument_[1].c_str()) - 1 > 48) ||
					 (atoi(macroArgument_[3].c_str()) < 1))
			{
				std::cout << "ERROR: On Line (" << lineNumber_ << ")";
				throw std::runtime_error(
					"4th parameter, integer must be between 1 and must add with bitposition to less than 48");
			}
			else if (macroArgument_[4] != "event_tag")
			{
				std::cout << "ERROR: On Line (" << lineNumber_ << ")";
				throw std::runtime_error("5th parameter should be event_tag=");
			}
			else if ((atoi(macroArgument_[5].c_str()) > 1536))
			{
				std::cout << "ERROR: On Line (" << lineNumber_ << ")";
				throw std::runtime_error("6th and last parameter, integer must be between 1 and 1536");
			}
			break;
		default:
			break;
	}
}

/****************TRANSCRIPTION********************/
// Transcription
void CFOLib::CFO_Compiler::transcribeIns()  // Outputs a byte stream based on the buffers.
{
	CFO_INSTR instructionOpcode = parse_instruction(instructionBuffer_);

	if (instructionOpcode == CFO_INSTR::INVALID) {
		std::cout << "ERROR: INVALID INSTRUCTION" << std::endl;
		return;
	}

	int64_t parameterCalc = calcParameter(instructionOpcode);

	errorCheck(instructionOpcode);
	output_.push_back(static_cast<char>(instructionOpcode));
	output_.push_back(0x00);

	switch (instructionOpcode)
	{
			// Instructions with value;
		case CFO_INSTR::START:
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

		case CFO_INSTR::AND:
			outParameter(parameterBuffer_);
			break;

		case CFO_INSTR::OR:
			outParameter(parameterBuffer_);
			break;

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
			std::cout << "ERROR: INVALID INSTRUCTION" << std::endl;
			break;
	}
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
			transcribeIns();
			feedInstruction("OR", "", parameterOR, "");
			transcribeIns();
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
			if (parameterBuffer_ == 0 or argumentBuffer_ == "event_tag") {
				return 1;
			}
			else
			{
				return parameterBuffer_;
			}
			break;

		case CFO_INSTR::WAIT:
			if (parameterBuffer_ == 0 && argumentBuffer_ != "NEXT") {
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

				if ((parameterBuffer_ % (paramTemp_ns) != 0)) {
					std::cout << "WARNING: FPGA can only wait in units of 25ns ()" << std::endl;
				}
				return paramTemp_ns;
			}
			else
			{
				return parameterBuffer_;
			}
			break;

		case CFO_INSTR::DO_LOOP:
			if (!loopStack_.empty()) {
				loopLine = loopStack_.top();
				loopStack_.pop();
				return (lineNumber_ - loopLine);
			}
			else
			{
				std::string errorMessage2 = "WARNING: Loop/DoLoop counts don't match. (More DO LOOPS) ";
				throw std::runtime_error(errorMessage2);
			}
			break;

		case CFO_INSTR::END:
			if (!loopStack_.empty()) {
				std::string errorMessage3 = "WARNING: LOOP/DO LOOP counts don't match (Less DO LOOPS)";
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
	for (int i = 5; i >= 0; i--) {
		output_.push_back(paramBufPtr[i]);
	}
}
