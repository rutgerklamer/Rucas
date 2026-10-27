// Runs a Rucas program; straight runs are worked out once as blocks, code outside any loop runs as it is read.
//   rucas <file.ruc> [input file]
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
	PSHS, CLRS, RMVS, EQL_TOPS, ADDS, SUBS, MULS, DIVS, MODS, ELSE, IF_LESS, IF_MORE, BLOCK, BLOCK_ONCE
};

enum Arg { NONE, NUMBER, CELL, STACK };

// what a BLOCK leaves on the stack; nothing changed, empty, or a ladder x, x, 2x, 3x, 5x... of this height
const int UNTOUCHED = -2, EMPTY = -1;

struct Instruction
{
	Op op;
	Arg arg;
	uint32_t n;
	int line;
	int match;
	int from, to;
	long long move;
	int ladder;
};

// one step of a BLOCK, on the cell at off (0 to TAPE_SIZE - 1) from where the block starts; v is the value PSHS took
enum ActionKind { A_ADD, A_MULADD, A_SET, A_DOUBLE, A_SETV, A_TAKE };

struct Action
{
	ActionKind kind;
	long long off;
	uint32_t k;
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
std::vector<Action> m_actions;
std::vector<uint32_t> m_ladder = {1, 1};
long long m_cell = 0;
int m_lazy_height = EMPTY;
uint32_t m_lazy_value = 0;
FILE* m_input = stdin;
bool m_started = false;

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

bool runFile(const char* path, bool execute);
bool readLine(Reader& r, std::string& text);
int parseLine(const std::string& text, int line, Instruction& ins);
bool checkOrder(const Instruction& ins, std::vector<Instruction>& open);
bool runSegment();
bool parseArg(const std::string& text, Instruction& ins);
void fuseBlocks();
bool endsAtZero(const Instruction& block);
bool addToBlock(const Instruction& ins, Instruction& block, int& state);
void addAction(const Instruction& block, ActionKind kind, long long off, uint32_t k);
void matchBlocks();
bool runCode();
uint32_t getValue(const Instruction& ins);
void movePointer(long long n);
void materialize();
uint32_t stackTop(size_t depth);
void fail(const Instruction& ins, const std::string& message);

int main(int argc, char const *argv[])
{
	if (argc < 2 || argc > 3) {
		std::cerr << "usage: rucas <file.ruc> [input file]" << std::endl;
		return 1;
	}
#ifdef _WIN32
	// bytes in and out as they are, without turning \n into \r\n
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	if (argc == 3) {
		m_input = std::fopen(argv[2], "rb");
		if (!m_input) {
			std::cerr << "error: cannot open " << argv[2] << std::endl;
			return 1;
		}
	}
	// the whole file is checked before any of it runs
	if (!runFile(argv[1], false))
		return 1;
	if (!runFile(argv[1], true))
		return 1;
	std::cout << std::flush;
	return 0;
}

bool runFile(const char* path, bool execute)
{
	Reader r = {std::fopen(path, "rb"), std::vector<char>(PIECE), 0, 0, 0};
	if (!r.file) {
		std::cerr << "error: cannot open " << path << std::endl;
		return false;
	}
	std::vector<Instruction> open;
	std::string text;
	Instruction ins;
	m_started = false;
	bool going = true;
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
	if (going && !open.empty()) {
		std::cerr << "error: line " << open.back().line << ": " << OP_NAMES[open.back().op] << " is never closed" << std::endl;
		return false;
	}
	if (going && execute)
		runSegment();
	std::fclose(r.file);
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
		std::cerr << "error: line " << line << ": unexpected '" << extra << "'" << std::endl;
		return -1;
	}

	// BITS sets up the machine, so it comes before the first instruction
	if (op == "BITS") {
		if (m_started) {
			std::cerr << "error: line " << line << ": BITS must come before the first instruction" << std::endl;
			return -1;
		}
		Instruction value;
		if (!parseArg(arg, value) || value.arg != NUMBER || (value.n != 8 && value.n != 16 && value.n != 32)) {
			std::cerr << "error: line " << line << ": BITS takes 8, 16 or 32" << std::endl;
			return -1;
		}
		m_bits = (int)value.n;
		m_mask = (m_bits == 32 ? 0xFFFFFFFFu : (1u << m_bits) - 1);
		return 0;
	}

