#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>

const int TAPE_SIZE = 30000;

struct Instruction
{
	std::string op;
	std::string arg;
	int line;
	int match;
};

std::vector<int> m_cells(TAPE_SIZE, 0);
std::vector<int> m_stack;
std::vector<Instruction> m_program;
int m_cell = 0;

bool loadProgram(const char* path);
bool matchBlocks();
void runCode();
int getValue(const Instruction& ins, int default_value);
int wrapValue(int v);
void movePointer(int n);
int stackTop(int depth);
void fail(const Instruction& ins, const std::string& message);

int main(int argc, char const *argv[])
{
	if (argc < 2) {
		std::cerr << "usage: rucas <file.ruc>" << std::endl;
		return 1;
	}
	if (!loadProgram(argv[1]) || !matchBlocks())
		return 1;
	runCode();
	return 0;
}

bool loadProgram(const char* path)
{
	std::ifstream fin(path);
	if (!fin) {
		std::cerr << "error: cannot open " << path << std::endl;
		return false;
	}
	std::string line;
	int line_number = 0;
	while (std::getline(fin, line)) {
		line_number++;
		std::istringstream words(line);
		Instruction ins;
		ins.line = line_number;
		ins.match = -1;
		if (!(words >> ins.op) || ins.op[0] == '#')
			continue;
		words >> ins.arg;
		if ((ins.op == "IF" && ins.arg == "EQL") || (ins.op == "EQL" && ins.arg == "TOPS")) {
			ins.op += " " + ins.arg;
			ins.arg.clear();
			words >> ins.arg;
		}
		std::string extra;
		if (words >> extra) {
			std::cerr << "error: line " << line_number << ": unexpected '" << extra << "'" << std::endl;
			return false;
		}
		m_program.push_back(ins);
	}
	return true;
}

bool matchBlocks()
{
	std::vector<int> open;
	for (int i = 0; i < (int)m_program.size(); i++) {
		const std::string& op = m_program[i].op;
		if (op == "STLP" || op == "IF" || op == "IF EQL") {
			open.push_back(i);
		}
		else if (op == "NDLP" || op == "ENDIF") {
			std::string expected = (op == "NDLP" ? "STLP" : "IF");
			if (open.empty() || m_program[open.back()].op.compare(0, expected.size(), expected) != 0) {
				std::cerr << "error: line " << m_program[i].line << ": " << op << " without matching " << expected << std::endl;
				return false;
			}
			m_program[i].match = open.back();
			m_program[open.back()].match = i;
			open.pop_back();
		}
	}
	if (!open.empty()) {
		std::cerr << "error: line " << m_program[open.back()].line << ": " << m_program[open.back()].op << " is never closed" << std::endl;
		return false;
	}
	return true;
}

void runCode()
{
	for (int i = 0; i < (int)m_program.size(); i++) {
		const Instruction& ins = m_program[i];
		const std::string& op = ins.op;
		int& cell = m_cells[m_cell];

		if (op == "ADD") {
			cell = wrapValue(cell + getValue(ins, 1));
		}
		else if (op == "SUB") {
			cell = wrapValue(cell - getValue(ins, 1));
		}
		else if (op == "JMPR") {
			movePointer(getValue(ins, 1));
		}
		else if (op == "JMPL") {
			movePointer(-getValue(ins, 1));
		}
		else if (op == "JMP") {
			if (ins.arg.empty())
				fail(ins, "JMP needs a cell number");
			int target = getValue(ins, 0);
			if (target < 0 || target >= TAPE_SIZE)
				fail(ins, "cell " + std::to_string(target) + " is outside the tape");
			m_cell = target;
		}
		else if (op == "PRNTA" || op == "PRNTD") {
			int count = getValue(ins, 1);
			for (int j = 0; j < count; j++) {
				int value = m_cells[(m_cell + j) % TAPE_SIZE];
				if (op == "PRNTA")
					std::cout << static_cast<char>(value);
				else
					std::cout << (j > 0 ? " " : "") << value;
			}
			std::cout << std::flush;
		}
		else if (op == "CINP") {
			int c = std::cin.get();
			cell = (c == EOF ? 0 : wrapValue(c));
		}
		else if (op == "STLP") {
			if (cell == 0)
				i = ins.match;
		}
		else if (op == "NDLP") {
			if (cell != 0)
				i = ins.match;
		}
		else if (op == "IF") {
			if (getValue(ins, cell) == 0)
				i = ins.match;
		}
		else if (op == "IF EQL") {
			if (ins.arg.empty())
				fail(ins, "IF EQL needs a value to compare with");
			if (getValue(ins, 0) != cell)
				i = ins.match;
		}
		else if (op == "ENDIF") {
		}
		else if (op == "PSHS") {
			m_stack.push_back(cell);
		}
		else if (op == "CLRS") {
			m_stack.clear();
		}
		else if (op == "RMVS") {
			if (!m_stack.empty())
				m_stack.erase(m_stack.begin());
		}
		else if (op == "EQL TOPS") {
			cell = stackTop(0);
		}
		else if (op == "ADDS") {
			m_stack.push_back(wrapValue(stackTop(0) + stackTop(1)));
		}
		else if (op == "SUBS") {
			m_stack.push_back(wrapValue(stackTop(0) - stackTop(1)));
		}
		else if (op == "HALT") {
			return;
		}
		else {
			fail(ins, "unknown instruction '" + op + "'");
		}
	}
}

int getValue(const Instruction& ins, int default_value)
{
	const std::string& arg = ins.arg;
	if (arg.empty())
		return default_value;
	if (arg == "$CELL")
		return m_cells[m_cell];
	if (arg == "$STACK")
		return stackTop(0);
	size_t used = 0;
	int value = 0;
	try {
		value = std::stoi(arg, &used);
	}
	catch (...) {
		used = 0;
	}
	if (used != arg.size() || value < 0)
		fail(ins, "'" + arg + "' is not a valid argument (expected N, $CELL or $STACK)");
	return value;
}

int wrapValue(int v)
{
	return ((v % 256) + 256) % 256;
}

void movePointer(int n)
{
	m_cell = ((m_cell + n) % TAPE_SIZE + TAPE_SIZE) % TAPE_SIZE;
}

int stackTop(int depth)
{
	if ((int)m_stack.size() <= depth)
		return 0;
	return m_stack[m_stack.size() - 1 - depth];
}

void fail(const Instruction& ins, const std::string& message)
{
	std::cout << std::flush;
	std::cerr << "\nerror: line " << ins.line << ": " << message << std::endl;
	std::exit(1);
}
