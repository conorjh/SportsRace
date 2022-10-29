#ifndef CAREER_H
#define CAREER_H
#include "Race.h"

namespace Game
{
	namespace Career
	{
		struct CareerProfileFinancial
		{
			int Cash;
		};

		struct CareerProfile
		{
			Race::Racer MainFella;
			CareerProfileFinancial Financials;
		};

		struct CareerData
		{
			Race::RacerDB Racers;

			CareerProfile Profile;
		};

		class CareerOrchestrator
		{
			CareerData* Data;
		public:
			CareerOrchestrator(CareerData*);


		};
	}

	
	
}
#endif