	ins.line = line;
	ins.match = -1;
	int k = 0;
	while (k < OP_COUNT && op != OP_NAMES[k])
		k++;
	if (k == OP_COUNT) {
		std::cerr << "error: line " << line << ": unknown instruction '" << op << "'" << std::endl;
		return -1;
	}
	ins.op = (Op)k;
	if (!parseArg(arg, ins)) {
		std::cerr << "error: line " << line << ": '" << arg << "' is not a valid argument (expected N, $CELL or $STACK)" << std::endl;
		return -1;
	}
	m_started = true;
	return 1;
}

bool checkOrder(const Instruction& ins, std::vector<Instruction>& open)
{
	Op op = ins.op;
	if ((op == JMP || op == IF_EQL || op == IF_LESS || op == IF_MORE) && ins.arg == NONE) {
		std::cerr << "error: line " << ins.line << ": " << OP_NAMES[op] << (op == JMP ? " needs a cell number" : " needs a value to compare with") << std::endl;
		return false;
	}
	// an ELSE takes the place of its IF, so a second ELSE or an NDLP finds no IF to belong to
	bool in_if = !open.empty() && open.back().op != STLP;
	if (op == STLP || op == IF || op == IF_EQL || op == IF_LESS || op == IF_MORE)
		open.push_back(ins);
	else if (op == ELSE) {
		if (!in_if || open.back().op == ELSE) {
			std::cerr << "error: line " << ins.line << ": ELSE without matching IF" << std::endl;
			return false;
		}
		open.back() = ins;
	}
	else if (op == NDLP || op == ENDIF) {
		bool loop = (op == NDLP);
		if (open.empty() || in_if == loop) {
			std::cerr << "error: line " << ins.line << ": " << OP_NAMES[op] << " without matching " << (loop ? "STLP" : "IF") << std::endl;
			return false;
		}
		open.pop_back();
	}
	return true;
}

