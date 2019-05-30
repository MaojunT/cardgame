#include <pthread.h>
#include <algorithm>
#include <time.h>
#include <fstream>

const int DECK_SIZE = 52,
          PLAYER_SIZE = 3;
int deck[DECK_SIZE];
int turn = 0;
int round = 1;
int seed;
bool win = false;
bool dealt = false;
int *top;
int *end;
FILE *fout;
struct hand{
    int card1, card2;
};
hand hand1, hand2, hand3;

pthread_t dealer;
pthread_t player[PLAYER_SIZE];
pthread_mutex_t player_wait1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t player_wait2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t deck_use = PTHREAD_COND_INITIALIZER;
pthread_cond_t dealer_done = PTHREAD_COND_INITIALIZER;;
pthread_mutex_t dealer_wait = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t wins = PTHREAD_COND_INITIALIZER;

void populateDeck();
void *dealerThread(void *arg);
void *playerThread(void *arg);
void usingDeck(long id, hand h);
void displayDeck();

int main(int argc, char* argv[])
{
    fout = fopen("Pairwar.txt", "a");

    populateDeck();
    //srand((int)time(0));
    seed = atoi(argv[1]);
    srand(seed);

    pthread_create(&dealer, NULL, &dealerThread, NULL);
    for(long i = 1; i <= PLAYER_SIZE; i++)
       pthread_create(&player[i], NULL, &playerThread, (void *)i);


    pthread_join(dealer, NULL);
    for(int i = 1; i <= PLAYER_SIZE; i++)
       pthread_join(player[i], NULL);

    fclose(fout);

	return 0;
}
///////////////////////////////////////
////This function creates the deck
void populateDeck()
{
   int card = 1,
       suit = 4,
       number_of_card = 0;
   while((number_of_card < DECK_SIZE) && (card <= (DECK_SIZE / suit))) {
      for(int i = 0; i < suit; i++) {
         deck[number_of_card] = card;
         number_of_card++;
      }
      card++;
   }
   top = deck;
   end = deck + 51;
}
///////////////////////////////////////
////What dealer thread does
void *dealerThread(void *arg)
{
   do{
        printf("--------ROUND: %d---------\n", round);
        fprintf(fout, "---------ROUND: %d---------\n", round);
        win = false;
        dealt == false;
        long id = 0;
        hand dHand;
        usingDeck(id, dHand);

        pthread_mutex_lock(&dealer_wait);
        while(win == false) {
            pthread_cond_wait(&wins, &dealer_wait);
        }
        pthread_mutex_unlock(&dealer_wait);
        fprintf(fout, "DEALER: exits round\n");
        round++;
    }while (round < 4);
   pthread_exit(NULL);
}
///////////////////////////////////////
////What player thread does
void *playerThread(void *arg){
   long id = (long)arg;  //uses long instead of integer to avoid precision error
   //decides which player draws first based on round
   while(round <= 3) {
        pthread_mutex_lock(&player_wait1);

        while (dealt == false)
        {
            pthread_cond_wait(&dealer_done, &player_wait1);
        }
        pthread_mutex_unlock(&player_wait1);
        hand phand;
        if(round == 1) {
            if(id == 1) phand = hand1;
            else if(id == 2) phand = hand2;
            else phand = hand3;
        }
        else if(round == 2) {
            if(id == 2) phand = hand1;
            else if(id == 3) phand = hand2;
            else phand = hand3;
            turn = 2;
        }
        else if(round == 3) {
            if(id == 3) phand = hand1;
            else if(id == 1) phand = hand2;
            else phand = hand3;
            turn = 3;
        }
        //waits untill the player can use deck
        while(win == false) {
            pthread_mutex_lock(&player_wait2);
            while(id != turn && win == false) {
                pthread_cond_wait(&deck_use, &player_wait2);
            }
            if(win == false) {
                usingDeck(id, phand);
            }
            pthread_mutex_unlock(&player_wait2);
        }
   }

   pthread_exit(NULL);
}
///////////////////////////////////////
////Each thread will be holding deck for different use
void usingDeck(long id, hand h){
   //dealer uses deck
   if(id == 0) {
      fprintf(fout, "DEALER: shuffle\n");  //dealer shuffles deck
      for(int i = 0; i < (DECK_SIZE - 1); i++) {
          int randPos = i + (rand() % (DECK_SIZE - i));
          int temp = deck[i];
          deck[i] = deck[randPos];
          deck[randPos] = temp;
      }
      //dealer deals 1 card to each player's hand
      hand1.card1 = *top;
      top++;
      hand2.card1 = *top;
      top++;
      hand3.card1 = *top;
      top++;
      dealt = true;
      pthread_cond_broadcast(&dealer_done);
   }
   //player uses deck
   else {
      fprintf(fout, "PLAYER %ld: hand %d \n", id, h.card1);
      h.card2 = *top,
      top++;
      fprintf(fout, "PLAYER %ld: draws %d \n", id, h.card2);
      printf("HAND: %d %d\n", h.card1, h.card2);
      //when 2 cards in hand is equal
      if(h.card1 == h.card2) {
         printf("WIN: yes\n");
         printf("DECK: ");
         int *ptr = top;
         while(ptr != end) {
            printf("%d ", *ptr);
            ptr++;
            if(ptr == end) {
                printf("%d \n", *ptr);
            }
         }
         fprintf(fout, "PLAYER %ld: hand %d %d\n", id, h.card1, h.card2);
         fprintf(fout, "PLAYER %ld: wins\n", id);
         win = true;
         pthread_cond_signal(&wins); // signal dealer to exit
         fprintf(fout, "PLAYER 1: exits round\n");
         fprintf(fout, "PLAYER 2: exits round\n");
         fprintf(fout, "PLAYER 3: exits round\n");
      }
      //when 2 cards in hand is not equal
      else{
         printf("WIN: no\n");
         top--;
         int *ptr = top;
         while(ptr != end) {
            *ptr = *(ptr + 1);
            ptr++;
         }
         int discard = rand() % 2;
         if(discard == 0) {
            fprintf(fout, "PLAYER %ld: discards %d\n", id, h.card1);
            *end = h.card1;
            h.card1 = h.card2;
            fprintf(fout, "PLAYER %ld: hand %d \n", id, h.card1);
         }
         else {
            fprintf(fout, "PLAYER %ld: discards %d\n", id, h.card2);
            *end = h.card2;
            fprintf(fout, "PLAYER %ld: hand %d \n", id, h.card1);
         }
         displayDeck();
      }
   }
   turn++;  //increase turn so that players draw card based on round
   if(turn > PLAYER_SIZE)
       turn = 1;
   pthread_cond_broadcast(&deck_use);
}
///////////////////////////////////////
////Writes content of deck to console and txt file
void displayDeck() {
   printf("DECK: ");
   fprintf(fout, "DECK: ");
   int *ptr = top;
   while(ptr != end) {
      printf("%d ", *ptr);
      fprintf(fout, "%d ", *ptr);
      ptr++;
      if(ptr == end) {
         printf("%d \n", *ptr);
         fprintf(fout, "%d \n", *ptr);
      }
   }
}
///////////////////////////////////////
