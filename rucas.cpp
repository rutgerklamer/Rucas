// Runs a Rucas program, one instruction at a time; code outside any loop runs as it is read.
//   rucas <file.ruc> [more .ruc files] [input file]
#include <vector>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>
#include <cstdint>
#include <algorithm>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

enum Op
{
	ADD, SUB, JMPR, JMPL, JMP, PRNTA, PRNTD, CINP, STLP, NDLP, IF, IF_EQL, ENDIF, HALT,
	PSHS, CLRS, RMVS, EQL_TOPS, ADDS, SUBS, MULS, DIVS, MODS, ELSE, IF_LESS, IF_MORE
};

enum Arg { NONE, NUMBER, CELL, STACK };

struct Instruction
{
	Op op;
	Arg arg;
	uint32_t n;
	int line;
	int file;
	int match;
};

const char* const OP_NAMES[] = {
	"ADD", "SUB", "JMPR", "JMPL", "JMP", "PRNTA", "PRNTD", "CINP", "STLP", "NDLP", "IF", "IF EQL", "ENDIF", "HALT",
	"PSHS", "CLRS", "RMVS", "EQL TOPS", "ADDS", "SUBS", "MULS", "DIVS", "MODS", "ELSE", "IF LESS", "IF MORE"
};
const int OP_COUNT = sizeof(OP_NAMES) / sizeof(OP_NAMES[0]);

int m_bits = 8;
uint32_t m_mask = 0xFF;
const long long TAPE_SIZE = 30000;
std::vector<uint32_t> m_cells(TAPE_SIZE);
std::vector<uint32_t> m_stack;
std::vector<Instruction> m_program;
long long m_cell = 0;
FILE* m_input = stdin;
bool m_started = false;
std::vector<std::string> m_files;
int m_file = 0;

// the program is read in pieces; code outside any loop runs once, so it runs as it is read and is dropped
const size_t PIECE = 1 << 20;
const size_t SEGMENT = 4096;

struct Reader
{
	FILE* file;
	std::vector<char> piece;
	size_t at, size;
	int line;
};

bool runFiles(bool execute);
bool readLine(Reader& r, std::string& text);
int parseLine(const std::string& text, int line, Instruction& ins);
bool checkOrder(const Instruction& ins, std::vector<Instruction>& open);
bool runSegment();
bool parseArg(const std::string& text, Instruction& ins);
void matchBlocks();
bool runCode();
uint32_t getValue(const Instruction& ins);
void movePointer(long long n);
uint32_t stackTop(size_t depth);
void fail(const Instruction& ins, const std::string& message);
std::string where(int file, int line);

int main(int argc, char const *argv[])
{
	// the first file always counts as the program, so every command line from before still works
	const char* input = nullptr;
	bool usable = argc >= 2;
	for (int i = 1; i < argc && usable; i++) {
		std::string arg = argv[i];
		if (i == 1 || (arg.size() > 4 && arg.compare(arg.size() - 4, 4, ".ruc") == 0))
			m_files.push_back(arg);
		else if (i == argc - 1)
			input = argv[i];
		else
			usable = false;
	}
	if (!usable) {
		std::cerr << "usage: rucas <file.ruc> [more .ruc files] [input file]" << std::endl;
		return 1;
	}
#ifdef _WIN32
	// bytes in and out as they are, without turning \n into \r\n
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	if (input) {
		m_input = std::fopen(input, "rb");
		if (!m_input) {
			std::cerr << "error: cannot open " << input << std::endl;
			return 1;
		}
	}
	// checked in full first, since code outside loops runs while it is read
	if (!runFiles(false))
		return 1;
	if (!runFiles(true))
		return 1;
	std::cout << std::flush;
	return 0;
}

// read as one program, so a loop can open in a base file and close in the program's own file
bool runFiles(bool execute)
{
	std::vector<Instruction> open;
	std::string text;
	Instruction ins;
	m_started = false;
	bool going = true;
	for (m_file = 0; going && m_file < (int)m_files.size(); m_file++) {
		Reader r = {std::fopen(m_files[m_file].c_str(), "rb"), std::vector<char>(PIECE), 0, 0, 0};
		if (!r.file) {
			std::cerr << "error: cannot open " << m_files[m_file] << std::endl;
			return false;
		}
		while (going && readLine(r, text)) {
			int got = parseLine(text, r.line, ins);
			if (got < 0 || (got > 0 && !checkOrder(ins, open)))
				return false;
			if (got == 0 || !execute)
				continue;
			m_program.push_back(ins);
			if (open.empty() && m_program.size() >= SEGMENT)
				going = runSegment();
		}
		std::fclose(r.file);
	}
	if (going && !open.empty()) {
		std::cerr << "error: " << where(open.back().file, open.back().line) << ": " << OP_NAMES[open.back().op] << " is never closed" << std::endl;
		return false;
	}
	if (going && execute)
		runSegment();
	return true;
}

