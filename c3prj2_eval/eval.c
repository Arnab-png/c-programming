#include "eval.h"
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// if hand contains at least 5 cards of one suit,
// return the suit, else return NUM_SUITS
// For example:
// Given Kd Qs 0s 9h 8s 7s, it would return SPADES.
//   Given Kd Qd 0s 9h 8c 7c, it would return NUM_SUITS.
suit_t flush_suit(deck_t * hand) {
  unsigned suit_counts[4] = {0};
  for(int i=0; i<hand->n_cards; i++) {
    suit_t suit = hand->cards[i]->suit;
    suit_counts[suit]++;
    if (suit_counts[suit] >= 5) {
      return suit;
    }
  }
  return NUM_SUITS;
}

// return largest element in an array of integers
// Used by get_match_counts, which is written in course 4
unsigned get_largest_element(unsigned * arr, size_t n) {
  unsigned largest = 0;
  for(int i=0; i<n; i++) {
    if (arr[i] > largest) {
      largest = arr[i];
    }
  }
  return largest;
}

// Return index in match_counts whose value is n of a kind
size_t get_match_index(unsigned * match_counts, size_t n,unsigned n_of_akind){
  int match_index;
  for(match_index=0; match_index<n; match_index++) {
    if (match_counts[match_index] == n_of_akind) {
      return match_index;
    }
  }
  return n;
}

// Assuming 3 of a kind or a pair has been found, see if
// there is another pair to make a flush or two pairs
// Return the index of the secondary pair, or -1 if there
// is none.
ssize_t  find_secondary_pair(deck_t * hand,
			     unsigned * match_counts,
			     size_t match_idx) {
  ssize_t index;
  for(index=0; index<hand->n_cards; index++) {
    if (match_counts[index] > 1
	&& hand->cards[index]->value != hand->cards[match_idx]->value) {
      return index;
    }
  }
  return -1;
}

/*
  This function should determine if there is a straight
   starting at index (and only starting at index) in the
   given hand.  If fs is NUM_SUITS, then it should look
   for any straight.  If fs is some other value, then
   it should look for a straight flush in the specified suit.
    This function should return:
    -1 if an Ace-low straight was found at that index
     0  if no straight was found at that index
     1  if any other straight was found at that index
*/
// assumes cards are sorted in descending order
// assumes at least 5 cards in hand starting at index
// n is the number of cards to check (for ace low, we check
// for an ace and 4 in a row starting with a 5).
int is_n_length_straight_at(deck_t * hand, size_t index, suit_t fs, int n) {
  int num_in_a_row = 0;
  unsigned last_value = hand->cards[index]->value+1;
  /*  
  if(fs != NUM_SUITS && hand->cards[index]->suit != fs) {
    return 0;
  }
  */
  if(index < hand->n_cards-1 && hand->cards[index]->value == hand->cards[index+1]->value) {
    return 0;
  }
  for(int i=index; i<hand->n_cards; i++) {
    if(fs == NUM_SUITS) {
      if(hand->cards[i]->value != last_value) {
	if (hand->cards[i]->value == last_value-1) {
	  num_in_a_row++;
	  if (num_in_a_row >= n) {
	    return 1;
	  }
	} else {
	  return 0;
	}
	last_value = hand->cards[i]->value;
      }
    } else if (hand->cards[i]->suit == fs) {
      if (hand->cards[i]->value == last_value-1) {
	num_in_a_row++;
	if (num_in_a_row >= n) {
	    return 1;
	}
      } else {
	return 0;
      }
      last_value = hand->cards[i]->value;
    }
  }
  return 0;
}

// assumes at least 4 cards in hand starting at index
int is_ace_low_straight_at(deck_t * hand, size_t index, suit_t fs, int n) {
  assert(hand->cards[index]->value == VALUE_ACE &&
	 (fs == NUM_SUITS || hand->cards[index]->suit == fs));
  int i = index+1;
  while(hand->cards[i]->value != 5 ||
	!(fs==NUM_SUITS || hand->cards[i]->suit ==fs)){
    i++;
    if (i > hand->n_cards - 4) {
      return 0;
    }
  }
  if (is_n_length_straight_at(hand, i, fs, 4)) {
    return -1;
  }
  return 0;
}

// Determine if there is a straight beginning at index
// (and only at index) in the hand
// Assumes cards will appear in descending order by value
int is_straight_at(deck_t * hand, size_t index, suit_t fs) {
  if (hand->n_cards - index < 5) {
    return 0;
  }
  if (is_n_length_straight_at(hand, index, fs, 5) == 1) {
    return 1;
  }
  int possible_index = -1;
  for(int i=index; hand->cards[i]->value == VALUE_ACE && i < hand->n_cards - 4; i++) {
    if (fs == NUM_SUITS || hand->cards[i]->suit == fs) {
      possible_index = i;
      break;
    }
  }
  if (possible_index >= 0) {
    return is_ace_low_straight_at(hand, possible_index, fs, 4);
  }
  return 0;
}

