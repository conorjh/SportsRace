#ifndef TESTHARNESS_HPP
#define TESTHARNESS_HPP

#include <exception>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace Test
{
	class Failure : public std::runtime_error
	{
	public:
		explicit Failure(const std::string& Message)
			: std::runtime_error(Message)
		{
		}
	};

	struct Case
	{
		std::string Name;
		void (*Body)();
	};

	inline std::vector<Case>& Cases()
	{
		static std::vector<Case> Tests;
		return Tests;
	}

	struct Registrar
	{
		Registrar(const std::string& Name, void (*Body)())
		{
			Cases().push_back({ Name, Body });
		}
	};

	inline void Fail(const char* Expression, const char* File, int Line)
	{
		std::ostringstream Stream;
		Stream << File << ":" << Line << ": assertion failed: " << Expression;
		throw Failure(Stream.str());
	}

	template <typename TExpected, typename TActual>
	void AreEqual(const TExpected& Expected, const TActual& Actual, const char* ExpectedExpression, const char* ActualExpression, const char* File, int Line)
	{
		if (!(Expected == Actual))
		{
			std::ostringstream Stream;
			Stream << File << ":" << Line << ": expected " << ExpectedExpression << " == " << ActualExpression
				<< ", got " << Expected << " and " << Actual;
			throw Failure(Stream.str());
		}
	}

	inline int RunAll()
	{
		int Failed = 0;
		for (const auto& TestCase : Cases())
		{
			try
			{
				TestCase.Body();
				std::cout << "[PASS] " << TestCase.Name << '\n';
			}
			catch (const std::exception& Ex)
			{
				Failed++;
				std::cerr << "[FAIL] " << TestCase.Name << " - " << Ex.what() << '\n';
			}
		}

		std::cout << Cases().size() - Failed << "/" << Cases().size() << " tests passed\n";
		return Failed == 0 ? 0 : 1;
	}
}

#define TEST_CASE(Name) \
	static void Name(); \
	static Test::Registrar Name##_Registrar(#Name, &Name); \
	static void Name()

#define ASSERT_TRUE(Expression) \
	do \
	{ \
		if (!(Expression)) \
			Test::Fail(#Expression, __FILE__, __LINE__); \
	} while (false)

#define ASSERT_FALSE(Expression) ASSERT_TRUE(!(Expression))

#define ASSERT_EQ(Expected, Actual) \
	Test::AreEqual((Expected), (Actual), #Expected, #Actual, __FILE__, __LINE__)

#endif
