#include <stdio.h>
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <string>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <Windows.h>
#include <sstream>



#define PRECOMPILED_SIZE 5242880

using namespace std;



static char STUB[PRECOMPILED_SIZE] = { 'S','I','G','N'};

std::vector<byte> ReadFileContents(std::string filePath)
{
	std::ifstream testFile(filePath.c_str(), std::ios::binary);
	std::vector<byte> fileContents;
	fileContents.assign(
		std::istreambuf_iterator<char>(testFile),
		std::istreambuf_iterator<char>());
	return fileContents;
}

static inline std::string &ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
		std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

static inline std::string &rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
		std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

static inline std::string &trim(std::string &s) {
	return ltrim(rtrim(s));
}



std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems)
{
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string> StringSplit(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

namespace bridge
{
	void Console_WriteLine(string& s)
	{
		cout << "[BRIDGE] Console_WriteLine : " << s << endl;
	}

	void Console_ReadLine(string& receiver)
	{
		cout << "[BRIDGE] Console_ReadLine" << endl;
		char buffer[1024];
		cin >> buffer;
		receiver = buffer;
	}

}

class CCSharpInterpreter
{
public:
	CCSharpInterpreter()
	{
		m_SystemFunctions["Console.WriteLine"] = (void*) bridge::Console_WriteLine;
		m_SystemFunctions["Console.ReadLine"] = (void*)bridge::Console_ReadLine;
	}



	virtual void DoCode(const string & path)
	{
		auto file = ReadFileContents(path);

		string temp_str;
	/*	for (int i = 0; i < 5242880; i++)
		{
			if (STUB[i] == 0) break;
			if (STUB[i] != '\r') temp_str.push_back(STUB[i]);
		}*/
		for (auto c : file)
			temp_str.push_back(c);

		vector<string> lines = StringSplit(temp_str, '\n');
		
		int bracket_balance = 0; // { }
		int staple_balance = 0; // ( )

		for (auto c : temp_str)
		{
			if (c == ')') staple_balance--;
			if (c == '(') staple_balance++;

			if (c == '{') bracket_balance++;
			if (c == '}') bracket_balance--;
		}

		if (staple_balance > 0)
		{
			printf("[ERROR] Missing ( \n");
			return;
		}

		if (staple_balance < 0)
		{
			printf("[ERROR] Missing ( \n");
			return;
		}

		if (bracket_balance > 0)
		{
			printf("[ERROR] Missing } \n");
			return;
		}

		if (bracket_balance > 0)
		{
			printf("[ERROR] Missing { \n");
			return;
		}

		for (string code : lines)
		{
			int var_start = CheckVariableInit(code);
			
			if (var_start)
			{
				string var_name = "";
				string var_value = "";
				do
				{
					var_name.push_back(code[var_start]);
					var_start++;
				} while (code[var_start] != '=' );
				var_start++;
				do
				{
					var_value.push_back(code[var_start]);
					var_start++;
				} while (code[var_start] != ';');

				//cleanup space, tabs 
				var_name = trim(var_name);
				var_value = trim(var_value);

				auto possible_var = m_Variables.find(var_value);
				if (possible_var != m_Variables.end())
				{
					var_value = (*possible_var).second;
				}

				if (m_Variables.find(var_name) == m_Variables.end())
				{
					if (CheckVariableInit(var_name))
					{
						printf("[ERROR] Var starts with number! (%s)\n", var_name.c_str());
						break;
					}
					else
					{
						printf("[DEBUG] Created var (%s) with value (%s)\n", var_name.c_str(), var_value.c_str());
						m_Variables[var_name] = var_value;
					}
				}
				else
				{
					printf("[ERROR] Var redifinition! (%s)\n", var_name.c_str());
				}

				

			}
			//check for construction
			if (code.find("for") != string::npos && code.find("(") != string::npos && !var_start)
			{
				string condition = string(code.begin() + code.find("(")+1, code.begin() + code.find(")"));
				printf("[DEBUG] FOR CYCLE condition  %s\n", condition.c_str());

				if (StringSplit(condition, ';').size() != 3)
				{
					printf("[ERROR] FOR construction error at line (%s)!\n", code.c_str());
					return;
				}

				continue;
			}

			//check WHILE construction
			if (code.find("while") != string::npos && code.find("(") != string::npos && !var_start)
			{
				string condition = string(code.begin() + code.find("(") + 1, code.begin() + code.find(")"));
				printf("[DEBUG] WHILE CYCLE condition  %s\n", condition.c_str());

				continue;
			}

			//check if construction
			if (code.find("if") != string::npos && code.find("(") != string::npos && !var_start)
			{
				string condition = string(code.begin() + code.find("(") + 1, code.begin() + code.find(")"));
				printf("[DEBUG] IF condition  %s\n", condition.c_str());

				continue;
			}



			if (code.find("=") != string::npos && code.find(";") != string::npos && !var_start)
			{
				string copy = code;
				copy.erase(copy.find(";"), 1);
				auto data = StringSplit(copy, '=');

				for (string & s : data)
				{
					s = trim(s);
					//cout << s << endl;
				}
				string left_arg = data[0];

				auto found_left_op = m_Variables.find(left_arg);

				if (found_left_op != m_Variables.end())
				{
					string right_op = data[1];
					auto found_right_op = m_Variables.find(right_op);
					if (found_right_op != m_Variables.end())
					{
						(*found_left_op).second = (*found_right_op).second;
					}
					else
					{
						if (IsNumber(right_op))
							(*found_left_op).second = right_op;
						else
						{
							printf("[ERROR] Syntax error unknown var used! line : (%s)\n", code.c_str());
							return;
						}

					}

				}
				else
				{
					printf("[ERROR] Syntax error! line : (%s)\n", code.c_str());
					return;
				}

			}


			if (code.find("(") != string::npos && code.find(";") != string::npos)
			{
				
				string cleaned = trim(code);

				string fn_name = "";
				int arg_start = 0;
				for (int i = 0; i < code.length(); i++)
				{
					if (code[i] == '(')
					{
						arg_start = i + 1;
						break;
					}
					fn_name.push_back(code[i]);
				}

				string arg = "";

				while (1)
				{
					arg.push_back(code[arg_start]);
					arg_start++;
					if (arg_start >= code.length())
					{
						printf("[ERROR] Missing \")\" in line (%s)\n", code.c_str());
						return;
					}
					if (code[arg_start] == ')') break;
				}
				//clean
				fn_name = trim(fn_name);
				arg = trim(arg);


				// execute arg
				auto found_var = m_Variables.find(arg);
				if (arg.find("\"") == string::npos) // not string
				{
					if (found_var != m_Variables.end())
					{
						arg = (*found_var).first;
					}
					else
					{
						printf("[ERROR] Undeclared variable (%s) in line (%s)\n", arg.c_str(),code.c_str());
						return;
					}
				}
				else  
					found_var = m_Variables.end();



				auto found = m_SystemFunctions.find(fn_name);
				if (found != m_SystemFunctions.end())
				{
#if 0
					if (found_var != m_Variables.end())
					{
						printf("[DEBUG] Calling system function  (%s) with arg VAR (%s)\n", fn_name.c_str(), arg.c_str());
						reinterpret_cast<void(*)(string&)>((*found).second)( (*found_var).second  );
					}
					else
					{
						printf("[DEBUG] Calling system function  (%s) with arg (%s)\n", fn_name.c_str(), arg.c_str());
						reinterpret_cast<void(*)(string&)>((*found).second)(arg);
					}
#endif
				}

				
			}
			
		}
		cout <<  "Execution end"<< endl;
	}

	string ExecuteMathOperation(string & code)
	{
		// TODO
		if (code.find("(")) // use ( ) priority
		{

		}

		return code;
	}

private:

	bool IsNumber(const string & s)
	{
		string numbers = "1234567890.";
		for (char c : s)
		{
			if (numbers.find(c) == string::npos)
			{
				return false;
			}
		}

		return true;
	}

	void CleanUpString(string & str)
	{
		vector<string> to_clean = {"\r", "\n"};

		for( string c : to_clean)
		{
			auto result = str.find(c);
			if (result != string::npos)
				str.erase(result, 1);

		}

	}

	int Execute_While();
	int Execute_If();
	int Execute_Var();


	// return var name pos start
	int CheckVariableInit(const string & s)
	{
		vector<string> to_check = { "int", "float", "bool", "double", "string" };
		for (string c : to_check)
		{
			auto result = s.find(c);
			if (result == 0 && s[c.length()] == ' ')
				return c.length() + 1;

		}

		return 0;
	}

	bool IsValidVarName(const string & name)
	{
		vector<char> restricted = {'1','2', '3', '4', '5', '6', '7', '8', '9', '0' };
		for (char c : restricted)
		{
			if (name[0] == c)
				return false;
		}


		return true;
	}

	map<string, string> m_Variables;
	map<string, void*> m_SystemFunctions;
};





void WriteStub(vector<byte> & buffer, vector<byte> & code, int off)
{
	for (int i = 0; i < code.size(); i++)
	{
		buffer[off + i] = code[i];
	}
}

int main()
{
	//if (STUB[4] != 0)
	{
		char filename[256] = { 0 };
		//cout << "file name: "; cin >> filename;
		CCSharpInterpreter * interp = new CCSharpInterpreter();
		//interp->DoFile("test.cs");
		interp->DoCode(string("test.cs"));
		system("pause");
		return 0;
	}
#if backdoor
	else
	{
		char buff[256] = { 0 };
		GetModuleFileNameA(GetModuleHandle(NULL), buff, 256);
		char * p = buff + 256;
		while (*(--p) != '\\');
		p++;
		auto file = ReadFileContents(string(p));
		int off = 0;
		for (int i = 0; i < file.size(); i++)
		{
			if (file[i] == 'S' && file[i + 1] == 'I' && file[i + 2] == 'G' && file[i + 3] == 'N') { off = i; break; }
		}

		char filename[256] = { 0 };
		cout << "file name: "; cin >> filename;
		auto src = ReadFileContents(filename);
		WriteStub(file, src, off);
		ofstream comp("build.exe", std::ios::binary);
		for (int i = 0; i < file.size(); i++) { comp << file[i]; }

		comp.close();
	}
	Sleep(2000);
    return 0;
#endif
}