// false once the program has stopped at HALT
bool runSegment()
{
	fuseBlocks();
	matchBlocks();
	bool going = runCode();
	m_program.clear();
	m_actions.clear();
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

// runs of moves, adds and stack steps become one BLOCK, worked out once here instead of on every pass
void fuseBlocks()
{
	std::vector<Instruction> fused;
	size_t i = 0;
	while (i < m_program.size()) {
		Instruction block = m_program[i];
		block.op = BLOCK;
		block.from = (int)m_actions.size();
		block.move = 0;
		int state = UNTOUCHED;
		size_t j = i;
		while (j < m_program.size() && addToBlock(m_program[j], block, state))
			j++;
		if (j - i >= 2) {
			block.to = (int)m_actions.size();
			block.ladder = state;
			fused.push_back(block);
			i = j;
		}
		else {
			m_actions.resize(block.from);
			fused.push_back(m_program[i]);
			i++;
		}
	}

	// STLP, a block that ends with its own cell at 0, NDLP; runs at most once, so it is one BLOCK_ONCE
	m_program.clear();
	for (size_t k = 0; k < fused.size(); k++) {
		if (fused[k].op == STLP && k + 2 < fused.size() && fused[k + 1].op == BLOCK && fused[k + 2].op == NDLP && endsAtZero(fused[k + 1])) {
			fused[k + 1].op = BLOCK_ONCE;
			k++;
			m_program.push_back(fused[k]);
			k++;
		}
		else
			m_program.push_back(fused[k]);
	}
}

bool endsAtZero(const Instruction& block)
{
	if (block.move % TAPE_SIZE != 0)
		return false;
	for (int a = block.to - 1; a >= block.from; a--)
		if (m_actions[a].off == 0)
			return (m_actions[a].kind == A_SET || m_actions[a].kind == A_TAKE) && m_actions[a].k == 0;
	return false;
}

bool addToBlock(const Instruction& ins, Instruction& block, int& state)
{
	long long& at = block.move;
	bool fixed = (ins.arg == NONE || ins.arg == NUMBER);
	switch (ins.op) {
	case JMPR:
	case JMPL:
		if (!fixed)
			return false;
		at += (ins.op == JMPR ? 1 : -1) * (long long)ins.n;
		return true;
	case ADD:
	case SUB: {
		bool add = (ins.op == ADD);
		if (fixed)
			addAction(block, A_ADD, at, add ? ins.n : 0u - ins.n);
		else if (ins.arg == CELL)
			addAction(block, add ? A_DOUBLE : A_SET, at, 0);
		else if (state >= 0)
			addAction(block, A_MULADD, at, add ? m_ladder[state] : 0u - m_ladder[state]);
		else if (state != EMPTY)
			return false;
		return true;
	}
	case CLRS:
		state = EMPTY;
		return true;
	case PSHS:
		if (state != EMPTY)
			return false;
		addAction(block, A_SETV, at, 0);
		state = 0;
		return true;
	case ADDS:
		if (state < 0)
			return false;
		state++;
		while ((int)m_ladder.size() <= state)
			m_ladder.push_back(m_ladder[m_ladder.size() - 1] + m_ladder[m_ladder.size() - 2]);
		return true;
	case EQL_TOPS:
		if (state == UNTOUCHED)
			return false;
		addAction(block, A_SET, at, state >= 0 ? m_ladder[state] : 0);
		return true;
	default:
		return false;
	}
}

void addAction(const Instruction& block, ActionKind kind, long long off, uint32_t k)
{
	off = (off % TAPE_SIZE + TAPE_SIZE) % TAPE_SIZE;
	if (kind == A_SET && k == 0 && (int)m_actions.size() > block.from && m_actions.back().kind == A_SETV && m_actions.back().off == off) {
		m_actions.back().kind = A_TAKE;
		return;
	}
	// an add joins an earlier add to the same cell when nothing in between touches that cell (or v)
	if (kind == A_ADD || kind == A_MULADD) {
		for (int a = (int)m_actions.size() - 1; a >= block.from && a >= (int)m_actions.size() - 64; a--) {
			Action& earlier = m_actions[a];
			if (kind == A_MULADD && (earlier.kind == A_SETV || earlier.kind == A_TAKE))
				break;
			if (earlier.off != off)
				continue;
			if (earlier.kind == kind) {
				earlier.k += k;
				return;
			}
			break;
		}
	}
	m_actions.push_back({kind, off, k});
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
		case BLOCK_ONCE:
			if (cell == 0)
				break;
			// fall through
		case BLOCK: {
			const Action* const actions = m_actions.data();
			uint32_t v = 0;
			for (int a = ins.from; a < ins.to; a++) {
				const Action& act = actions[a];
				long long at = m_cell + act.off;
				if (at >= TAPE_SIZE)
					at -= TAPE_SIZE;
				uint32_t& c = tape[at];
				switch (act.kind) {
				case A_ADD: c = (c + act.k) & mask; break;
				case A_MULADD: c = (c + act.k * v) & mask; break;
				case A_SET: c = (act.k * v) & mask; break;
				case A_DOUBLE: c = (c + c) & mask; break;
				case A_SETV: v = c; break;
				case A_TAKE: v = c; c = 0; break;
				}
			}
			movePointer(ins.move);
			if (ins.ladder != UNTOUCHED) {
				m_stack.clear();
				m_lazy_height = ins.ladder;
				m_lazy_value = v;
			}
			break;
		}
		case PSHS:
			materialize();
			m_stack.push_back(cell);
			break;
		case CLRS:
			m_stack.clear();
			m_lazy_height = EMPTY;
			break;
		case RMVS:
			materialize();
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

// a BLOCK leaves its ladder unwritten; it is pushed only when something reads the stack
void materialize()
{
	for (int j = 0; j <= m_lazy_height; j++)
		m_stack.push_back((m_ladder[j] * m_lazy_value) & m_mask);
	m_lazy_height = EMPTY;
}

uint32_t stackTop(size_t depth)
{
	materialize();
	if (m_stack.size() <= depth)
		return 0;
	return m_stack[m_stack.size() - 1 - depth];
}

void fail(const Instruction& ins, const std::string& message)
{
	std::cout << std::flush;
	std::cerr << "\nerror: line " << ins.line << ": " << message << std::endl;
	std::exit(1);
}