// assumes cards in hand are in descending order
hand_eval_t build_hand_from_match(deck_t * hand,
				  unsigned n,
				  hand_ranking_t what,
				  size_t idx) {
  hand_eval_t ans;
  ans.ranking = what;
  // Copy "n" cards from the hand, starting at "idx"
  // into the first "n" elements of the "cards" array
  // of "ans"
  for(int i=0; i<n; i++) {
    ans.cards[i] = hand->cards[idx+i];    // see eval.h
  }
  // Fill the remainder of the "cards" array with the
  // highest-value cards from the hand which were not
  // in the "n of a kind".
  int i=n;
  int j=0;
  for(; i<5 && j<idx; i++, j++) {
    ans.cards[i] = hand->cards[j];
  }
  if(i < 5) {
    j=idx+n;
    for(; i<5; i++, j++) {
      ans.cards[i] = hand->cards[j];
    }
  }
  return ans;
}

void sort_hand(deck_t * hand) {
  sort_cards(hand->cards, hand->n_cards);
}

int compare_hands(deck_t * hand1, deck_t * hand2) {
  sort_hand(hand1); sort_hand(hand2);
  hand_eval_t e1 = evaluate_hand(hand1);
  hand_eval_t e2 = evaluate_hand(hand2);
  if (e1.ranking != e2.ranking) {
    return e2.ranking - e1.ranking;
  }
  for(int i=0; i<5; i++) {
    if(e1.cards[i]->value != e2.cards[i]->value) {
      return e1.cards[i]->value - e2.cards[i]->value;
    }
  }
  return 0;
}



//   Given a hand (deck_t) of cards, this function
//   allocates an array of unsigned ints with as
//   many elements as there are cards in the hand.
//   It then fills in this array with
//   the "match counts" of the corresponding cards.
//   That is, for each card in the original hand,
//   the value in the match count array
//   is how many times a card of the same
//   value appears in the hand.  For example,
//   given
//     Ks Kh Qs Qh 0s 9d 9c 9h
//   This function would return
//     2  2  2  2  1  3  3  3
//   because there are 2 kings, 2 queens,
//   1 ten, and 3 nines.
int get_match_count(unsigned * values, size_t nvalues, int ifrom) {
  int i = ifrom + 1;
  int count = 1;
  while (i < nvalues && values[i] == values[ifrom]) {
    count++; i++;
  }
  return count;
}

unsigned * get_match_counts(deck_t * hand) {
  if (hand == NULL || hand->n_cards == 0) {
    return NULL;
  }
  // the counts to be returned
  unsigned * counts = malloc(hand->n_cards * sizeof(*counts));
  
  // a temporary array to make it easier to loop through values
  unsigned * values = malloc(hand->n_cards * sizeof(*counts));
  for (int i=0; i < hand->n_cards; i++) {
    values[i] = hand->cards[i]->value;
  }

  int i = 0;
  int count = 0;
  while (i < hand->n_cards) {
    count = get_match_count(values, hand->n_cards, i);
    for (int j = 0; j < count; j++) {
      counts[i+j] = count;
    }
    i += count;
  }
  free(values);
  return counts;
}

void
btrace(void)
{
  const int BT_BUF_SIZE = 256;
  int j, nptrs;
  void *buffer[BT_BUF_SIZE];
  char **strings;

  nptrs = backtrace(buffer, BT_BUF_SIZE);
  printf("backtrace() returned %d addresses\n", nptrs);

  /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
     would produce similar output to the following: */

  strings = backtrace_symbols(buffer, nptrs);
  if (strings == NULL) {
    perror("backtrace_symbols");
    exit(EXIT_FAILURE);
  }

  for (j = 0; j < nptrs; j++)
    printf("%s\n", strings[j]);

  free(strings);
}

// We provide the below functions.  You do NOT need to modify them
// In fact, you should not modify them!


//This function copies a straight starting at index "ind" from deck "from".
//This copies "count" cards (typically 5).
//into the card array "to"
//if "fs" is NUM_SUITS, then suits are ignored.
//if "fs" is any other value, a straight flush (of that suit) is copied.
void copy_straight(card_t ** to, deck_t *from, size_t ind, suit_t fs, size_t count) {
  if(!(fs == NUM_SUITS || from->cards[ind]->suit == fs)) {
    btrace();
  }
  assert(fs == NUM_SUITS || from->cards[ind]->suit == fs);
  unsigned nextv = from->cards[ind]->value;
  size_t to_ind = 0;
  while (count > 0) {
    assert(ind < from->n_cards);
    assert(nextv >= 2);
    assert(to_ind <5);
    if (from->cards[ind]->value == nextv &&
	(fs == NUM_SUITS || from->cards[ind]->suit == fs)){
      to[to_ind] = from->cards[ind];
      to_ind++;
      count--;
      nextv--;
    }
    ind++;
  }
}


