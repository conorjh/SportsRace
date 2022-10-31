#ifndef CAREER_H
#define CAREER_H
#include "Race.h"

namespace Game
{
	namespace Career
	{
		struct CareerProfileFinancial
		{
			int Cash = 5000;
		};

		struct CareerProfile
		{
			CareerProfile();

			Race::Racer MainFella;
			CareerProfileFinancial Financials;
		};

		struct CareerData
		{
			CareerData();

			Race::RacerDB Racers;

			CareerProfile Profile;
		};

		class CareerOrchestrator
		{

		public:
			CareerData* Data;
			CareerProfile* Profile;

			CareerOrchestrator(CareerData*, CareerProfile*);


		};
	}

	
	
}
#endif