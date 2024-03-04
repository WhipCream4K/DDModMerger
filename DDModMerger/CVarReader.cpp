#include "CVarReader.h"
#include <iostream>
#include <stdexcept>

void CVarReader::ParseArguments(int argc, char* argv[])
{
	for (int i = 1; i < argc; ++i)
	{
		std::string arg{ argv[i] };

		try
		{
			if (arg[0] == '-')
			{
				std::string cVar{ arg };
				std::string value{ argv[++i] };
				m_cVars[cVar] = value;
			}
			else
			{
				throw std::invalid_argument("Invalid argument: " + arg);
			}
		}
		catch (const std::exception& e )
		{
			std::cerr << e.what() << '\n';
		}

	}
}

std::string CVarReader::ReadCVar(const std::string& cVar) const
{
	try
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
	catch (const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}

}

bool CVarReader::CheckArgs() const
{
	return m_cVars.size() > 0;
}
