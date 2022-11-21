#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <cstring>

class InputHandler
{
private:
	int m_Arguments;
	bool m_Lowered, m_Uppered;
	std::vector<std::string> m_InputStrings;
public:
	explicit InputHandler(int& argc, char** argv) : m_Arguments(argc), m_Lowered(false), m_Uppered(false)
	{
		m_InputStrings.reserve(argc);
		for (int i = 0; i < m_Arguments; ++i)
			m_InputStrings.emplace_back(argv[i]);
	}


	bool contains(const char* element)
	{
		for (int i = 0; i < m_Arguments; ++i)
			if (strcmp(m_InputStrings[i].c_str(), element) == 0)
				return true;

		return false;
	}

	bool contains_l(const std::string& element)
	{
		std::string element_l = lower(element);

		if (m_Lowered)
		{
			for (int i = 0; i < m_Arguments; ++i)
				if (m_InputStrings[i] == element_l)
					return true;

			return false;
		}

		for (int i = 0; i < m_Arguments; ++i)
			if (lower(m_InputStrings[i]) == element_l)
				return true;

		return false;
	}

	bool contains_u(const std::string& element)
	{
		std::string element_u = upper(element);

		if (m_Uppered)
		{
			for (int i = 0; i < m_Arguments; ++i)
				if (m_InputStrings[i] == element_u)
					return true;

			return false;
		}

		for (int i = 0; i < m_Arguments; ++i)
			if (upper(m_InputStrings[i]) == element_u)
				return true;

		return false;
	}

	bool isNumber(const char* data)
	{
		try {
			std::stoi(data);
			return true;
		}
		catch (...) {
			return false;
		}
	}

	int location(const char* data)
	{
		for (int i = 0; i < m_Arguments; ++i)
			if (strcmp(m_InputStrings[i].c_str(), data) == 0)
				return i;
		return -1;
	}

	int location_l(const char* data)
	{
		std::string data_l = lower(data);
		if (m_Lowered)
		{
			for (int i = 0; i < m_Arguments; ++i)
				if (data_l == m_InputStrings[i])
					return i;
			return -1;
		}

		for (int i = 0; i < m_Arguments; ++i)
			if (data_l == lower(m_InputStrings[i]))
				return i;
		return -1;
	}

	int location_u(const char* data)
	{
		std::string data_u = upper(data);
		if (m_Uppered)
		{
			for (int i = 0; i < m_Arguments; ++i)
				if (data_u == m_InputStrings[i])
					return i;
			return -1;
		}

		for (int i = 0; i < m_Arguments; ++i)
			if (data_u == upper(m_InputStrings[i]))
				return i;
		return -1;
	}


	std::string lower(const std::string& data)
	{
		std::string result;

		for (int i = 0; i < data.size(); ++i)
			result += ::tolower(data[i]);

		return result;
	}

	std::string upper(const std::string& data)
	{
		std::string result;

		for (int i = 0; i < data.size(); ++i)
			result += ::toupper(data[i]);

		return result;
	}

	void upperAll()
	{
		if (m_Uppered)
			return;

		for (int i = 0; i < m_InputStrings.size(); ++i)
			for (int j = 0; j < m_InputStrings[i].size(); ++j)
				m_InputStrings[i][j] = std::toupper(m_InputStrings[i][j]);

		m_Uppered = true;
		m_Lowered = false;
	}

	void lowerAll()
	{
		if (m_Lowered)
			return;

		for (int i = 0; i < m_InputStrings.size(); ++i)
			for (int j = 0; j < m_InputStrings[i].size(); ++j)
				m_InputStrings[i][j] = std::tolower(m_InputStrings[i][j]);

		m_Uppered = false;
		m_Lowered = true;
	}


	inline std::string& operator[](const int& index) { return m_InputStrings[index]; }

	inline const int number(const std::string& data) const { return std::stoi(data); }
	inline const int number(const char* data) const { return std::stoi(data); }
	inline const int size() const { return m_Arguments; }
	inline const std::vector<std::string>& elements() const { return m_InputStrings; }
	inline const char* at(const int& index) const { return m_InputStrings[index].c_str(); }

	void print()
	{
		std::cout << '[';
		for (int i = 0; i < m_Arguments; ++i)
			std::cout << m_InputStrings[i] << ", ";
		std::cout << "]\n";
	}
};