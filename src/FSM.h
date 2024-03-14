

#pragma once 

/*------------------------------ includes ---------------------------------*/
#include <Arduino.h>

/*------------------------------ defines ----------------------------------*/
class State 
{
	public:
		explicit State(void(*runFunction)());
		State(void(*enterFunction)(), void (*runFunction)(), void (*exitFunction)());

		void enter();
		void run();
		void exit();
	private:
		void (*userEnter)();
		void (*userRun)();
		void (*userExit)();
};

class FSM 
{
	public:
		explicit FSM(State& current);

		FSM& run();
		FSM& transitionTo(State& state);
		FSM& immediateTransitionTo(State& state);
		State* getCurrentState();
		State* getPrevState();
		bool isInState(State &state) const;
		unsigned long timeInCurrentState();

	private:
		bool 	needToTriggerEnter;
		State* 	currentState;
		State* 	nextState;
		State* 	prevState;
		unsigned long stateChangeTime;
};
