#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 4096
#define MAX_STATES 256
#define MAX_SYMBOLS 256
#define EPSILON '~'

// Array of end states
typedef struct {
	int count;
	int states[MAX_STATES];
} endState;

// Automaton structure that holds all the data related to this DFA
typedef struct {
	// This is a set of possible states
	char * statesNames[MAX_STATES];
	
	// Number of states total
	int statesNum;
	
	// Set of finishing states
	int finishState[MAX_STATES];
	
	// Index of start state
	int startStateIndex;
	
	// Transition symbols
	char transitions[MAX_SYMBOLS];
	
	// Number of transition symbols
	int transitionsNum;
	
	// 2D array of transitions
	endState ** transitionTable;
} Automaton;

// This function loads a string from file and stores it in temporary buffer
// It returns only non-empty strings
// It outputs NULL if file ended and pointer to string if something was read
const char * GetLine(FILE * f) {
	static char line[MAX_LINE_LENGTH];
	
	// We need to load a string from file that begins from anything besides
	// \n - newline character
	// \0 - file end (presumably)
	// # - comment symbol
	do {
		// If we could not load string, it seems like we stumbled upon file name
		if (fgets(line, MAX_LINE_LENGTH, f) == NULL)
			return NULL;
		
		// Repeat until we got a string starting from something other than \n, \0 and # and
		// end of file was not reached
	} while (!feof(f) && (line[0] == '\0' || line[0] == '\n' || line[0] == '#'));
	
	// If there is an end, no string received
	if (feof(f))
		return NULL;
	
	// We need to cut last '\n' symbol
	int nextlinePos = strlen(line) - 1;
	if (nextlinePos <= 0)
		return NULL; // Should not get here
	if (line[nextlinePos] == '\n')
		line[nextlinePos] = '\0';
	
	return line;
}

// This function returns index of state or -1 if not found
// Would never return a->statesNum or larger
int StateToIdx(Automaton * a, const char * state) {
	int i;
	
	// Iterate through all states and find the one with exact match with 'state'
	for (i = 0; i < a->statesNum; i++)
		if (strcmp(state, a->statesNames[i]) == 0)
			return i;
	
	// 'state' is not found
	return -1;
}

// Thus function returns index of transition symbol or -1 if not found
// Would never return a->transitionsNum or larger
int TransitionToIdx(Automaton * a, const char transition) {
	int i;
	
	// Iterate through all transition symbols to find one that matches with 'transition' symbol
	for (i = 0; i < a->transitionsNum; i++)
		if (transition == a->transitions[i])
			return i;
	
	// Not found
	return -1;
}

// This function reads a word from string and returns pointer to the next word
// If string is emptied, returns NULL
const char * ReadWord(const char * str, char * word) {
	// Try to read word
	if (sscanf(str, "%s", word) != 1)
		return NULL;
	
	// Word is read successfully. Now we need to shift our pointer to next word (or to the end)
	const char * strPtr = str;
	
	// Skip to the next word
	while (*strPtr != ' ' && *strPtr != '\0')
		strPtr++;
	
	// Skip spaces
	while (*strPtr == ' ')
		strPtr++;
	
	// It is a beginning of the next word or end of string
	return strPtr;
}

