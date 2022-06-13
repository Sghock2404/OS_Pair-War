/* 
*project1.cpp
*Scott Hockaday
*Course: Ooperating Systems CS4328
*project 1: Pair War
* due: 10/08/21
*/

#ifdef __unix
#define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),  (mode)))==NULL
#endif

#include <fstream>
#include <pthread.h>
#include <cstdlib>
#include <cstdio>
#include <stdio.h>
#include <stdlib.h>


constexpr auto NUMBER_OF_THREADS = 3; //# of players(threads);

pthread_mutex_t mutex_dealerExit = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_useTheDeck = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition_var = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_win1 = PTHREAD_COND_INITIALIZER;
pthread_t player_Threads[NUMBER_OF_THREADS];
pthread_t dealer_Thread;

FILE* pFile;
const int NUMBER_OF_CARDS = 52; //# of cards in a deck
int deck[NUMBER_OF_CARDS]; //defines deck with 52
auto numRound = 1; //starts at round 1, keep track of round
int roundsToPlay = 3; //# of rounds to play
auto testValue = 1; //random test value (just 1 for now)
auto turn = 0; //keep track of turns
auto playerTurn = 1; //keep track of a player's turn
int* topDeck; //pointer to top of the deck
int* bottomDeck; //pointer bottom of the deck
int seed = 0; //seed for rand
bool win = false; //boolean to indicate which player won

struct hand { 
	int card_One, card_Two;
};
hand hand_One, hand_Two, hand_Three;

//functions
void* dealerThread(void* args);
void* playerThreads(void* playerID);
void use_Deck(long, hand);
void randSeed();
void build_Deck();
void deal_Cards();
void shuffle_Deck();
void roundPlay();
void print_Deck();

int main(int argc, char* argv[])
{
	fopen_s(&pFile, "LOG.txt", "a");
	randSeed();
	build_Deck();

	if (pFile == NULL) {
		printf("cannot open file!");
		return false;
	}
	srand(atoi(argv[0]));

	do{
		roundPlay();
		numRound++;
		win = false;
	} while (numRound <= roundsToPlay);

	fclose(pFile);
	exit(EXIT_SUCCESS);

	return 0;
}

void randSeed()
{    
	srand(seed);
}

void build_Deck() //put 4 of each card (suit) value into deck[]
{ 
	int card_Value = 0;
	int card = 0;
	int suitNum = 4;
	do{
		for (int i = 0; i < suitNum; ++i) {
			deck[card] = card_Value;
			card++;
		}
		card_Value++;
	} while ((card < NUMBER_OF_CARDS) && (card_Value < (NUMBER_OF_CARDS / suitNum)));
	topDeck = deck;          // point to top of deck
	bottomDeck = deck + 51;  // point to bottom of deck
}

void roundPlay()
{
	printf("<ROUND: %d>\n", numRound);
	fprintf(pFile, "<ROUND: %d>\n", numRound);

	int retDealer = pthread_create(&dealer_Thread, NULL, &dealerThread, NULL);
	int retPlayer;
	for (long i = 1; i <= NUMBER_OF_THREADS; ++i)
	{
		retPlayer = pthread_create(&player_Threads[i], NULL, playerThreads, (void *)i);
	}
	pthread_join(dealer_Thread, NULL);
	for (int j = 0; j < 3; ++j)
	{
		pthread_join(player_Threads[j], NULL);
	}
}

void* dealerThread(void* args) //dealer thread
{
	long pid = 0;
	turn = 0;
	hand dealerHand{};
	use_Deck(pid, dealerHand);

	pthread_mutex_lock(&mutex_dealerExit);
	do {
		pthread_cond_wait(&cond_win1, &mutex_dealerExit);
	} while (!win);
	pthread_mutex_unlock(&mutex_dealerExit);
	fprintf(pFile, "DEALER: exits round\n");
	pthread_exit(NULL);

	return 0;
}