bool readLine(Reader& r, std::string& text)
{
	text.clear();
	bool any = false;
	for (;;) {
		if (r.at == r.size) {
			r.size = std::fread(r.piece.data(), 1, r.piece.size(), r.file);
			r.at = 0;
			if (r.size == 0) {
				if (any)
					r.line++;
				return any;
			}
		}
		any = true;
		const char* start = r.piece.data() + r.at;
		const char* end = static_cast<const char*>(std::memchr(start, '\n', r.size - r.at));
		if (end) {
			text.append(start, end - start);
			r.at += end - start + 1;
			r.line++;
			return true;
		}
		text.append(start, r.size - r.at);
		r.at = r.size;
	}
}

// 1 is an instruction, 0 is nothing on this line (or BITS), -1 is an error, already reported
int parseLine(const std::string& text, int line, Instruction& ins)
{
	static std::string op, arg, extra;
	// a # starts a comment, on its own line or after an instruction
	size_t i = 0, n = std::min(text.size(), text.find('#'));
	auto word = [&](std::string& w) {
		while (i < n && (text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\v' || text[i] == '\f'))
			i++;
		size_t from = i;
		while (i < n && !(text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\v' || text[i] == '\f'))
			i++;
		w.assign(text, from, i - from);
	};
	word(op);
	if (op.empty())
		return 0;
	word(arg);
	if ((op == "IF" && (arg == "EQL" || arg == "LESS" || arg == "MORE")) || (op == "EQL" && arg == "TOPS")) {
		op += " " + arg;
		word(arg);
	}
	word(extra);
	if (!extra.empty()) {
		std::cerr << "error: " << where(m_file, line) << ": unexpected '" << extra << "'" << std::endl;
		return -1;
	}

	// BITS sets up the machine, so it comes before the first instruction
	if (op == "BITS") {
		if (m_started) {
			std::cerr << "error: " << where(m_file, line) << ": BITS must come before the first instruction" << std::endl;
			return -1;
		}
		Instruction value;
		if (!parseArg(arg, value) || value.arg != NUMBER || (value.n != 8 && value.n != 16 && value.n != 32)) {
			std::cerr << "error: " << where(m_file, line) << ": BITS takes 8, 16 or 32" << std::endl;
			return -1;
		}
		m_bits = (int)value.n;
		m_mask = (m_bits == 32 ? 0xFFFFFFFFu : (1u << m_bits) - 1);
		return 0;
	}

	ins.line = line;
	ins.file = m_file;
	ins.match = -1;
	int k = 0;
	while (k < OP_COUNT && op != OP_NAMES[k])
		k++;
	if (k == OP_COUNT) {
		std::cerr << "error: " << where(m_file, line) << ": unknown instruction '" << op << "'" << std::endl;
		return -1;
	}
	ins.op = (Op)k;
	if (!parseArg(arg, ins)) {
		std::cerr << "error: " << where(m_file, line) << ": '" << arg << "' is not a valid argument (expected N, $CELL or $STACK)" << std::endl;
		return -1;
	}
	m_started = true;
	return 1;
}

bool checkOrder(const Instruction& ins, std::vector<Instruction>& open)
{
	Op op = ins.op;
	if ((op == JMP || op == IF_EQL || op == IF_LESS || op == IF_MORE) && ins.arg == NONE) {
		std::cerr << "error: " << where(ins.file, ins.line) << ": " << OP_NAMES[op] << (op == JMP ? " needs a cell number" : " needs a value to compare with") << std::endl;
		return false;
	}
	// an ELSE takes the place of its IF, so a second ELSE or an NDLP finds no IF to belong to
	bool in_if = !open.empty() && open.back().op != STLP;
	if (op == STLP || op == IF || op == IF_EQL || op == IF_LESS || op == IF_MORE)
		open.push_back(ins);
	else if (op == ELSE) {
		if (!in_if || open.back().op == ELSE) {
			std::cerr << "error: " << where(ins.file, ins.line) << ": ELSE without matching IF" << std::endl;
			return false;
		}
		open.back() = ins;
	}
	else if (op == NDLP || op == ENDIF) {
		bool loop = (op == NDLP);
		if (open.empty() || in_if == loop) {
			std::cerr << "error: " << where(ins.file, ins.line) << ": " << OP_NAMES[op] << " without matching " << (loop ? "STLP" : "IF") << std::endl;
			return false;
		}
		open.pop_back();
	}
	return true;
}

// false once the program has stopped at HALT
bool runSegment()
{
	matchBlocks();
	bool going = runCode();
	m_program.clear();
	return going;
}

bool parseArg(const std::string& text, Instruction& ins)
{
	// without an argument the value is 1, except for IF, which then looks at the cell
	ins.n = 1;
	if (text.empty())
		ins.arg = NONE;
	else if (text == "$CELL")
		ins.arg = CELL;
	else if (text == "$STACK")
		ins.arg = STACK;
	else {
		if (text.size() > 10)
			return false;
		unsigned long long value = 0;
		for (char c : text) {
			if (c < '0' || c > '9')
				return false;
			value = value * 10 + (c - '0');
		}
		if (value > 0xFFFFFFFFull)
			return false;
		ins.arg = NUMBER;
		ins.n = (uint32_t)value;
	}
	return true;
}

// a segment holds whole loops and IF blocks only (checkOrder has seen them), so every end has its start
void matchBlocks()
{
	std::vector<int> open;
	for (int i = 0; i < (int)m_program.size(); i++) {
		Op op = m_program[i].op;
		if (op == STLP || op == IF || op == IF_EQL || op == IF_LESS || op == IF_MORE)
			open.push_back(i);
		// a false IF jumps to its ELSE, and the ELSE (reached from the true branch) jumps to ENDIF
		else if (op == ELSE || op == NDLP || op == ENDIF) {
			m_program[i].match = open.back();
			m_program[open.back()].match = i;
			open.pop_back();
			if (op == ELSE)
				open.push_back(i);
		}
	}
}

bool runCode()
{
	const int size = (int)m_program.size();
	const Instruction* const program = m_program.data();
	uint32_t* const tape = m_cells.data();
	const uint32_t mask = m_mask;
	for (int i = 0; i < size; i++) {
		const Instruction& ins = program[i];
		uint32_t& cell = tape[m_cell];

		switch (ins.op) {
		case ADD:
			cell = (cell + getValue(ins)) & mask;
			break;
		case SUB:
			cell = (cell - getValue(ins)) & mask;
			break;
		case JMPR:
			movePointer(getValue(ins));
			break;
		case JMPL:
			movePointer(-(long long)getValue(ins));
			break;
		case JMP: {
			uint32_t target = getValue(ins);
			if (target >= TAPE_SIZE)
				fail(ins, "cell " + std::to_string(target) + " is outside the tape");
			m_cell = target;
			break;
		}
		case PRNTA:
		case PRNTD: {
			uint32_t count = getValue(ins);
			for (uint32_t j = 0; j < count; j++) {
				uint32_t value = m_cells[(m_cell + j) % TAPE_SIZE];
				if (ins.op == PRNTA)
					std::cout.put(static_cast<char>(value));
				else
					std::cout << (j > 0 ? " " : "") << value;
			}
			break;
		}
		case CINP: {
			if (m_input == stdin)
				std::cout << std::flush;
			int c = m_input ? std::getc(m_input) : EOF;
			cell = (c == EOF ? 0 : (uint32_t)(unsigned char)c);
			break;
		}
		case STLP:
			if (cell == 0)
				i = ins.match;
			break;
		case NDLP:
			if (cell != 0)
				i = ins.match;
			break;
		case IF:
			if ((ins.arg == NONE ? cell : getValue(ins)) == 0)
				i = ins.match;
			break;
		case IF_EQL:
			if (getValue(ins) != cell)
				i = ins.match;
			break;
		case IF_LESS:
			if (!(cell < getValue(ins)))
				i = ins.match;
			break;
		case IF_MORE:
			if (!(cell > getValue(ins)))
				i = ins.match;
			break;
		case ELSE:
			i = ins.match;
			break;
		case ENDIF:
			break;
		case PSHS:
			m_stack.push_back(cell);
			break;
		case CLRS:
			m_stack.clear();
			break;
		case RMVS:
			if (!m_stack.empty())
				m_stack.erase(m_stack.begin());
			break;
		case EQL_TOPS:
			cell = stackTop(0);
			break;
		case ADDS:
			m_stack.push_back((stackTop(0) + stackTop(1)) & m_mask);
			break;
		case SUBS:
			m_stack.push_back((stackTop(0) - stackTop(1)) & m_mask);
			break;
		case MULS:
			m_stack.push_back((stackTop(0) * stackTop(1)) & m_mask);
			break;
		case DIVS:
		case MODS: {
			uint32_t top = stackTop(0), second = stackTop(1);
			if (second == 0)
				fail(ins, std::string(OP_NAMES[ins.op]) + " by 0: the second value on the stack is 0");
			m_stack.push_back(ins.op == DIVS ? top / second : top % second);
			break;
		}
		case HALT:
			return false;
		}
	}
	return true;
}

inline uint32_t getValue(const Instruction& ins)
{
	if (ins.arg == CELL)
		return m_cells[m_cell];
	if (ins.arg == STACK)
		return stackTop(0);
	return ins.n;
}

void movePointer(long long n)
{
	m_cell += n;
	if (m_cell < 0 || m_cell >= TAPE_SIZE)
		m_cell = (m_cell % TAPE_SIZE + TAPE_SIZE) % TAPE_SIZE;
}

uint32_t stackTop(size_t depth)
{
	if (m_stack.size() <= depth)
		return 0;
	return m_stack[m_stack.size() - 1 - depth];
}

void fail(const Instruction& ins, const std::string& message)
{
	std::cout << std::flush;
	std::cerr << "\nerror: " << where(ins.file, ins.line) << ": " << message << std::endl;
	std::exit(1);
}

// with several files a line number alone no longer says where, so the file is named too
std::string where(int file, int line)
{
	return (m_files.size() > 1 ? m_files[file] + " " : "") + "line " + std::to_string(line);
}