// This function loads automaton from file
// Returns 0 on success, 1 on failure
int LoadAutomaton(Automaton * a, const char path[]) {
	// Initialize numbers 
	a->statesNum = 0;
	a->transitionsNum = 0;
	
	FILE * f;
	f = fopen(path, "r");
	
	if (f == NULL) {
		fprintf(stderr, "File not found or could not be opened: %s\n", path);
		return 1;
	}
	
	// Load initial state
	char initialState[MAX_LINE_LENGTH];
	const char * initialStateStr = GetLine(f);
	if (initialStateStr == NULL) {
		fprintf(stderr, "Cannot read initial state!\n");
		fclose(f);
		return 1;
	}
	strcpy(initialState, initialStateStr);
	
	// Load states string
	const char * states = GetLine(f);
	if (states == NULL) {
		fprintf(stderr, "Cannot read set of states!\n");
		fclose(f);
		return 1;
	}
	
	// Load possible states and assign them to numbers
	char curState[MAX_LINE_LENGTH];
	while ((states = ReadWord(states, curState)) != NULL) {
		a->statesNames[a->statesNum] = (char *) malloc(MAX_LINE_LENGTH * sizeof(char));
		strcpy(a->statesNames[a->statesNum], curState);
		a->statesNum++;
	}
	
	// Evaluate start state index
	a->startStateIndex = StateToIdx(a, initialState);
	if (a->startStateIndex == -1) {
		fprintf(stderr, "Start state %s is not listed in states list!\n", initialState);
		fclose(f);
		return 1;
	}
	
	// Read symbol table
	const char * transitions = GetLine(f);
	if (transitions == NULL) {
		fprintf(stderr, "Cannot read transition symbols!\n");
		fclose(f);
		return 1;
	}
	
	char curSymbol[MAX_LINE_LENGTH];
	while ((transitions = ReadWord(transitions, curSymbol)) != NULL) {
		char c = curSymbol[0];
		
		// check c for duplicates
		int t;
		for (t = 0; t < a->transitionsNum; t++)
			if (a->transitions[t] == c) {
				fprintf(stderr, "Symbol %c occurs in symbol list twice!\n", c);
				fclose(f);
				return 1;
			}
		
		a->transitions[a->transitionsNum] = c;
		a->transitionsNum++;
	}
	
	// Read finish states
	int i,j;
	for (i = 0; i < a->statesNum; i++)
		a->finishState[i] = 0;
	
	const char * finishStates = GetLine(f);
	if (finishStates == NULL) {
		fprintf(stderr, "Cannot read set of finish states!\n");
		fclose(f);
		return 1;
	}
	
	while ((finishStates = ReadWord(finishStates, curState)) != NULL) {
		int idx = StateToIdx(a, curState);
		if (idx == -1) {
			fprintf(stderr, "Finishing state %s is not listed in states list!\n", curState);
			fclose(f);
			return 1;
		}
		
		if (a->finishState[idx] == 1) {
			fprintf(stderr, "Duplicated finishing state: %s\n", curState);
			fclose(f);
			return 1;
		}
		
		a->finishState[idx] = 1;
	}
	
	// Initialize transition table
	a->transitionTable = (endState **) malloc(a->statesNum * sizeof(endState *));
	for (i = 0; i < a->statesNum; i++) {
		a->transitionTable[i] = (endState *) malloc(a->transitionsNum * sizeof(endState));
		for (j = 0; j < a->transitionsNum; j++)
			a->transitionTable[i][j].count = 0;
	}
	
	// Load transition table from file
	const char * transitionLine;
	while ((transitionLine = GetLine(f)) != NULL) {
		char from[MAX_LINE_LENGTH], symb[MAX_LINE_LENGTH], to[MAX_LINE_LENGTH];
		sscanf(transitionLine, "%s %s %s", from, symb, to);
		
		int fromIdx, symbolIdx, toIdx;
		fromIdx = StateToIdx(a, from);
		symbolIdx = TransitionToIdx(a, symb[0]);
		toIdx = StateToIdx(a, to);
		
		if (fromIdx == -1 || symbolIdx == -1 || toIdx == -1) {
			fprintf(stderr, "Invalid transition: %s %s %s\n", from, symb, to);
			fclose(f);
			return 1;
		}
		
		// Now duplicating states are possible
		a->transitionTable[fromIdx][symbolIdx].states[a->transitionTable[fromIdx][symbolIdx].count] = toIdx;
		a->transitionTable[fromIdx][symbolIdx].count++;
	}
	
	fclose(f);
	return 0;
}

// Debug automaton print
void PrintAutomaton(Automaton * a) {
	int i,j,k;
	
	printf("Start state: %s\n", a->statesNames[a->startStateIndex]);
	
	printf("End states:  ");
	for (i = 0; i < a->statesNum; i++)
		if (a->finishState[i] == 1)
			printf("%s ", a->statesNames[i]);
	printf("\n");
	
	printf("All states:  ");
	for (i = 0; i < a->statesNum; i++)
		printf("%s ", a->statesNames[i]);
	printf("\n");
	
	printf("Symbols:     ");
	for (i = 0; i < a->transitionsNum; i++)
		printf("%c ", a->transitions[i]);
	printf("\n");
	
	printf("Transition table: -------------\n");
	
	for (i = 0; i < a->statesNum; i++)
		for (j = 0; j < a->transitionsNum; j++) {
			int count = a->transitionTable[i][j].count;
			
			if (count != 0) {
				printf("%6s %c", a->statesNames[i], a->transitions[j]);
				for (k = 0; k < count; k++)
					printf(" %6s", a->statesNames[a->transitionTable[i][j].states[k]]);
				printf("\n");
			}
		}
}

