#include "CVarReader.h"
#include <stdexcept>

void CVarReader::ParseArguments(int argc, char* argv[])
{
	for (int i = 1; i < argc; ++i)
	{
		std::string arg{ argv[i] };
		if (arg[0] == '-')
		{
			std::string cVar{ arg.substr(1) };
			std::string value{ argv[++i] };
			m_cVars[cVar] = value;
		}
		else
		{
			throw std::invalid_argument("Invalid argument: " + arg);
		}
	}
}

std::string CVarReader::ReadCVar(const std::string& cVar) const
{
	auto it = m_cVars.find(cVar);
	if (it != m_cVars.end())
	{
		return it->second;
	}
	else
	{
		throw std::invalid_argument("Error: CVar not found: " + cVar);
	}
}