void* playerThreads(void* playerID) //player thread(s)
{
	long pid = (long)playerID;
	
	hand Hand{};

	if(numRound == 1)
	{
		if (pid == 1) Hand = hand_One;
		else if (pid == 2) Hand = hand_Two;
		else Hand = hand_Three;
	}
	else if(numRound == 2)
	{
		if (pid == 2) Hand = hand_One;
		else if (pid == 3) Hand = hand_Two;
		else Hand = hand_Three;
	}
	else if(numRound == 3)
	{
		if (pid == 3) Hand = hand_One;
		else if (pid == 1) Hand = hand_Two;
		else Hand = hand_Three;
	}
	while(win == 0) 
	{
		pthread_mutex_lock(&mutex_useTheDeck); //lock deck  
		while (pid != turn && win == 0) //make players wait for their turn
		{
			pthread_cond_wait(&condition_var, &mutex_useTheDeck);
		}
		if (win == 0) {
			use_Deck(pid, Hand); //let players use deck
		}
		pthread_mutex_unlock(&mutex_useTheDeck); //unlocks deck
	}
	// leave the player thread
	fprintf(pFile, "PLAYER %ld: exits round\n", pid);
	pthread_exit(NULL);

	return 0;
}

void use_Deck(long pid, hand Hand) 
{
	if (pid == 0) //dealer uses deck
	{ 

		fprintf(pFile, "<DEALER: shuffle>\n"); shuffle_Deck(); //Dealer shuffles deck    
		fprintf(pFile, "<DEALER: deals cards>\n"); deal_Cards(); // Dealer deals cards
	}
	else //player uses deck
	{ 

	   //show hand
		printf("PLAYER %ld: \n", pid);
		fprintf(pFile, "PLAYER %ld: hand %d \n", pid, Hand.card_One);

		//draw a card
		Hand.card_Two = *topDeck,
			topDeck = topDeck + 1;
		fprintf(pFile, "PLAYER %ld: draws %d \n", pid, Hand.card_Two);

		//show hand     
		printf("HAND %d %d\n", Hand.card_One, Hand.card_Two);
		fprintf(pFile, "PLAYER %ld: hand %d %d\n", pid, Hand.card_One, Hand.card_Two);

		//compare cards
		//if cards match, then declare a winner  
		//if cards don't match, then discard a card 
		if (Hand.card_One == Hand.card_Two) {
			          
			printf("WIN yes\n");
			fprintf(pFile, "PLAYER %ld: wins\n", pid);
			win = true;
			pthread_cond_signal(&cond_win1); //signals dealer to exit...           
		}
		else 
		{	           
			printf("WIN no\n");
			//shift cards in deck to make room for discard decl=k
			topDeck = topDeck - 1;
			int* ptr = topDeck;
			while (ptr != bottomDeck) 
			{
				*ptr = *(ptr + 1);
				ptr = ptr + 1;
			}
			int discard = rand() % 2; //randomly selects discard, puts it back in the deck
			if (discard == 0) 
			{
				fprintf(pFile, "PLAYER %ld: discards %d \n", pid, Hand.card_One);
				*bottomDeck = Hand.card_One;  //put card back in deck
				Hand.card_One = Hand.card_Two; //set card_One to remaining card value
			}
			else 
			{
				fprintf(pFile, "PLAYER %ld: discards %d \n", pid, Hand.card_Two);
				*bottomDeck = Hand.card_Two;
			}
			print_Deck(); //print contents of the deck
		}
	}
	++turn; //increment turn, next player can use  deck
	if (turn > NUMBER_OF_THREADS) 
	{
		turn = 1; //if player3 went, move to player1
	}
	pthread_cond_broadcast(&condition_var); //broadcast that deck is available
}

void shuffle_Deck()
{
	for(int i = 0; i < (NUMBER_OF_CARDS - 1); ++i) 
	{
		int randPos = i + (rand() % (NUMBER_OF_CARDS - i));
		int temp = deck[i];
		deck[i] = deck[randPos];
		deck[randPos] = temp;
	}
}

void deal_Cards()
{
	hand_One.card_One = *topDeck; 
	topDeck = topDeck + 1;
	
	hand_Two.card_One = *topDeck;
	topDeck = topDeck + 1;
	
	hand_Three.card_One = *topDeck;
	topDeck = topDeck + 1;
}

void print_Deck()
{
	printf("DECK: ");
	fprintf(pFile, "DECK: ");
	int* ptr = topDeck;
	while(ptr != bottomDeck)
	{
		printf("%d ", *ptr);
		fprintf(pFile, "%d ", *ptr);
		++ptr;
		if (ptr == bottomDeck)
		{
			printf("%d \n", *ptr);
			fprintf(pFile, "%d \n", *ptr);
		}
	}
}