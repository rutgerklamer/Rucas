#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <Windows.h>

std::vector<int> m_cells;
std::vector<int> m_current_loop;
std::vector<int> m_current_if_statement;
std::vector<std::string> m_ruc_code;
std::vector<int> m_stack;
int m_cell;
int m_inp_pos;
std::string m_inp;
bool m_false_loop;
bool m_false_if_statement;
int m_found_brackets;

void runCode();
int getIntFromString(std::string s);
void wrapValue(int i);
void wrapPosition(int i);
void addSubStack(int x, int y);

int main(int argc, char const *argv[])
{
	std::ifstream fin("main.ruc", std::ios_base::in);
	m_inp = "BAAABBB";
	for (unsigned int i = 0; i < 30000; i++)
		m_cells.push_back(0);
	m_cell = 0;
	m_inp_pos = 0;
	m_false_loop = false;
	m_false_if_statement = false;
	std::string line;
	while (std::getline(fin, line))
	{
		m_ruc_code.push_back(line);
	}
	runCode();
	Sleep(5000);
	return 0;
}

void runCode()
{
	int i = 0;
	for (i = 0; i < m_ruc_code.size(); i++) {
		if (!m_false_loop && !m_false_if_statement) {
			if (m_ruc_code[i].find("JMP") != std::string::npos && m_ruc_code[i].find("JMPR") == std::string::npos && m_ruc_code[i].find("JMPL") == std::string::npos) {
				if (getIntFromString(m_ruc_code[i]) >= 0 && getIntFromString(m_ruc_code[i]) < 30000) {
					m_cell = getIntFromString(m_ruc_code[i]);
				}
			}
			else if (m_ruc_code[i].find("JMPR") != std::string::npos) {
				wrapPosition(getIntFromString(m_ruc_code[i]));
			}
			else if (m_ruc_code[i].find("JMPL") != std::string::npos) {
				wrapPosition(-getIntFromString(m_ruc_code[i]));
			}
			else if (m_ruc_code[i].find("ADD") != std::string::npos && m_ruc_code[i].find("ADDS") == std::string::npos) {
				wrapValue(getIntFromString(m_ruc_code[i]));
			}
			else if (m_ruc_code[i].find("SUB") != std::string::npos && m_ruc_code[i].find("SUBS") == std::string::npos) {
				wrapValue(-getIntFromString(m_ruc_code[i]));
			}
			else if (m_ruc_code[i].find("STLP") != std::string::npos) {
				(m_cells[m_cell] != 0 ? m_current_loop.push_back(i) : m_false_loop = true, m_found_brackets = 0);
			}
			else if (m_ruc_code[i].find("NDLP") != std::string::npos) {
				((int)m_cells[m_cell] == 0 ? m_current_loop.pop_back() : i = m_current_loop[m_current_loop.size() - 1]);
			}
			else if (m_ruc_code[i].find("PRNTA") != std::string::npos) {
				if (getIntFromString(m_ruc_code[i]) > 1) {
					for (int j = 0; j < getIntFromString(m_ruc_code[i]); j++) {
						std::cout << static_cast<char>(m_cells[m_cell]) << std::flush;
						wrapPosition(1);
					}
				}
				else {
					std::cout << static_cast<char>(m_cells[m_cell]) << std::flush;
				}
			}
			else if (m_ruc_code[i].find("PRNTD") != std::string::npos) {
				if (getIntFromString(m_ruc_code[i]) > 1) {
					for (int j = 0; j < getIntFromString(m_ruc_code[i]); j++) {
						std::cout << (m_cells[m_cell]) << std::flush;
						wrapPosition(1);
					}
				}
				else {
					std::cout << (m_cells[m_cell]) << std::flush;
				}
			}
			else if (m_ruc_code[i].find("CINP") != std::string::npos) {
				(m_inp_pos < m_inp.size() ? m_cells[m_cell] = static_cast<int>(m_inp[m_inp_pos]), m_inp_pos++ : m_cells[m_cell] = 0);
			}
			else if (m_ruc_code[i].find("PSHS") != std::string::npos) {
				m_stack.push_back(m_cells[m_cell]);
			}
			else if (m_ruc_code[i].find("CLRS") != std::string::npos) {
				m_stack.empty();
			}
			else if (m_ruc_code[i].find("RMVS") != std::string::npos) {
				if (m_stack.size() > 0)
					m_stack.erase(m_stack.begin());
			}
			else if (m_ruc_code[i].find("EQL TOPS") != std::string::npos) {
				m_cells[m_cell] = m_stack[m_stack.size() - 1];
			}
			else if (m_ruc_code[i].find("ADDS") != std::string::npos) {
				addSubStack(m_stack[m_stack.size() - 1], m_stack[m_stack.size() - 2]);
			}
			else if (m_ruc_code[i].find("SUBS") != std::string::npos) {
				addSubStack(m_stack[m_stack.size() - 1], m_stack[m_stack.size() - 2]);
			}
			else if (m_ruc_code[i].find("HALT") != std::string::npos) {
				i = m_ruc_code.size();
				std::cout << "\nTERMINATING PROGRAM" << std::endl;
			}
			else if (m_ruc_code[i].find("IF") != std::string::npos && m_ruc_code[i].find("ENDIF") == std::string::npos) {
				if (getIntFromString(m_ruc_code[i]) == 0) {
					m_false_if_statement = true;
					m_found_brackets = 0;
				}
			}
			else if (m_ruc_code[i].find("ENDIF") != std::string::npos) {
					m_found_brackets = 0;
			}
		}
		else if (m_ruc_code[i].find("STLP") != std::string::npos && m_false_loop) {
			m_found_brackets++;
			if (m_found_brackets > 0)
				m_false_loop = false;
		}
		else if (m_ruc_code[i].find("NDLP") != std::string::npos && m_false_loop) {
			m_found_brackets--;
		}
		else if (m_ruc_code[i].find("IF") != std::string::npos && m_false_if_statement && m_ruc_code[i].find("ENDIF") == std::string::npos) {
			m_found_brackets++;
			
		}
		else if (m_ruc_code[i].find("ENDIF") != std::string::npos && m_false_if_statement) {
			m_found_brackets--;
			if (m_found_brackets < 0)
				m_false_if_statement = false;
		}
	}
}

int getIntFromString(std::string s)
{
	for (int i = 0; i < s.size(); i++) {
		if ((int)s[i] >= 47 && (int)s[i] <= 57)
			s.erase(s.begin(), s.begin() + i);
		else if (i >= s.size() - 1) {
			if (s.find("$CELL") != std::string::npos)
				return m_cells[m_cell];
			else if (s.find("$STACK") != std::string::npos) {
				if (m_stack.size() > 0)
					return m_stack[m_stack.size() - 1];
			} else
				return 1;
		}
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
			(m_cell > 0 ? m_cell-- : m_cell = m_cells.size() - 1
				);
}

void addSubStack(int x, int y)
{
	if (x + y > 255 && y > 0) {
		for (int i = 0; i < y; i++)
			(x < 255 ? x++ : x = 0);
		m_stack.push_back(x);
	}
	else if (x + y < 0) {
		for (int k = 0; k > y; k--)
			(x > 0 ? x-- : x = 255);
		m_stack.push_back(x);
	}
	else {
		m_stack.push_back(x + y);
	}
}
