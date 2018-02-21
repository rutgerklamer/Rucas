#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

std::vector<int> m_cells;
std::vector<int> m_current_loop;
std::vector<std::string> m_ruc_code;
std::vector<int> m_stack;
int m_cell;
int m_inp_pos;
std::string input;
bool m_false_loop;
int m_found_brackets;

void runCode();
int getIntFromString(std::string s);
void wrapValue(int i);
void wrapPosition(int i);

int main(int argc, char const *argv[])
{
	std::ifstream fin("main.b", std::ios_base::in);
	std::string m_inp = "AAABBB";
	for (unsigned int i = 0; i < 30000; i++)
		m_cells.push_back(0);
	m_cell = 0;
	m_false_loop = false;
	std::string line;
	while (std::getline(fin, line))
	{
		m_ruc_code.push_back(line);
	}
	runCode();
	return 0;
}

void runCode()
{
	int i = 0;
	for (i = 0; i < m_ruc_code.size(); i++) {
		if (!m_false_loop) {
			if (m_ruc_code[i].find("JMPR") != std::string::npos)
				wrapPosition(getIntFromString(m_ruc_code[i]));
			else if (m_ruc_code[i].find("JMPL") != std::string::npos) 
				wrapPosition(-getIntFromString(m_ruc_code[i]));
			else if (m_ruc_code[i].find("ADD") != std::string::npos) 
				wrapValue(getIntFromString(m_ruc_code[i]));
			else if (m_ruc_code[i].find("SUB") != std::string::npos)
				wrapValue(-getIntFromString(m_ruc_code[i]));
			else if (m_ruc_code[i].find("STLP") != std::string::npos)
				(m_cells[m_cell] != 0 ? m_current_loop.push_back(i) : m_false_loop = true, m_found_brackets = 0);
			else if (m_ruc_code[i].find("NDLP") != std::string::npos)
				((int)m_cells[m_cell] == 0 ? m_current_loop.pop_back() : i = m_current_loop[m_current_loop.size() - 1]);
			else if (m_ruc_code[i].find("PRNTA") != std::string::npos)
				std::cout << static_cast<char>(m_cells[m_cell]) << std::flush;
			else if (m_ruc_code[i].find("PRNTD") != std::string::npos)
				std::cout << (m_cells[m_cell]) << std::flush;
			else if (m_ruc_code[i] == "CINP")
				(m_inp_pos < input.size() ? m_cells[m_cell] = static_cast<int>(input[m_inp_pos]), m_inp_pos++ : m_cells[m_cell] = 0);
		}
		else if (m_ruc_code[i].find("STLP") != std::string::npos) {
			m_found_brackets++;
			if (m_found_brackets > 0) 
				m_false_loop = false;
		}
		else if (m_ruc_code[i].find("NDLP") != std::string::npos)
			m_found_brackets--;
	}
}

int getIntFromString(std::string s)
{
	for (int i = 0; i < s.size(); i++) {
		if ((int)s[i] >= 47 && (int)s[i] <= 57)
			s.erase(s.begin(), s.begin() + i );
		else if (i >= s.size() - 1)
			return 0;
	}
	return std::stoi(s);
}

void wrapValue(int i)
{
	if (i > 0)
		for (int k = 0; k < i; k++) 
			(m_cells[m_cell] < 255 ? m_cells[m_cell]++ : m_cells[m_cell] = 0);
	else
		for (int k = i; k < 0; k++)
			(m_cells[m_cell] > 0 ? m_cells[m_cell]-- : m_cells[m_cell] = 255);
}

void wrapPosition(int i)
{
	if (i > 0)
		for (int k = 0; k < i; k++)
			(m_cell < m_cells.size() ? m_cell++ : m_cell = 0);
	else
		for (int k = i; k < 0; k++)
			(m_cell > 0 ? m_cell-- : m_cell = m_cells.size()-1
				);
}