// Makes epsilon closure, i.e. mark as possible to visit all the vertices within
// epsilon range to current ones
void ProcessString_EpsilonClosure(Automaton * a, int * possibleStates) {
	int i,k;
	int epsilonIdx = TransitionToIdx(a, EPSILON);
	
	if (epsilonIdx == -1)
		return;
	
	int changed;
	do { // Repeat until no changes remained
		changed = 0;
		for (i = 0; i < a->statesNum; i++)
			if (possibleStates[i])
				for (k = 0; k < a->transitionTable[i][epsilonIdx].count; k++) {
					int toChangeIdx = a->transitionTable[i][epsilonIdx].states[k];
					if (possibleStates[toChangeIdx] == 0) {
						changed = 1;
						possibleStates[toChangeIdx] = 1;
					}
				}
	} while (changed != 0);
}

// Process string using automaton. Possible results:
// 0 - ACCEPTED
// 1 - REJECTED
// 2 - Found wrong symbol
int ProcessString(Automaton * a, const char * string) {
	int len = strlen(string);
	int i,k,t;
	
	// Check if every symbol belongs to automaton symbol set
	for (i = 0; i < len; i++)
		if (TransitionToIdx(a, string[i]) == -1)
			return 2;
	
	// Start simulation
	// Countrary to simple DFSM, NDFSM has a set of current possible states
	// At first step there is only one possible state - starting state
	int currentPossibleStates[MAX_STATES], nextPossibleStates[MAX_STATES];
	for (i = 0; i < a->statesNum; i++)
		currentPossibleStates[i] = 0;
	currentPossibleStates[a->startStateIndex] = 1;
	ProcessString_EpsilonClosure(a, currentPossibleStates);
	
	// Cycle through whole string
	for (t = 0; t < len; t++) {
		int curSymbolIdx = TransitionToIdx(a, string[t]);
		
		if (curSymbolIdx == -1)
			return 2;
		
		// Wipe next states list
		for (i = 0; i < a->statesNum; i++)
			nextPossibleStates[i] = 0;
		
		// Make a step from all current possible states
		for (i = 0; i < a->statesNum; i++)
			if (currentPossibleStates[i])
				for (k = 0; k < a->transitionTable[i][curSymbolIdx].count; k++)
					nextPossibleStates[a->transitionTable[i][curSymbolIdx].states[k]] = 1;
		
		// Make epsilon transitions (if they're present)
		ProcessString_EpsilonClosure(a, nextPossibleStates);
		
		// Copy array to current state
		for (i = 0; i < a->statesNum; i++)
			currentPossibleStates[i] = nextPossibleStates[i];
	}
	
	// Check if at least one possible state is finishing state
	for (i = 0; i < a->statesNum; i++)
		if (currentPossibleStates[i] && a->finishState[i])
			return 0;
	
	return 1;
}

// Main function
int main() {
	// Ask for file paths
	char automatonPath[MAX_LINE_LENGTH] = "NDFSM.txt", stringPath[MAX_LINE_LENGTH] = "string.txt";
	printf("Enter automaton file path: ");
	scanf("%s", automatonPath);
	
	printf("Enter strings file path:   ");
	scanf("%s", stringPath);
	
	Automaton a;
	
	if (LoadAutomaton(&a, automatonPath)) {
		fprintf(stderr, "Could not load automation.\n");
		return 1;
	}
	
	// Debug print
	// PrintAutomaton(&a);
	
	// Open a file
	FILE * f;
	f = fopen(stringPath, "r");
	if (f == NULL) {
		printf("Cannot open strings file %s!\n", stringPath);
		return 1;
	}
	
	// Process every string from this file
	const char * line;
	while ((line = GetLine(f)) != NULL) {
		int res = ProcessString(&a, line);
		switch (res) {
			case 0:
			printf("ACCEPTED LINE %s\n", line);
			break;
			
			case 1:
			printf("REJECTED LINE %s\n", line);
			break;
			
			case 2:
			printf("WRONG SYMBOL: %s\n", line);
			break;
			
			case 3:
			printf("UNKNOWN ERROR %s\n", line);
			break;
		}
	}
	
	fclose(f);
	
	// Actually, there is no need to free automaton resources because there is only one automaton
	// that would be automatically unloaded anyway when application is terminated
	
	return 0;
}