//This looks for a straight (or straight flush if "fs" is not NUM_SUITS)
// in "hand".  It calls the student's is_straight_at for each possible
// index to do the work of detecting the straight.
// If one is found, copy_straight is used to copy the cards into
// "ans".
int find_straight(deck_t * hand, suit_t fs, hand_eval_t * ans) {
  if (hand->n_cards < 5){
    return 0;
  }
  for(size_t i = 0; i <= hand->n_cards -5; i++) {
    int x = is_straight_at(hand, i, fs);
    if (x != 0){
      if (x < 0) { //ace low straight
	if (hand->cards[i]->value == VALUE_ACE &&
	       (fs == NUM_SUITS || hand->cards[i]->suit == fs)) {
	  return 0;
	}
	//	assert(hand->cards[i]->value == VALUE_ACE &&
	//       (fs == NUM_SUITS || hand->cards[i]->suit == fs));
	ans->cards[4] = hand->cards[i];
	size_t cpind = i+1;
	while(hand->cards[cpind]->value != 5 ||
	      !(fs==NUM_SUITS || hand->cards[cpind]->suit ==fs)){
	  cpind++;
	  assert(cpind < hand->n_cards);
	}
	copy_straight(ans->cards, hand, cpind, fs,4) ;
      }
      else {
	copy_straight(ans->cards, hand, i, fs,5);
      }
      return 1;
    }
  }
  return 0;
}


//This function puts all the hand evaluation logic together.
//This function is longer than we generally like to make functions,
//and is thus not so great for readability :(
hand_eval_t evaluate_hand(deck_t * hand) {
  suit_t fs = flush_suit(hand);
  hand_eval_t ans;
  if (fs != NUM_SUITS) {
    if(find_straight(hand, fs, &ans)) {
      ans.ranking = STRAIGHT_FLUSH;
      return ans;
    }
  }
  unsigned * match_counts = get_match_counts(hand);
  unsigned n_of_a_kind = get_largest_element(match_counts, hand->n_cards);
  assert(n_of_a_kind <= 4);
  size_t match_idx = get_match_index(match_counts, hand->n_cards, n_of_a_kind);
  ssize_t other_pair_idx = find_secondary_pair(hand, match_counts, match_idx);
  free(match_counts);
  if (n_of_a_kind == 4) { //4 of a kind
    return build_hand_from_match(hand, 4, FOUR_OF_A_KIND, match_idx);
  }
  else if (n_of_a_kind == 3 && other_pair_idx >= 0) {     //full house
    ans = build_hand_from_match(hand, 3, FULL_HOUSE, match_idx);
    ans.cards[3] = hand->cards[other_pair_idx];
    ans.cards[4] = hand->cards[other_pair_idx+1];
    return ans;
  }
  else if(fs != NUM_SUITS) { //flush
    ans.ranking = FLUSH;
    size_t copy_idx = 0;
    for(size_t i = 0; i < hand->n_cards;i++) {
      if (hand->cards[i]->suit == fs){
	ans.cards[copy_idx] = hand->cards[i];
	copy_idx++;
	if (copy_idx >=5){
	  break;
	}
      }
    }
    return ans;
  }
  else if(find_straight(hand,NUM_SUITS, &ans)) {     //straight
    ans.ranking = STRAIGHT;
    return ans;
  }
  else if (n_of_a_kind == 3) { //3 of a kind
    return build_hand_from_match(hand, 3, THREE_OF_A_KIND, match_idx);
  }
  else if (other_pair_idx >=0) {     //two pair
    assert(n_of_a_kind ==2);
    ans = build_hand_from_match(hand, 2, TWO_PAIR, match_idx);
    ans.cards[2] = hand->cards[other_pair_idx];
    ans.cards[3] = hand->cards[other_pair_idx + 1];
    if (match_idx > 0) {
      ans.cards[4] = hand->cards[0];
    }
    else if (other_pair_idx > 2) {  //if match_idx is 0, first pair is in 01
      //if other_pair_idx > 2, then, e.g. A A K Q Q
      ans.cards[4] = hand->cards[2];
    }
    else {       //e.g., A A K K Q
      ans.cards[4] = hand->cards[4]; 
    }
    return ans;
  }
  else if (n_of_a_kind == 2) {
    return build_hand_from_match(hand, 2, PAIR, match_idx);
  }
  return build_hand_from_match(hand, 0, NOTHING, 0);
}
