#pragma once

#include <unordered_map>
#include <any>

class BlackBoard
{
public:

	BlackBoard() = default;

	template<typename T>
	static T& Get(std::any& value)
	{
		return std::any_cast<T&>(value);
	}

	template<typename T>
	T& Get(int32_t key)
	{
		return std::any_cast<T&>(m_BlackBoard[key]);
	}

	
	std::any& operator[](int32_t key)
	{
		return m_BlackBoard[key];
	}

private:

	std::unordered_map<int32_t, std::any> m_BlackBoard;
};

