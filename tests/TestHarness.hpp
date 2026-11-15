#ifndef TESTHARNESS_HPP
#define TESTHARNESS_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace Test
{
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

	inline std::string& CurrentFailureMessage()
	{
		static std::string Message;
		return Message;
	}

	inline void Fail(const char* Expression, const char* File, int Line)
	{
		if (!CurrentFailureMessage().empty())
			return;

		std::ostringstream Stream;
		Stream << File << ":" << Line << ": assertion failed: " << Expression;
		CurrentFailureMessage() = Stream.str();
	}

	template <typename TExpected, typename TActual>
	bool AreEqual(const TExpected& Expected, const TActual& Actual, const char* ExpectedExpression,
		const char* ActualExpression, const char* File, int Line)
	{
		if (Expected == Actual)
			return true;

		if (CurrentFailureMessage().empty())
		{
			std::ostringstream Stream;
			Stream << File << ":" << Line << ": expected " << ExpectedExpression << " == " << ActualExpression
				<< ", but the values differ";
			CurrentFailureMessage() = Stream.str();
		}
		return false;
	}

	inline int RunAll()
	{
		int Failed = 0;
		for (const auto& TestCase : Cases())
		{
			CurrentFailureMessage().clear();
			TestCase.Body();
			if (CurrentFailureMessage().empty())
				std::cout << "[PASS] " << TestCase.Name << '\n';
			else
			{
				Failed++;
				std::cerr << "[FAIL] " << TestCase.Name << " - " << CurrentFailureMessage() << '\n';
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
		{ \
			Test::Fail(#Expression, __FILE__, __LINE__); \
			return; \
		} \
	} while (false)

#define ASSERT_FALSE(Expression) ASSERT_TRUE(!(Expression))

#define ASSERT_EQ(Expected, Actual) \
	do \
	{ \
		if (!Test::AreEqual((Expected), (Actual), #Expected, #Actual, __FILE__, __LINE__)) \
			return; \
	} while (false)

#endif
