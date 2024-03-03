#pragma once

#include <string>
#include <unordered_map>

class CVarReader
{
public:

	CVarReader() = default;

	void ParseArguments(int argc, char* argv[]);

	std::string ReadCVar(const std::string& cVar) const;

private:

	std::unordered_map<std::string, std::string> m_cVars;
};

