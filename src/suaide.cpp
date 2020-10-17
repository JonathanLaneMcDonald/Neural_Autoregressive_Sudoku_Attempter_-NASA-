
// g++ suaide.cpp -lcurses -O3 -o suaide

/** TODO Currently Working On: Fix the bugs in Almost Deadly Polygons.  Those damned Extended Polygons are still acting up.*/

/**
NOTE	Checking for brokenness to help in solving is tantamount to nishio.
		Revision: Checking to see if a branch of a logical technique breaks a puzzle is not nishio.  it's something a human would notice in 
			practice and therefore, i'll implement it again.  besides, i feel like i'm being cut out of a lot of action! :  )

TODO TODO
	ASE (i think i can do this by running naked and hidden singles only in the groups that all the cells can see)(i think this would be beautiful
		with recursion)(set the n-th guy, do the eliminations, see what the (n+1)-th guy has left and run through those to n-levels.  swell)
		the expensive part will be ... well... both the recursion and figuring out what all cells involved have in common.  but this is another
		thing that can be generalized with the counter so i can say jump into there and see what it gets you.  along with placements and normal
		puzzle eliminations, this shouldn't be very demanding.  but if you run it on a blank puzzle, you're talking about 30 million rounds for
		just the one shot... that's not counting eliminations... including those, you can multiply by either 8 or 17 times.  but if the group
		already has like four placements, then you're down to more like 120 for that group, down from around 366,000.
		***it'll come in real handy to eliminate placed cells.  that'll cut down this complexity a little***

		the steps for this technique will be to run the 0-512 count 27 times.  once for each group.  only executing for 2 <= count <= 8 and 
		find what cells they can all see, then i'd make a vector of what they had in common (just to speed up cancellation).  then i'd toss it
		to a recursive function that would go to the level which was the bit count cause that tells me how many cells are involved.  i'd set it
		at the beginning and then call the guy underneath me.  i'd call all the way to the bottom and check for validity.  mainly to make sure
		no cells end up empty.  because of the way i'm stepping through, i think that's the only type of error that could occur.  i'm kind of
		eager to get this working on...  it seems like it'll be interesting.  so i'll go ahead and bump it up behind bilocation and bivalue.

		the reason you only go 2 to 8 is that if you did 1, you'd be doing forcing chains and if you do 8, you'll force the 9th cell into a
		value and there's no purpose trying to figure out which option you'll choose when you only have one option.  i think, for this, i'll
		send a physical map and reuse that and let it expire and retry, etc... because it won't take much RAM.  we're talking about as many as
		8 levels... what's that, like 650 integers.  besides, it'll alieviate the problem of trying to undo my doings.  it'll be quicker and
		simpler just to declare that a certain thing will or won't work and then to toss that map and try the next one.

		now the thing i have to figure out is how i'm going to communicate that a thing didn't work.  i was thinking i might send a 
		vector<string> pointer or something.  i think i'd just push the sequences that didn't work.  that way, i could analyze them after i
		got out of the recursion and see if a thing was in the same position for a lot of these failures... that's another thing.  i'm going
		to have to figure out how to analyze the results!
	ALS + Death Blossom
		i think a good way to do this would be to go around finding almost locked sets.  then visit each cell and see how many als it intersects
		and if the cell can see anything that might be useful somehow.. hmm.  

		wait a second... isn't an almost locked subset equal to an almost naked subset?  that could come in handy!
	Finned Fish
	Sashimi Fish
	Franken Fish
	Mutant Fish
	Kraken Fish

i expect these three to work magic... they seem to work on principles completely different from what i already have implemented.  als and ase at least
	wings (the wings actually look like subsets of ase... so i should probably just go ahead and try to figure those out.)
	almost locked sets + death blossom
	aligned pair/triple/subset exclusion

	Something other than SuDoku

[ ] Wings (i really want this to be one of those techniques that can be generalized and scaled!!!)
	[ ] XY Wing	[ ] XYZ Wing	[ ] WXYZ Wing
[ ] ALS
	[ ] XZ		[ ] XY-Wing	[ ] Death Blossom
[ ] ASE
	[ ] Double	[ ] Triple	[ ] Other
*/

#include <cstring>
#include <curses.h>
#include <fstream>
#include <vector>

#include <stdlib.h>
#include <time.h>

using namespace std;

ofstream debug("suaide-v0.18.x debugging log.txt");

void show_candidates(int cell)
{
	for( int i = 0; i < 9; i++ )
		if( cell & (1<<i) )
			debug << i+1;
}

#define ALL_CANDIDATES 511
short CANDIDATES[]= {1,2,4,8,16,32,64,128,256};

int ybox = 4;//box dimensions
int xbox = 8;//box dimensions

#define null					-1//so i can keep up with links that are not valid in the menu

#define EVERYTHINGS_OK				1
#define SOMETHINGS_WRONG			11
#define VALID_SOLUTION				12

#define WHITE_ON_BLACK				20
#define BLACK_ON_WHITE				21

#define USER_MAKE_MAP				(1<<0)
#define DEBUG_MODE				(1<<1)
#define AUTOSOLVE				(1<<2)
#define DISQUALIFICATION_MODE			(1<<3)
#define TECH_MODE				(1<<4)
#define EXTREME_DEBUG_MODE			(1<<5)
#define SHOW_COMMANDS				(1<<6)
#define HIGHLIGHT_MODE				(1<<7)	//so the user can choose to highlight characters...
#define UPDATE_MODE				(1<<8)
#define HINT_MODE				(1<<9)	//let the user know what techniques were used to
#define MEDUSA_MODE				(1<<10) //draw the map as a medusa coloring map
#define SINGLE_PASS_MODE			(1<<11)	//solver makes a single pass instead of doing all it can
#define COLOR_MODE				(1<<12)	//you can specifically highlight candidates on the map
#define SHOW_HINT_MODE				(1<<13)	//highlight a possible move on the map
#define TRY_TO_SOLVE_MODE			(1<<14)	//user can see how effective the solver is on a book of puzzles

enum{
	CLASS_OTHER = 0,
	CLASS_BLOCK_INTERACTIONS,
	CLASS_SUBSETS,
	CLASS_COLORING,
	CLASS_FISH,
	CLASS_CHAINING,
	CLASS_UNIQUENESS
};

/**Other*/
#define TECH_OTHER_BASIC_ELIMINATION		(1<<0)
#define TECH_OTHER_BRUTE_FORCE			(1<<1)

/**Block Interactions*/
#define TECH_BLOCK_INTERACTIONS_BLOCK_RC	(1<<0)
#define TECH_BLOCK_INTERACTIONS_BLOCK_BLOCK	(1<<1)

/**Subsets*/
#define TECH_SUBSETS_NAKED_SINGLES		(1<<0)
#define TECH_SUBSETS_NAKED_DOUBLES		(1<<1)
#define TECH_SUBSETS_NAKED_TRIPLES		(1<<2)
#define TECH_SUBSETS_NAKED_QUADS		(1<<3)
#define TECH_SUBSETS_NAKED_OTHER		(1<<4)
#define TECH_SUBSETS_HIDDEN_SINGLES		(1<<5)
#define TECH_SUBSETS_HIDDEN_DOUBLES		(1<<6)
#define TECH_SUBSETS_HIDDEN_TRIPLES		(1<<7)
#define TECH_SUBSETS_HIDDEN_QUADS		(1<<8)
#define TECH_SUBSETS_HIDDEN_OTHER		(1<<9)

/**Coloring*/
#define TECH_COLORING_NORMAL			(1<<0)
#define TECH_COLORING_JUMPER			(1<<1)
#define TECH_COLORING_UPDATE			(1<<2)
#define TECH_COLORING_MEDUSA			(1<<3)
#define TECH_COLORING_BILOCATION		(1<<4)
#define TECH_COLORING_BIVALUE			(1<<5)

/**Fish*/
#define TECH_FISH_NORMAL_XWING			(1<<0)
#define TECH_FISH_NORMAL_SWORDFISH		(1<<1)
#define TECH_FISH_NORMAL_JELLYFISH		(1<<2)
#define TECH_FISH_NORMAL_SQUIRMBAG		(1<<3)
#define TECH_FISH_NORMAL_OTHER			(1<<4)
#define TECH_FISH_FINNED_XWING			(1<<5)
#define TECH_FISH_FINNED_SWORDFISH		(1<<6)
#define TECH_FISH_FINNED_JELLYFISH		(1<<7)
#define TECH_FISH_FINNED_SQUIRMBAG		(1<<8)
#define TECH_FISH_FINNED_OTHER			(1<<9)
#define TECH_FISH_SASHIMI_XWING			(1<<10)
#define TECH_FISH_SASHIMI_SWORDFISH		(1<<11)
#define TECH_FISH_SASHIMI_JELLYFISH		(1<<12)
#define TECH_FISH_SASHIMI_SQUIRMBAG		(1<<13)
#define TECH_FISH_SASHIMI_OTHER			(1<<14)

/**Chaining*/
#define TECH_CHAINING_FORCING			(1<<0)
#define TECH_CHAINING_XY			(1<<1)

/**Uniqueness*/
#define TECH_UNIQUENESS_ADP			(1<<0)//ADP = Almost Deadly Polygons
#define TECH_UNIQUENESS_ONE_SIDED_ADP		(1<<1)
#define TECH_UNIQUENESS_EXTENDED_ADP		(1<<2)

class Technique
{
public:
	char title[40];
	int responsibility_class;//there are more techniques than can fit in one integer, so i have to come up with types for more room.
	int responsibilities;//these are the techniques that get activated or deactivated when this technique is chosen
	//int tech_position;//so i can tell if this is currently selected... i don't think this is necessary... i'll just make an array.
	int x;//menu position x
	int y;//menu position y

	//this is so these techniques can point to one another... i know they're in an array together, so this should work just fine.
	int up;
	int down;
	int left;
	int right;

	Technique(const char *title, int responsibility_class, int responsibilities, int x, int y)
	{
		strcpy(this->title,title);
		this->responsibility_class = responsibility_class;
		this->responsibilities = responsibilities;
		this->x = x;
		this->y = y;

		//this is pretty much for debugging
		up = null;
		down = null;
		left = null;
		right = null;
	}

	Technique(const char *title, int responsibility_class, int responsibilities, int x, int y, int up, int down, int left, int right)
	{
		strcpy(this->title,title);
		this->responsibility_class = responsibility_class;
		this->responsibilities = responsibilities;
		this->x = x;
		this->y = y;
		this->up = up;
		this->down = down;
		this->left = left;
		this->right = right;
	}
};

Technique TECH[] = {
/*0*/		Technique(" Basic Elimination",CLASS_OTHER,TECH_OTHER_BASIC_ELIMINATION,1,75,null,1,null,null),
/*1*/		Technique(" Block Row/Column Interactions",CLASS_BLOCK_INTERACTIONS,TECH_BLOCK_INTERACTIONS_BLOCK_RC,2,75,0,2,null,null),
/*2*/		Technique(" Block Block Interactions",CLASS_BLOCK_INTERACTIONS,TECH_BLOCK_INTERACTIONS_BLOCK_BLOCK,3,75,1,3,null,null),
/*3*/		Technique(" Subsets",CLASS_SUBSETS,
			TECH_SUBSETS_NAKED_SINGLES | TECH_SUBSETS_NAKED_DOUBLES | TECH_SUBSETS_NAKED_TRIPLES | TECH_SUBSETS_NAKED_QUADS | TECH_SUBSETS_NAKED_OTHER |
			TECH_SUBSETS_HIDDEN_SINGLES | TECH_SUBSETS_HIDDEN_DOUBLES | TECH_SUBSETS_HIDDEN_TRIPLES | TECH_SUBSETS_HIDDEN_QUADS | TECH_SUBSETS_HIDDEN_OTHER,
			4,75,2,6,null,4),
/*4*/		Technique(" Naked",CLASS_SUBSETS,
			TECH_SUBSETS_NAKED_SINGLES | TECH_SUBSETS_NAKED_DOUBLES | TECH_SUBSETS_NAKED_TRIPLES | TECH_SUBSETS_NAKED_QUADS | TECH_SUBSETS_NAKED_OTHER,
			5,86,3,7,3,5),
/*5*/		Technique(" Hidden",CLASS_SUBSETS,
			TECH_SUBSETS_HIDDEN_SINGLES | TECH_SUBSETS_HIDDEN_DOUBLES | TECH_SUBSETS_HIDDEN_TRIPLES | TECH_SUBSETS_HIDDEN_QUADS | TECH_SUBSETS_HIDDEN_OTHER,
			5,98,3,8,4,null),
/*6*/		Technique(" Singles",CLASS_SUBSETS,TECH_SUBSETS_NAKED_SINGLES | TECH_SUBSETS_HIDDEN_SINGLES,6,77,3,9,null,7),
/*7*/		Technique("",CLASS_SUBSETS,TECH_SUBSETS_NAKED_SINGLES,6,90,4,10,6,8),
/*8*/		Technique("",CLASS_SUBSETS,TECH_SUBSETS_HIDDEN_SINGLES,6,102,5,11,7,null),
/*9*/		Technique(" Doubles",CLASS_SUBSETS,TECH_SUBSETS_NAKED_DOUBLES | TECH_SUBSETS_HIDDEN_DOUBLES,7,77,6,12,null,10),
/*10*/		Technique("",CLASS_SUBSETS,TECH_SUBSETS_NAKED_DOUBLES,7,90,7,13,9,11),
/*11*/		Technique("",CLASS_SUBSETS,TECH_SUBSETS_HIDDEN_DOUBLES,7,102,8,14,10,null),
/*12*/		Technique(" Triples",CLASS_SUBSETS,TECH_SUBSETS_NAKED_TRIPLES | TECH_SUBSETS_HIDDEN_TRIPLES,8,77,9,15,null,13),
/*13*/		Technique("",CLASS_SUBSETS,TECH_SUBSETS_NAKED_TRIPLES,8,90,10,16,12,14),
/*14*/		Technique("",CLASS_SUBSETS,TECH_SUBSETS_HIDDEN_TRIPLES,8,102,11,17,13,null),
/*15*/		Technique(" Quads",CLASS_SUBSETS,TECH_SUBSETS_NAKED_QUADS | TECH_SUBSETS_HIDDEN_QUADS,9,77,12,18,null,16),
/*16*/		Technique("",CLASS_SUBSETS,TECH_SUBSETS_NAKED_QUADS,9,90,13,19,15,17),
/*17*/		Technique("",CLASS_SUBSETS,TECH_SUBSETS_HIDDEN_QUADS,9,102,14,20,16,null),
/*18*/		Technique(" Other",CLASS_SUBSETS,TECH_SUBSETS_NAKED_OTHER | TECH_SUBSETS_HIDDEN_OTHER,10,77,15,21,null,19),
/*19*/		Technique("",CLASS_SUBSETS,TECH_SUBSETS_NAKED_OTHER,10,90,16,21,18,20),
/*20*/		Technique("",CLASS_SUBSETS,TECH_SUBSETS_HIDDEN_OTHER,10,102,17,21,19,null),
/*21*/		Technique(" Coloring",CLASS_COLORING,TECH_COLORING_NORMAL | TECH_COLORING_JUMPER | TECH_COLORING_UPDATE | TECH_COLORING_MEDUSA,11,75,18,22,null,22),
/*22*/		Technique(" Norm",CLASS_COLORING,TECH_COLORING_NORMAL,12,77,21,26,21,23),
/*23*/		Technique(" Multi",CLASS_COLORING,TECH_COLORING_JUMPER,12,87,21,26,22,24),
/*24*/		Technique(" Medusa",CLASS_COLORING,TECH_COLORING_MEDUSA,12,98,21,26,23,25),
/*25*/		Technique(" Imp",CLASS_COLORING,TECH_COLORING_UPDATE,12,110,21,26,24,null),
/*26*/		Technique(" Fish",CLASS_FISH,
			TECH_FISH_NORMAL_XWING | TECH_FISH_NORMAL_SWORDFISH | TECH_FISH_NORMAL_JELLYFISH | TECH_FISH_NORMAL_SQUIRMBAG | TECH_FISH_NORMAL_OTHER |
			TECH_FISH_FINNED_XWING | TECH_FISH_FINNED_SWORDFISH | TECH_FISH_FINNED_JELLYFISH | TECH_FISH_FINNED_SQUIRMBAG | TECH_FISH_FINNED_OTHER |
			TECH_FISH_SASHIMI_XWING | TECH_FISH_SASHIMI_SWORDFISH | TECH_FISH_SASHIMI_JELLYFISH | TECH_FISH_SASHIMI_SQUIRMBAG | TECH_FISH_SASHIMI_OTHER,
			13,75,22,30,null,27),
/*27*/		Technique(" Normal",CLASS_FISH,
			TECH_FISH_NORMAL_XWING | TECH_FISH_NORMAL_SWORDFISH | TECH_FISH_NORMAL_JELLYFISH | TECH_FISH_NORMAL_SQUIRMBAG | TECH_FISH_NORMAL_OTHER,
			14,88,26,31,26,28),
/*28*/		Technique(" Finned",CLASS_FISH,
			TECH_FISH_FINNED_XWING | TECH_FISH_FINNED_SWORDFISH | TECH_FISH_FINNED_JELLYFISH | TECH_FISH_FINNED_SQUIRMBAG | TECH_FISH_FINNED_OTHER,
			14,100,26,32,27,29),
/*29*/		Technique(" Sashimi",CLASS_FISH,
			TECH_FISH_SASHIMI_XWING | TECH_FISH_SASHIMI_SWORDFISH | TECH_FISH_SASHIMI_JELLYFISH | TECH_FISH_SASHIMI_SQUIRMBAG | TECH_FISH_SASHIMI_OTHER,
			14,112,26,33,28,null),
/*30*/		Technique(" X-Wing",CLASS_FISH,TECH_FISH_NORMAL_XWING | TECH_FISH_FINNED_XWING | TECH_FISH_SASHIMI_XWING,
			15,77,26,34,null,31),
/*31*/		Technique("",CLASS_FISH,TECH_FISH_NORMAL_XWING,15,93,27,35,30,32),
/*32*/		Technique("",CLASS_FISH,TECH_FISH_FINNED_XWING,15,105,28,36,31,33),
/*33*/		Technique("",CLASS_FISH,TECH_FISH_SASHIMI_XWING,15,117,29,37,32,null),
/*34*/		Technique(" Swordfish",CLASS_FISH,TECH_FISH_NORMAL_SWORDFISH | TECH_FISH_FINNED_SWORDFISH | TECH_FISH_SASHIMI_SWORDFISH,
			16,77,30,38,null,35),
/*35*/		Technique("",CLASS_FISH,TECH_FISH_NORMAL_SWORDFISH,16,93,31,39,34,36),
/*36*/		Technique("",CLASS_FISH,TECH_FISH_FINNED_SWORDFISH,16,105,32,40,35,37),
/*37*/		Technique("",CLASS_FISH,TECH_FISH_SASHIMI_SWORDFISH,16,117,33,41,39,null),
/*38*/		Technique(" Jellyfish",CLASS_FISH,TECH_FISH_NORMAL_JELLYFISH | TECH_FISH_FINNED_JELLYFISH | TECH_FISH_SASHIMI_JELLYFISH,
			17,77,34,42,null,39),
/*39*/		Technique("",CLASS_FISH,TECH_FISH_NORMAL_JELLYFISH,17,93,35,43,38,40),
/*40*/		Technique("",CLASS_FISH,TECH_FISH_FINNED_JELLYFISH,17,105,36,44,39,41),
/*41*/		Technique("",CLASS_FISH,TECH_FISH_SASHIMI_JELLYFISH,17,117,37,45,40,null),
/*42*/		Technique(" Squirmbag",CLASS_FISH,TECH_FISH_NORMAL_SQUIRMBAG | TECH_FISH_FINNED_SQUIRMBAG | TECH_FISH_SASHIMI_SQUIRMBAG,
			18,77,38,46,null,43),
/*43*/		Technique("",CLASS_FISH,TECH_FISH_NORMAL_SQUIRMBAG,18,93,39,47,42,44),
/*44*/		Technique("",CLASS_FISH,TECH_FISH_FINNED_SQUIRMBAG,18,105,40,48,43,45),
/*45*/		Technique("",CLASS_FISH,TECH_FISH_SASHIMI_SQUIRMBAG,18,117,41,49,44,null),
/*46*/		Technique(" Other",CLASS_FISH,TECH_FISH_NORMAL_OTHER | TECH_FISH_FINNED_OTHER | TECH_FISH_SASHIMI_OTHER,
			19,77,42,50,null,47),
/*47*/		Technique("",CLASS_FISH,TECH_FISH_NORMAL_OTHER,19,93,43,50,46,48),
/*48*/		Technique("",CLASS_FISH,TECH_FISH_FINNED_OTHER,19,105,44,50,47,49),
/*49*/		Technique("",CLASS_FISH,TECH_FISH_SASHIMI_OTHER,19,117,45,50,48,null),
/*50*/		Technique(" Forcing Chains",CLASS_CHAINING,TECH_CHAINING_FORCING,20,75,46,51,null,null),
/*51*/		Technique(" XY-Chains",CLASS_CHAINING,TECH_CHAINING_XY,21,75,50,52,null,null),
/*52*/		Technique(" Almost Deadly Polygons",CLASS_UNIQUENESS,TECH_UNIQUENESS_ADP | TECH_UNIQUENESS_ONE_SIDED_ADP | TECH_UNIQUENESS_EXTENDED_ADP,
			22,75,51,53,null,53),
/*53*/		Technique(" Normal",CLASS_UNIQUENESS,TECH_UNIQUENESS_ADP,23,77,52,56,52,54),
/*54*/		Technique(" Extended",CLASS_UNIQUENESS,TECH_UNIQUENESS_EXTENDED_ADP,23,89,52,56,53,55),
/*55*/		Technique(" One-Sided",CLASS_UNIQUENESS,TECH_UNIQUENESS_ONE_SIDED_ADP,23,103,52,56,54,null),
/*56*/		Technique(" Bilocation",CLASS_COLORING,TECH_COLORING_BILOCATION,24,75,53,58,null,57),
/*57*/		Technique(" Bivalue",CLASS_COLORING,TECH_COLORING_BIVALUE,24,100,56,58,56,null),
/*58*/		Technique(" Brute Force",CLASS_OTHER,TECH_OTHER_BRUTE_FORCE,25,75,56,null,null,null)
};

short BIT_COUNTER[512]={0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8, 5, 6, 6, 7, 6, 7, 7, 8, 6, 7, 7, 8, 7, 8, 8, 9};

short BITS_LOOKUP[257]={0,0,1,0,2,0,0,0,3,0,0,0,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,//32
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,//64
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//96
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,7,//128
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//160
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//192
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,//224
			0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,8};//256

//these definitions help make the process simpler and less confusing
short ROWS[9][9]={		{ 0, 1, 2, 3, 4, 5, 6, 7, 8},
				{ 9,10,11,12,13,14,15,16,17},
				{18,19,20,21,22,23,24,25,26},
				{27,28,29,30,31,32,33,34,35},
				{36,37,38,39,40,41,42,43,44},
				{45,46,47,48,49,50,51,52,53},
				{54,55,56,57,58,59,60,61,62},
				{63,64,65,66,67,68,69,70,71},
				{72,73,74,75,76,77,78,79,80}	};


short ROW_LOOKUP[81]={		 0, 0, 0, 0, 0, 0, 0, 0, 0,
				 1, 1, 1, 1, 1, 1, 1, 1, 1,
				 2, 2, 2, 2, 2, 2, 2, 2, 2,
				 3, 3, 3, 3, 3, 3, 3, 3, 3,
				 4, 4, 4, 4, 4, 4, 4, 4, 4,
				 5, 5, 5, 5, 5, 5, 5, 5, 5,
				 6, 6, 6, 6, 6, 6, 6, 6, 6,
				 7, 7, 7, 7, 7, 7, 7, 7, 7,
				 8, 8, 8, 8, 8, 8, 8, 8, 8	};


short COLUMNS[9][9]={		{0, 9,18,27,36,45,54,63,72},
				{1,10,19,28,37,46,55,64,73},
				{2,11,20,29,38,47,56,65,74},
				{3,12,21,30,39,48,57,66,75},
				{4,13,22,31,40,49,58,67,76},
				{5,14,23,32,41,50,59,68,77},
				{6,15,24,33,42,51,60,69,78},
				{7,16,25,34,43,52,61,70,79},
				{8,17,26,35,44,53,62,71,80}	};


short COLUMN_LOOKUP[81]={	 0, 1, 2, 3, 4, 5, 6, 7, 8,
				 0, 1, 2, 3, 4, 5, 6, 7, 8,
				 0, 1, 2, 3, 4, 5, 6, 7, 8,
				 0, 1, 2, 3, 4, 5, 6, 7, 8,
				 0, 1, 2, 3, 4, 5, 6, 7, 8,
				 0, 1, 2, 3, 4, 5, 6, 7, 8,
				 0, 1, 2, 3, 4, 5, 6, 7, 8,
				 0, 1, 2, 3, 4, 5, 6, 7, 8,
				 0, 1, 2, 3, 4, 5, 6, 7, 8	};


short BLOCKS[9][9]={		{ 0, 1, 2, 9,10,11,18,19,20},
				{ 3, 4, 5,12,13,14,21,22,23},
				{ 6, 7, 8,15,16,17,24,25,26},
				{27,28,29,36,37,38,45,46,47},
				{30,31,32,39,40,41,48,49,50},
				{33,34,35,42,43,44,51,52,53},
				{54,55,56,63,64,65,72,73,74},
				{57,58,59,66,67,68,75,76,77},
				{60,61,62,69,70,71,78,79,80}	};


short BLOCK_LOOKUP[81]={	 0, 0, 0, 1, 1, 1, 2, 2, 2,
				 0, 0, 0, 1, 1, 1, 2, 2, 2,
				 0, 0, 0, 1, 1, 1, 2, 2, 2,
				 3, 3, 3, 4, 4, 4, 5, 5, 5,
				 3, 3, 3, 4, 4, 4, 5, 5, 5,
				 3, 3, 3, 4, 4, 4, 5, 5, 5,
				 6, 6, 6, 7, 7, 7, 8, 8, 8,
				 6, 6, 6, 7, 7, 7, 8, 8, 8,
				 6, 6, 6, 7, 7, 7, 8, 8, 8	};

//this is for block row/column and block/block interactions
short BLOCK_ROW_FRIENDS[9][2]={		{1, 2}, {0, 2}, {0, 1},
					{4, 5}, {3, 5}, {3, 4},
					{7, 8}, {6, 8}, {6, 7}		};


short BLOCK_COLUMN_FRIENDS[9][2]={	{3, 6}, {4, 7}, {5, 8},
					{0, 6}, {1, 7}, {2, 8},
					{0, 3}, {1, 4}, {2, 5}		};

//this is spelling things out... [block number][row number][constituents of that row]
short BLOCK_ROWS[9][3][3]={	{	{0,1,2},	{9,10,11},	{18,19,20}	},//block 1
				{	{3,4,5},	{12,13,14},	{21,22,23}	},//block 2
				{	{6,7,8},	{15,16,17},	{24,25,26}	},//block 3

				{	{27,28,29},	{36,37,38},	{45,46,47}	},//etc...
				{	{30,31,32},	{39,40,41},	{48,49,50}	},
				{	{33,34,35},	{42,43,44},	{51,52,53}	},

				{	{54,55,56},	{63,64,65},	{72,73,74}	},
				{	{57,58,59},	{66,67,68},	{75,76,77},	},
				{	{60,61,62},	{69,70,71},	{78,79,80}	}	};

//this is spelling things out... [block number][column number][constituents of that column]
short BLOCK_COLUMNS[9][3][3]={	{	{0,9,18},	{1,10,19},	{2,11,20}	},//block 1
				{	{3,12,21},	{4,13,22},	{5,14,23}	},//block 2
				{	{6,15,24},	{7,16,25},	{8,17,26}	},//block 3

				{	{27,36,45},	{28,37,46},	{29,38,47}	},//etc...
				{	{30,39,48},	{31,40,49},	{32,41,50}	},
				{	{33,42,51},	{34,43,52},	{35,44,53}	},

				{	{54,63,72},	{55,64,73},	{56,65,74}	},
				{	{57,66,75},	{58,67,76},	{59,68,77}	},
				{	{60,69,78},	{61,70,79},	{62,71,80}	}	};

short CELL_FRIENDS[81][20]={
{1,9,2,18,3,27,4,36,10,5,45,11,6,54,7,63,19,8,72,20},{0,10,2,19,3,28,9,4,37,5,46,11,6,55,18,7,64,8,73,20},
{0,1,11,20,3,29,9,4,38,10,5,47,6,56,18,7,65,19,8,74},{0,1,12,4,2,21,5,30,39,13,48,14,6,57,7,66,22,8,75,23},
{0,3,1,13,2,22,5,31,12,40,49,14,6,58,21,7,67,8,76,23},{0,3,1,14,4,2,23,32,12,41,13,50,6,59,21,7,68,22,8,77},
{0,1,15,7,2,24,8,3,33,4,42,16,5,51,17,60,69,25,78,26},{0,6,1,16,2,25,8,3,34,15,4,43,5,52,17,61,24,70,79,26},
{0,6,1,17,7,2,26,3,35,15,4,44,16,5,53,62,24,71,25,80},{0,10,1,11,18,2,12,27,13,36,14,45,15,54,16,63,19,17,72,20},
{9,1,0,11,19,2,12,28,13,37,14,46,15,55,18,16,64,17,73,20},{9,2,0,10,1,20,12,29,13,38,14,47,15,56,18,16,65,19,17,74},
{9,3,10,4,11,21,5,30,13,39,14,48,15,57,16,66,22,17,75,23},{9,4,3,10,11,22,5,12,31,40,14,49,15,58,21,16,67,17,76,23},
{9,5,3,10,4,11,23,12,32,13,41,50,15,59,21,16,68,22,17,77},{9,6,10,7,11,24,8,12,33,13,42,16,14,51,17,60,69,25,78,26},
{9,7,6,10,11,25,8,12,34,15,13,43,14,52,17,61,24,70,79,26},{9,8,6,10,7,11,26,12,35,15,13,44,16,14,53,62,24,71,25,80},
{0,19,9,1,20,2,21,27,22,36,10,23,45,11,24,54,25,63,26,72},{18,1,0,10,20,2,21,28,9,22,37,23,46,11,24,55,25,64,26,73},
{18,2,0,19,11,1,21,29,9,22,38,10,23,47,24,56,25,65,26,74},{18,3,19,12,4,20,5,30,22,39,13,23,48,14,24,57,25,66,26,75},
{18,4,3,19,13,20,5,21,31,12,40,23,49,14,24,58,25,67,26,76},{18,5,3,19,14,4,20,21,32,12,22,41,13,50,24,59,25,68,26,77},
{18,6,19,15,7,20,8,21,33,22,42,16,23,51,17,60,25,69,26,78},{18,7,6,19,16,20,8,21,34,15,22,43,23,52,17,24,61,70,26,79},
{18,8,6,19,17,7,20,21,35,15,22,44,16,23,53,24,62,25,71,80},{0,28,9,29,18,30,36,31,37,32,45,38,33,54,34,63,46,35,72,47},
{27,1,10,29,19,30,36,31,37,32,46,38,33,55,45,34,64,35,73,47},{27,2,28,11,20,30,36,31,38,37,32,47,33,56,45,34,65,46,35,74},
{27,3,28,12,31,29,21,32,39,40,48,41,33,57,34,66,49,35,75,50},{27,4,30,28,13,29,22,32,39,40,49,41,33,58,48,34,67,35,76,50},
{27,5,30,28,14,31,29,23,39,41,40,50,33,59,48,34,68,49,35,77},{27,6,28,15,34,29,24,35,30,42,31,43,32,51,44,60,69,52,78,53},
{27,7,33,28,16,29,25,35,30,42,31,43,32,52,44,61,51,70,79,53},{27,8,33,28,17,34,29,26,30,42,31,44,43,32,53,62,51,71,52,80},
{0,27,37,9,28,38,18,29,39,40,41,45,42,54,43,63,46,44,72,47},{36,1,27,10,28,38,19,29,39,40,41,46,42,55,45,43,64,44,73,47},
{36,2,27,37,11,28,20,29,39,40,41,47,42,56,45,43,65,46,44,74},{36,3,30,37,12,31,38,21,32,40,41,48,42,57,43,66,49,44,75,50},
{36,4,30,37,13,31,38,22,32,39,41,49,42,58,48,43,67,44,76,50},{36,5,30,37,14,31,38,23,32,39,40,50,42,59,48,43,68,49,44,77},
{36,6,33,37,15,34,38,24,35,39,40,43,41,51,44,60,69,52,78,53},{36,7,33,37,16,34,38,25,35,39,42,40,41,52,44,61,51,70,79,53},
{36,8,33,37,17,34,38,26,35,39,42,40,43,41,53,62,51,71,52,80},{0,27,46,9,28,47,18,29,48,36,49,37,50,38,51,54,52,63,53,72},
{45,1,27,10,28,47,19,29,48,36,49,37,50,38,51,55,52,64,53,73},{45,2,27,46,11,28,20,29,48,36,49,38,37,50,51,56,52,65,53,74},
{45,3,30,46,12,31,47,21,32,39,49,40,50,41,51,57,52,66,53,75},{45,4,30,46,13,31,47,22,32,48,39,40,50,41,51,58,52,67,53,76},
{45,5,30,46,14,31,47,23,32,48,39,49,41,40,51,59,52,68,53,77},{45,6,33,46,15,34,47,24,35,48,42,49,43,50,44,60,52,69,53,78},
{45,7,33,46,16,34,47,25,35,48,42,49,43,50,44,51,61,70,53,79},{45,8,33,46,17,34,47,26,35,48,42,49,44,43,50,51,62,52,71,80},
{0,55,9,56,18,57,27,63,58,36,64,59,45,65,60,72,61,73,62,74},{54,1,10,56,19,57,28,63,58,37,64,59,46,65,60,72,61,73,62,74},
{54,2,55,11,20,57,29,63,58,38,64,59,47,65,60,72,61,73,62,74},{54,3,55,12,58,56,21,59,30,66,39,67,48,68,60,75,61,76,62,77},
{54,4,57,55,13,56,22,59,31,66,40,67,49,68,60,75,61,76,62,77},{54,5,57,55,14,58,56,23,32,66,41,67,50,68,60,75,61,76,62,77},
{54,6,55,15,61,56,24,62,57,33,69,58,42,70,59,51,71,78,79,80},{54,7,60,55,16,56,25,62,57,34,69,58,43,70,59,52,71,78,79,80},
{54,8,60,55,17,61,56,26,57,35,69,58,44,70,59,53,71,78,79,80},{0,54,64,9,55,65,18,56,66,27,67,36,68,45,69,72,70,73,71,74},
{63,1,54,10,55,65,19,56,66,28,67,37,68,46,69,72,70,73,71,74},{63,2,54,64,11,55,20,56,66,29,67,38,68,47,69,72,70,73,71,74},
{63,3,57,64,12,58,65,21,59,30,67,39,68,48,69,75,70,76,71,77},{63,4,57,64,13,58,65,22,59,66,31,40,68,49,69,75,70,76,71,77},
{63,5,57,64,14,58,65,23,59,66,32,67,41,50,69,75,70,76,71,77},{63,6,60,64,15,61,65,24,62,66,33,67,42,70,68,51,71,78,79,80},
{63,7,60,64,16,61,65,25,62,66,34,69,67,43,68,52,71,78,79,80},{63,8,60,64,17,61,65,26,62,66,35,69,67,44,70,68,53,78,79,80},
{0,54,73,9,55,74,18,56,75,27,63,76,36,64,77,45,65,78,79,80},{72,1,54,10,55,74,19,56,75,28,63,76,37,64,77,46,65,78,79,80},
{72,2,54,73,11,55,20,56,75,29,63,76,38,64,77,47,65,78,79,80},{72,3,57,73,12,58,74,21,59,30,66,76,39,67,77,48,68,78,79,80},
{72,4,57,73,13,58,74,22,59,75,31,66,40,67,77,49,68,78,79,80},{72,5,57,73,14,58,74,23,59,75,32,66,76,41,67,50,68,78,79,80},
{72,6,60,73,15,61,74,24,62,75,33,69,76,42,70,77,51,71,79,80},{72,7,60,73,16,61,74,25,62,75,34,69,76,43,70,77,52,71,78,80},
{72,8,60,73,17,61,74,26,62,75,35,69,76,44,70,77,53,71,78,79}	};

class Cell
{
public:
	int row;//		0
	int column;//		1
	int block;//		2
	int dimension;//	3
};

class ColoringBook
{
public:
	bool hard_linked[9][81];//keep track of link status
	Cell hard_links[9][81];//if something happens to be hard linked, this is where those links can be found
	bool tried[9][81];//keeps track of the hard chains that have been visited.  i plan to enforce that by running through hard links first, then setting the original_chain bool or whatever i call it to false before trying other group members
};

///yes, i know... i'm really wasting more than half of these integers.  Deal with it!
class Puzzle
{
public:	//everything will always follow the format (row, column)
	int original_map[81];//the map entered or loaded by the user
	int solution_map[81];//the solution map
	int suggestion_map[81];//when the solution functions come up with a naked single, it goes here.
	int highlight_map[81];//allows the user to highlight certain parts of the map
	int candidate_map[81];//the working map, this is the one that gets displayed.
	int historical_map[81];
	ColoringBook book;//this is the basis of the coloring techniques
	bool updating;
	int hint[81];//this is to tell the user where moves can be made if he wants the help

	int disqualification_map[81];//the user can use this to manually disqualify candidates

	//the user does not actually mess with the suggestion map.  only the original, solution and highlight maps.

	void clear_highlights()
	{
		for( int i = 0; i < 81; i++ )
			highlight_map[i] = false;
	}

	void reset()
	{
		for( int i = 0; i < 81; i++ )
			solution_map[i] = original_map[i];
	}
	//void rollback(int moves){}

	int row_contains(short row, short value)//count of value
	{
		int result = 0;
		for( int i = 0; i < 9; i++ )
			if( candidate_map[ROWS[row][i]] & value )
				result ++;
		return result;
	}

	int column_contains(short column, short value)
	{
		int result = 0;
		for( int i = 0; i < 9; i++ )
			if( candidate_map[COLUMNS[column][i]] & value )
				result ++;
		return result;
	}

	int block_contains(short block, short value)
	{
		int result = 0;
		for( int i = 0; i < 9; i++ )
			if( candidate_map[BLOCKS[block][i]] & value )
				result ++;
		return result;
	}

	int block_subrow_contains(short block, short row, short value)
	{
		int result = 0;
		for( int i = 0; i < 3; i++ )
			if( candidate_map[BLOCK_ROWS[block][row][i]] & value )
				result ++;
		return result;
	}

	int block_subcolumn_contains(short block, short column, short value )
	{
		int result = 0;
		for( int i = 0; i < 3; i++ )
			if( candidate_map[BLOCK_COLUMNS[block][column][i]] & value )
				result ++;
		return result;
	}

	void prep_candidate_map()
	{
		if( updating )
			return;

		//set all candidates as valid
		for( int i = 0; i < 81; i++ )
			candidate_map[i] = ALL_CANDIDATES;
		//go through and, based on the solution map, knock out candidates
		for( int i = 0; i < 81; i++ )
		{
			for( int value = 0; value < 9; value++ )
			{
				if( solution_map[i] == (value+1) )
				{
					//not having this line was getting me into trouble with hidden singles
					candidate_map[i] = 0;
					for( int j = 0; j < 9; j++ )
					{
						//find out what row/column/block we're looking at... and do the switcheroo
						candidate_map[ROWS[ROW_LOOKUP[i]][j]] &= ~(1<<value);
						candidate_map[COLUMNS[COLUMN_LOOKUP[i]][j]] &= ~(1<<value);
						candidate_map[BLOCKS[BLOCK_LOOKUP[i]][j]] &= ~(1<<value);
					}
				}
			}
		}

		for( int i = 0; i < 81; i++ )
			candidate_map[i] &= ~disqualification_map[i];
	}

	void reset_candidate_map()
	{
		for( int i = 0; i < 81; i++ )
			candidate_map[i] = ALL_CANDIDATES;
		for( int i = 0; i < 81; i++ )
			candidate_map[i] &= ~disqualification_map[i];
	}

	void dq_reset()
	{
		for( int i = 0; i < 81; i++ )
			disqualification_map[i] = 0;
	}

	void generate_suggestion_map()
	{
		for( int i = 0; i < 81; i++ )
			suggestion_map[i] = 0;
		for( int i = 0; i < 81; i++ )
		{
			int bit_count = 0;
			for( int value = 0; value < 9; value++ )
				if( candidate_map[i] & (1<<value) )
					bit_count++;
			//if( bit_count == 0 ) //then something's broken or selected in the solution...
				
			if( bit_count == 1 )
				for( int identity = 0; identity < 9; identity++ )
				{
					if( CANDIDATES[identity] == candidate_map[i] )
						suggestion_map[i] = identity+1;
				}
		}
	}

	void clear_suggestion_map()
	{
		for( int i = 0; i < 81; i++ )
			suggestion_map[i] = 0;
	}

	void autosolve()
	{
		for( int i = 0; i < 81; i++ )
			if( suggestion_map[i] )
				solution_map[i] = suggestion_map[i];
	}

	void init()
	{
		for( int i = 0; i < 81; i++ )
		{
			original_map[i] = solution_map[i] = suggestion_map[i] = 
			highlight_map[i] = candidate_map[i] = disqualification_map[i] = 0;
			hint[i] = 0;
		}
		updating = false;
	}

	//i'm but a simple program.  i can really only understand puzzles either all as one line or in a 9x9 grid.
	void get_puzzle_from(char *puzzleloc)
	{
		fstream i;
		i.open(puzzleloc, ios::in);
		char line[85];
		i >> line;

		init();
		if( strlen(line) == 81 )
		{
			for( int a = 0; a < 81; a++ )
				if( line[a] > 48 && line[a] < 48+10 )
					original_map[a] = solution_map[a] = line[a]-48;
		}
		else//assume it's 9x9
		{
		  if( strcmp("[Puzzle]",line) == 0 )//this conditional allows for importing sadman puzzles
				i >> line;
			for( int a = 0; a < 9; a++ )
				if( line[a] > 48 && line[a] < 48+10 )
					original_map[a] = solution_map[a] = line[a]-48;
			for( int a = 1; a < 9; a++ )
			{
				i >> line;
				for( int b = 0; b < 9; b++ )
					if( line[b] > 48 && line[b] < 48+10 )
						original_map[a*9+b] = solution_map[a*9+b] = line[b]-48;
			}
		}
	}

	void put_puzzle_in(char *puzzleloc)
	{
		fstream o;
		o.open(puzzleloc, ios::out);
		for( int i = 0; i < 81; i++ )
			o << original_map[i];
		o << endl;
	}

	bool update()
	{
		updating = false;
		for( int i = 0; i < 81; i++ )
			if( historical_map[i] != candidate_map[i] )
			{
				historical_map[i] = candidate_map[i];
				updating = true;
			}
		return updating;
	}

	int bit_count(int position)
	{
		return BIT_COUNTER[candidate_map[position]];
	}

	//ok, i give this another looking over, but i think i'm ready to move on to the actual coloring function.
	//this is nice because there should be much less overhead associated with this initial function than with the older one.
	//as we've seen, the great part about truthiness is that it's true.  hard links are one per definition and every participant
	//in a chain knows it's participating and will be cooperative.  woo!  makes my job simpler.  hello, logic!
	void build_coloringbook(int *candidate_map)
	{
		for( int candidate = 0; candidate < 9; candidate++ )
		{
			int value = (1<<candidate);
			for( int position = 0; position < 81; position++ )
			{
				book.hard_linked[candidate][position] = false;
				book.hard_links[candidate][position].row = null;
				book.hard_links[candidate][position].column = null;
				book.hard_links[candidate][position].block = null;
				book.hard_links[candidate][position].dimension = null;

				int row = ROW_LOOKUP[position];
				int column = COLUMN_LOOKUP[position];
				int block = BLOCK_LOOKUP[position];

				if( candidate_map[position] & value )//then look for things to link with
				{
					if( row_contains(row,value) == 2 )//then hard link the other guy
						//look for the other instance and call it a hard link
						for( int possible_position = 0; possible_position < 9; possible_position++ )
							//if this is the case, we've found the other guy and can hard link him.
							if( (candidate_map[ROWS[row][possible_position]] & value) && (position != ROWS[row][possible_position]) )
									book.hard_links[candidate][position].row = ROWS[row][possible_position];

					if( column_contains(column,value) == 2 )
						for( int possible_position = 0; possible_position < 9; possible_position++ )
							if( (candidate_map[COLUMNS[column][possible_position]] & value) && (position != COLUMNS[column][possible_position]) )
								book.hard_links[candidate][position].column = COLUMNS[column][possible_position];

					if( block_contains(block,value) == 2 )
						for( int possible_position = 0; possible_position < 9; possible_position++ )
							if( (candidate_map[BLOCKS[block][possible_position]] & value) && (position != BLOCKS[block][possible_position]) )
								book.hard_links[candidate][position].block = BLOCKS[block][possible_position];

					//we've gotten this far, so we know our value is one of the two values present here, let's find the other
					if( BIT_COUNTER[candidate_map[position]] == 2 )
						for( int finder = 0; finder < 9; finder++ )
							//if you find someone and he's not you, he must be the other guy
							if( candidate_map[position] & (1<<finder) && (1<<finder) != value )
								book.hard_links[candidate][position].dimension = finder;

					if( 	book.hard_links[candidate][position].row != null || 
						book.hard_links[candidate][position].column != null || 
						book.hard_links[candidate][position].block != null || 
						book.hard_links[candidate][position].dimension != null )
						book.hard_linked[candidate][position] = true;
				}
///it looks like this function is working just fine.
				/*if( book.hard_linked[candidate][position] == true )
debug << "Candidate=" << candidate+1 << "\tPosition=" << position+1 << "\tRowLink=" << book.hard_links[candidate][position].row+1 << 
"\tColLink=" << book.hard_links[candidate][position].column+1 << "\tBlkLink=" << book.hard_links[candidate][position].block+1 <<
"\tDimLink=" << book.hard_links[candidate][position].dimension+1 << "\n";*/
			}
		}
//debug.flush();
	}

	void build_coloringbook()
	{
		build_coloringbook(candidate_map);
	}

	void clear_hints()
	{
		for( int i = 0; i < 81; i++ )
			hint[i] = false;
	}
};

/**god damn... this is really messy.  it's like a fucking clusterbomb went off in my code.  i'll get this organized one day...*/

int row_contains(int *candidate_map, short row, short value)//count of value
{
	int result = 0;
	for( int i = 0; i < 9; i++ )
		if( candidate_map[ROWS[row][i]] & value )
			result ++;
	return result;
}

int column_contains(int *candidate_map, short column, short value)
{
	int result = 0;
	for( int i = 0; i < 9; i++ )
		if( candidate_map[COLUMNS[column][i]] & value )
			result ++;
	return result;
}

int block_contains(int *candidate_map, short block, short value)
{
	int result = 0;
	for( int i = 0; i < 9; i++ )
		if( candidate_map[BLOCKS[block][i]] & value )
			result ++;
	return result;
}

int block_subrow_contains(int *candidate_map, short block, short row, short value)
{
	int result = 0;
	for( int i = 0; i < 3; i++ )
		if( candidate_map[BLOCK_ROWS[block][row][i]] & value )
			result ++;
	return result;
}

int block_subcolumn_contains(int *candidate_map, short block, short column, short value )
{
	int result = 0;
	for( int i = 0; i < 3; i++ )
		if( candidate_map[BLOCK_COLUMNS[block][column][i]] & value )
			result ++;
	return result;
}

using namespace std;

void draw_puzzle(int ybox, int xbox);
void fill_blocks(int ybox, int xbox, int cury, int curx, Puzzle *p, int user_actions);
void medusa_map(int ybox, int xbox, int cury, int curx, Puzzle *p, int user_actions);
void solver_and_menu(int user_actions, int *techniques, int tech_options_position, Puzzle *p);
void process_and_display(Puzzle *p,int cursor_position_x,int cursor_position_y,int *techniques,int user_actions,int tech_options_position,int highlight_these,int *trio);
int brute_force(int *mymap, Puzzle *p);///return the number of valid solutions.  so count through all possibilities making naked singles elimination

bool everything_seems_ok(int *solution, int *pencilmarks);
bool valid_solution(Puzzle *p);

vector <string> puzzles;
int current_puzzle = 0;

void load(char *path)
{
	puzzles.resize(0);
	current_puzzle = 0;

	fstream i;
	i.open(path, ios::in);
	string line;

	i >> line;
	do
	{
		puzzles.push_back(line);
		i >> line;
	}while(!i.eof());
}

int main()
{
	initscr();
	start_color();

	init_pair(1,COLOR_YELLOW,COLOR_BLACK);//this outlines the blocks

	init_pair(2,COLOR_BLACK,COLOR_YELLOW);//this shows parts of the original puzzle
	init_pair(3,COLOR_BLACK,COLOR_GREEN);//this shows parts of the solved puzzle
	init_pair(4,COLOR_BLACK,COLOR_MAGENTA);//this shows where there are move suggestions

	init_pair(5,COLOR_WHITE,COLOR_RED);//this shows the currently highlighted block

	init_pair(6,COLOR_YELLOW,COLOR_WHITE);//this shows when the cursor is over a block of the original map
	init_pair(7,COLOR_GREEN,COLOR_WHITE);//this shows when the cursor is over a block of the solution map
	init_pair(8,COLOR_MAGENTA,COLOR_WHITE);//this shows when the cursor is over a block of the suggestion map

	init_pair(9,COLOR_BLACK,COLOR_CYAN);//this is for blocks the user has highlighted
	init_pair(10,COLOR_CYAN,COLOR_BLUE);//when the cursor moves over a highlighted block

	init_pair(BLACK_ON_WHITE,COLOR_BLACK,COLOR_WHITE);//for highlighted text

	init_pair(11,COLOR_WHITE,COLOR_BLUE);//for naked singles in medusa view mode
	init_pair(12,COLOR_CYAN,COLOR_BLUE);//for the highlighted version of this

	init_pair(13,COLOR_BLACK,COLOR_BLACK);//maybe this will be helpful hiding keypresses

	int tech_options_position = 0;
	int cursor_position_x = 4;
	int cursor_position_y = 4;
	Puzzle p;
	p.init();
	p.prep_candidate_map();

	int user_actions = 0;
	int techniques[] = {0,0,0,0,0,0,0};
	int highlight_these = 0;

	int trio[3] = {0,0,0};//solved, unsolved, invalid

	solver_and_menu(user_actions, techniques, tech_options_position, &p);

	draw_puzzle(ybox, xbox);
	//medusa_map(ybox, xbox, &p);
	fill_blocks(ybox, xbox, cursor_position_y, cursor_position_x, &p, 0);
	refresh();

	int ch = 0;
	keypad(stdscr, true);
	while((ch = getch()) != KEY_F(1))
	{
		switch(ch)
		{
			case KEY_LEFT:
				if( user_actions & TECH_MODE )
				{
					if( TECH[tech_options_position].left != null )
						tech_options_position = TECH[tech_options_position].left;
				}
				else
					if( cursor_position_x > 0 )
						cursor_position_x--;
				break;
			case KEY_RIGHT:
				if( user_actions & TECH_MODE )
				{
					if( TECH[tech_options_position].right != null )
						tech_options_position = TECH[tech_options_position].right;
				}
				else
					if( cursor_position_x < 8 )
						cursor_position_x++;
				break;
			case KEY_UP:
				if( user_actions & TECH_MODE )
				{
					if( TECH[tech_options_position].up != null )
						tech_options_position = TECH[tech_options_position].up;
				}
				else
					if( cursor_position_y > 0 )
						cursor_position_y--;
				break;
			case KEY_DOWN:
				if( user_actions & TECH_MODE )
				{
					if( TECH[tech_options_position].down != null )
						tech_options_position = TECH[tech_options_position].down;
				}
				else
					if( cursor_position_y < 8 )
						cursor_position_y++;
				break;
			case KEY_IC:
			case KEY_DC:
			case KEY_HOME:
			case KEY_END:
			case KEY_PPAGE:
			case KEY_NPAGE:
				if( !puzzles.empty() )
				{
					p.init();
					switch(ch)
					{
						case KEY_IC:
							if( current_puzzle > 0 )
								current_puzzle--;
							break;
						case KEY_DC:
							if( current_puzzle < puzzles.size()-1 )
								current_puzzle++;
							break;
						case KEY_HOME:
							if( current_puzzle > 10 )
								current_puzzle-=10;
							else
								current_puzzle = 0;
							break;
						case KEY_END:
							if( current_puzzle < puzzles.size()-10-1 )
								current_puzzle+=10;
							else
								current_puzzle = puzzles.size()-1;
							break;
						case KEY_PPAGE:
							if( current_puzzle > 100 )
								current_puzzle-=100;
							else
								current_puzzle = 0;
							break;
						case KEY_NPAGE:
							if( current_puzzle < puzzles.size()-100-1 )
								current_puzzle+=100;
							else
								current_puzzle=puzzles.size()-1;
							break;
					}
					for( int i = 0; i < 81; i++ )
						if( puzzles[current_puzzle][i] > 48 && puzzles[current_puzzle][i] < 48+10 )
							p.original_map[i] = p.solution_map[i] = puzzles[current_puzzle][i]-48;
					mvprintw(37,0,"Puzzle #%i          ",current_puzzle+1);
debug << "Changed to Puzzle #" << current_puzzle+1 << "\n";
				}
				break;
			case 'b':
				if( user_actions & SINGLE_PASS_MODE )
					user_actions &= ~SINGLE_PASS_MODE;
				else
					user_actions |= SINGLE_PASS_MODE;
				break;
			case 'c':
				//in extreme debug mode, the highlight function will be hijacked. (or not)
				if( user_actions & COLOR_MODE )
					user_actions &= ~COLOR_MODE;
				else
				{
					user_actions |= COLOR_MODE;
					user_actions &= ~(USER_MAKE_MAP | TECH_MODE | DISQUALIFICATION_MODE | HIGHLIGHT_MODE);
				}
				break;
				//p.highlight_map[cursor_position_y*9+cursor_position_x] ^= true;
				break;
			case 'd':
				if( user_actions & DEBUG_MODE )
					user_actions &= ~DEBUG_MODE;
				else
				{
					user_actions |= DEBUG_MODE;
					user_actions &= ~EXTREME_DEBUG_MODE;
				}
				break;
			case 'D':
				if( user_actions & EXTREME_DEBUG_MODE )
					user_actions &= ~EXTREME_DEBUG_MODE;
				else
				{
					user_actions |= EXTREME_DEBUG_MODE;
					user_actions &= ~DEBUG_MODE;//more mutual exclusion
				}
				break;
			case 'e':
				if( user_actions & USER_MAKE_MAP )
					user_actions &= ~USER_MAKE_MAP;
				else
				{
					user_actions |= USER_MAKE_MAP;
					user_actions &= ~(DISQUALIFICATION_MODE | TECH_MODE | HIGHLIGHT_MODE | COLOR_MODE);//mutual exclusion
				}
				break;
			case 'f':
				{
					mvprintw(37,0,"<put> or <get>: ");
					char choice[10];
					getstr(choice);
					if( strcmp(choice,"get") == 0 )
					{
						char puzzleloc[100];
						mvprintw(38,0,"get puzzle from: ");
						getstr(puzzleloc);
						p.get_puzzle_from(puzzleloc);
						for( int i = 0; i < 17+strlen(puzzleloc); i++ )
							mvprintw(38,i," ");
					}
					else if( strcmp(choice,"put") == 0 )
					{
						char puzzleloc[100];
						mvprintw(38,0,"put puzzle in: ");
						getstr(puzzleloc);
						p.put_puzzle_in(puzzleloc);
						for( int i = 0; i < 17+strlen(puzzleloc); i++ )
							mvprintw(38,i," ");
					}
					for( int i = 0; i < 16+strlen(choice); i++ )
						mvprintw(37,i," ");
				}
				break;
			case 'G':
				/*srand(clock());
				do{
					p.init();///initialize the puzzle

					for( int i = 0; i < 60; i++ )///generate and place random numbers (make sure they're valid)
					{
						int position = rand() % 81;//pull a position out of the air
						int candidate = 0;//rand() % BIT_COUNTER[p.candidate_map[position]];//pick one of the available candidates
						for( int c = 0; c < 9; c++ )//find and set the candidate
							if( p.candidate_map[position] & (1<<c) )
							{
								candidate--;
								if( candidate == 0 )
									p.original_map[position] = p.solution_map[position] = (1<<c);
							}
						p.prep_candidate_map();
					}

					int map[81];
					while( brute_force(map,&p) > 1 )///add more random numbers until there is only one solution
					{
						int position = rand() % 81;//pull a position out of the air
						int candidate = 0;//rand() % BIT_COUNTER[p.candidate_map[position]];//pick one of the available candidates
						for( int c = 0; c < 9; c++ )//find and set the candidate
							if( p.candidate_map[position] & (1<<c) )
							{
								candidate--;
								if( candidate == 0 )
									p.original_map[position] = p.solution_map[position] = (1<<c);
							}
						p.prep_candidate_map();
					}

					///if there's only one solution, save the puzzle (by outputting it in 81 digit output to debugging file)
					if( brute_force(map,&p) == 1 )
					{
						for( int i = 0; i < 81; i++ )
							if( p.original_map[i] )
								debug << p.original_map[i] + 49;
							else
								debug << ".";
						debug << "\n";
					}
					debug.flush();
				}while(true);*/
				break;
			case 'h':
				if( user_actions & HIGHLIGHT_MODE )
					user_actions &= ~HIGHLIGHT_MODE;
				else
				{
					user_actions |= HIGHLIGHT_MODE;
					user_actions &= ~(USER_MAKE_MAP | DISQUALIFICATION_MODE | COLOR_MODE);
				}
				break;
			case 'H':
				if( user_actions & HINT_MODE )
					user_actions &= ~HINT_MODE;
				else
					user_actions |= HINT_MODE;
				break;
			case 'l':
				{
					char puzzleloc[100];
					mvprintw(37,0,"get collection from: ");
					getstr(puzzleloc);
					for( int i = 0; i < 21+strlen(puzzleloc); i++ )
					mvprintw(37,i," ");
					load(puzzleloc);
					p.init();
					for( int i = 0; i < 81; i++ )
						if( puzzles[current_puzzle][i] > 48 && puzzles[current_puzzle][i] < 48+10 )
							p.original_map[i] = p.solution_map[i] = puzzles[current_puzzle][i]-48;
					mvprintw(37,0,"Puzzle #%i          ",current_puzzle+1);
				}
				break;
			case 'm':
				if( user_actions & MEDUSA_MODE )
					user_actions &= ~MEDUSA_MODE;
				else
					user_actions |= MEDUSA_MODE;
				break;
			case 'o':
				if( user_actions & SHOW_HINT_MODE )
					user_actions &= ~SHOW_HINT_MODE;
				else
					user_actions |= SHOW_HINT_MODE;
				break;
			case 'p':
				//'paste' a puzzle.  only supported format is a string of 81 digits.
				mvprintw(37,0,"Puzzle: ");
				char puzzle[85];
				getstr(puzzle);
				p.init();
				if( strlen(puzzle) < 81 )
				{
					mvprintw(38,0,"puzzle does not fit format.  read documentation for format.");
					getch();
					for( int i = 0; i < 8+strlen(puzzle); i++ )
						mvprintw(37,i," ");
					mvprintw(38,0,"                                                           ");
				}
				else
				{
					for( int i = 0; i < 81; i++ )
						if( puzzle[i] > 48 && puzzle[i] < 48+10 )
							p.original_map[i] = p.solution_map[i] = puzzle[i]-48;
					for( int i = 0; i < 8+strlen(puzzle); i++ )
						mvprintw(37,i," ");
				}
				break;
			case 'r':
				if( user_actions & DISQUALIFICATION_MODE )
					p.dq_reset();
				else
					p.reset();
				break;
			case 's':
				user_actions |= AUTOSOLVE;
				break;
			case 't':
				if( user_actions & TECH_MODE )
					user_actions &= ~TECH_MODE;
				else
				{
					user_actions |= TECH_MODE;
					user_actions &= ~(DISQUALIFICATION_MODE | USER_MAKE_MAP | COLOR_MODE);//mutual exclusion
				}
				break;
			case 'u':
				user_actions |= UPDATE_MODE;
				break;
			case 'x':
				//allow the user to manually remove candidates from consideration
				if( user_actions & DISQUALIFICATION_MODE )
					user_actions &= ~DISQUALIFICATION_MODE;
				else
				{
					user_actions |= DISQUALIFICATION_MODE;
					user_actions &= ~(USER_MAKE_MAP | TECH_MODE | HIGHLIGHT_MODE | COLOR_MODE);
				}
				break;
			case 'Y':
				if( !puzzles.empty() )
					user_actions |= TRY_TO_SOLVE_MODE;
				break;
			case ' ':
				if( user_actions & TECH_MODE )
				{
					if( techniques[TECH[tech_options_position].responsibility_class] & TECH[tech_options_position].responsibilities )
						techniques[TECH[tech_options_position].responsibility_class] &= ~TECH[tech_options_position].responsibilities;
					else
						techniques[TECH[tech_options_position].responsibility_class] |= TECH[tech_options_position].responsibilities;
				}
				else if( user_actions & COLOR_MODE )
				{
					if( p.highlight_map[cursor_position_y*9+cursor_position_x] )
						p.highlight_map[cursor_position_y*9+cursor_position_x] = 0;
					else
						p.highlight_map[cursor_position_y*9+cursor_position_x] = ALL_CANDIDATES;
				}
				else if( !(user_actions & USER_MAKE_MAP) && p.suggestion_map[cursor_position_y*9+cursor_position_x] != 0 )
					p.solution_map[cursor_position_y*9+cursor_position_x] = p.suggestion_map[cursor_position_y*9+cursor_position_x];
				break;
			case '0':
				if( user_actions & USER_MAKE_MAP )
					p.original_map[cursor_position_y*9+cursor_position_x] = 
					p.solution_map[cursor_position_y*9+cursor_position_x] = 0;
				else if( user_actions & DISQUALIFICATION_MODE )
					p.disqualification_map[cursor_position_y*9+cursor_position_x] = 0;
				else if( user_actions & HIGHLIGHT_MODE )
				{
					if( highlight_these != ALL_CANDIDATES )
						highlight_these = ALL_CANDIDATES;
					else
						highlight_these = 0;
				}
				else if( user_actions & COLOR_MODE )
				{
					p.highlight_map[cursor_position_y*9+cursor_position_x] = 0;
				}
				else
					if( p.original_map[cursor_position_y*9+cursor_position_x] == 0 )
						p.solution_map[cursor_position_y*9+cursor_position_x] = 0;
				break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if( user_actions & USER_MAKE_MAP )
					p.original_map[cursor_position_y*9+cursor_position_x] = 
					p.solution_map[cursor_position_y*9+cursor_position_x] = ch-48;
				else if( user_actions & DISQUALIFICATION_MODE )
				{
					if( p.disqualification_map[cursor_position_y*9+cursor_position_x] & (1<<(ch-49)) )
						p.disqualification_map[cursor_position_y*9+cursor_position_x] &= ~(1<<(ch-49));
					else
						p.disqualification_map[cursor_position_y*9+cursor_position_x] |= (1<<(ch-49));
				}
				else if( user_actions & HIGHLIGHT_MODE )
				{
					if( highlight_these & (1<<(ch-49)) )
						highlight_these &= ~(1<<(ch-49));
					else
						highlight_these |= (1<<(ch-49));
				}
				else if( user_actions & COLOR_MODE )
				{
					if( p.highlight_map[cursor_position_y*9+cursor_position_x] & (1<<(ch-49)) )
						p.highlight_map[cursor_position_y*9+cursor_position_x] &= ~(1<<(ch-49));
					else
						p.highlight_map[cursor_position_y*9+cursor_position_x] |= (1<<(ch-49));
				}
				else
					p.solution_map[cursor_position_y*9+cursor_position_x] = ch-48;
				break;
		}

		if( user_actions & TRY_TO_SOLVE_MODE )
		{
			user_actions &= ~TRY_TO_SOLVE_MODE;

			trio[0] = 0;
			trio[1] = 0;
			trio[2] = 0;

			for( int n = 0; n < puzzles.size(); n++ )
			{
				p.init();
				for( int i = 0; i < 81; i++ )
					if( puzzles[n][i] > 48 && puzzles[n][i] < 48+10 )
						p.original_map[i] = p.solution_map[i] = puzzles[n][i]-48;
				mvprintw(37,0,"Puzzle #%i          ",n+1);
debug << "Changed to Puzzle #" << n+1 << "\n";

				process_and_display(&p,cursor_position_x,cursor_position_y,techniques,user_actions,tech_options_position,highlight_these,trio);
				user_actions &= ~AUTOSOLVE;//ugh...
				user_actions &= ~UPDATE_MODE;
				mvprintw(27,100,"Solved  : %i",trio[0]);
				mvprintw(28,100,"Unsolved: %i",trio[1]);
				mvprintw(29,100,"Invalid : %i",trio[2]);
			}
		}
		else
		{
			process_and_display(&p,cursor_position_x,cursor_position_y,techniques,user_actions,tech_options_position,highlight_these,trio);
			user_actions &= ~AUTOSOLVE;
			user_actions &= ~UPDATE_MODE;
			mvprintw(27,100,"                ",trio[0]);
			mvprintw(28,100,"                ",trio[1]);
			mvprintw(29,100,"                ",trio[2]);
		}
	}

	endwin();
	return 0;
}

//hidden singles appears to work properly
void hidden_singles(int *mymap, int *candidate_map, int flags)
{
	for( int i = 0; i < 81; i++ )
		mymap[i] = candidate_map[i];

	//check each row/column and block
	for( int i = 0; i < 9; i++ )
	{
		for( int value = 0; value < 9; value++ )
		{
			//if there's only one in this row, then find it and remove the other candidates
			if( row_contains(candidate_map,i,(1<<value)) == 1 )
			{
				for( int find = 0; find < 9; find++ )
					if( candidate_map[ROWS[i][find]] & (1<<value) )
						mymap[ROWS[i][find]] = (1<<value);
			}
			//same for the columns
			if( column_contains(candidate_map,i,(1<<value)) == 1 )
			{
				for( int find = 0; find < 9; find++ )
					if( candidate_map[COLUMNS[i][find]] & (1<<value) )
						mymap[COLUMNS[i][find]] = (1<<value);
			}
			//same for the blocks
			if( block_contains(candidate_map,i,(1<<value)) == 1 )
			{
				for( int find = 0; find < 9; find++ )
					if( candidate_map[BLOCKS[i][find]] & (1<<value) )
						mymap[BLOCKS[i][find]] = (1<<value);
			}
		}
	}
}

void block_rc_interactions(int *mymap, int *candidate_map, int flags)
{
	for( int i = 0; i < 81; i++ )
		mymap[i] = candidate_map[i];

	for( int block_number = 0; block_number < 9; block_number++ )
	{
		for( int value = 0; value < 9; value++ )
		{
			//if there are two or more of a candidate in a block, then proceed to check rows and columns
			if( block_contains(candidate_map, block_number,(1<<value)) > 1 )
			{
				for( int row = 0; row < 3; row++ )
					if( block_subrow_contains(candidate_map, block_number, row, (1<<value)) == block_contains(candidate_map, block_number,(1<<value)) )//and if this row contains all instances in this block
					{
						for( int friends = 0; friends < 2; friends++ )
							for( int position = 0; position < 3; position++ )
								mymap[BLOCK_ROWS[BLOCK_ROW_FRIENDS[block_number][friends]][row][position]] &= ~(1<<value);
					}

				for( int column = 0; column < 3; column++ )
					if( block_subcolumn_contains(candidate_map, block_number, column, (1<<value)) == block_contains(candidate_map, block_number,(1<<value)) )
					{
						for( int friends = 0; friends < 2; friends++ )
							for( int position = 0; position < 3; position++ )
								mymap[BLOCK_COLUMNS[BLOCK_COLUMN_FRIENDS[block_number][friends]][column][position]] &= ~(1<<value);
					}
			}
		}
	}
}

void block_block_interactions(int *mymap, int *candidate_map, int flags)
{
	for( int i = 0; i < 81; i++ )
		mymap[i] = candidate_map[i];

	short order_table[][2] = {	{0,1},	{0,2},	{1,2}	};

	//check from each block.  sure, this is inefficient, but we can spare the overhead.
	for( int block_number = 0; block_number < 9; block_number++ )
	{
		//check for every candidate
		for( int value = 0; value < 9; value++ )
		{
			//consider the originating block as it relates to each of the other blocks in its row
			for( int friend_number = 0; friend_number < 2; friend_number++ )
			{
				//ROW HANDLER
				//this is just an exhaustive look through each possible combination of rows
				for( int order_number = 0; order_number < 3; order_number++ )
				{
					int blockA_total = block_contains(candidate_map,block_number,(1<<value));
					int blockA_row1_total = block_subrow_contains(candidate_map,block_number,order_table[order_number][0],(1<<value));
					int blockA_row2_total = block_subrow_contains(candidate_map,block_number,order_table[order_number][1],(1<<value));
					int blockB_total = block_contains(candidate_map,BLOCK_ROW_FRIENDS[block_number][friend_number],(1<<value));
					int blockB_row1_total = block_subrow_contains(candidate_map,BLOCK_ROW_FRIENDS[block_number][friend_number],order_table[order_number][0],(1<<value));
					int blockB_row2_total = block_subrow_contains(candidate_map,BLOCK_ROW_FRIENDS[block_number][friend_number],order_table[order_number][1],(1<<value));

					//if these are all non-zero values
					if( blockA_row1_total && blockA_row2_total && blockB_row1_total && blockB_row2_total )
					{
						//and they happen to be all the instances of the candidate in both blocks...
						if( blockA_row1_total + blockA_row2_total == blockA_total && 
							blockB_row1_total + blockB_row2_total == blockB_total )
						{
							for( int clobber_value = 0; clobber_value < 3; clobber_value++ )
							{
								mymap[BLOCK_ROWS[BLOCK_ROW_FRIENDS[block_number][(friend_number+1)%2]][order_table[order_number][0]][clobber_value]] &= ~(1<<value);
								mymap[BLOCK_ROWS[BLOCK_ROW_FRIENDS[block_number][(friend_number+1)%2]][order_table[order_number][1]][clobber_value]] &= ~(1<<value);
							}
						}
					}
				}

				//COLUMN HANDLER
				//this is just an exhaustive look through each possible combination of columns
				for( int order_number = 0; order_number < 3; order_number++ )
				{
					int blockA_total = block_contains(candidate_map,block_number,(1<<value));
					int blockA_column1_total = block_subcolumn_contains(candidate_map,block_number,order_table[order_number][0],(1<<value));
					int blockA_column2_total = block_subcolumn_contains(candidate_map,block_number,order_table[order_number][1],(1<<value));
					int blockB_total = block_contains(candidate_map,BLOCK_COLUMN_FRIENDS[block_number][friend_number],(1<<value));
					int blockB_column1_total = block_subcolumn_contains(candidate_map,BLOCK_COLUMN_FRIENDS[block_number][friend_number],order_table[order_number][0],(1<<value));
					int blockB_column2_total = block_subcolumn_contains(candidate_map,BLOCK_COLUMN_FRIENDS[block_number][friend_number],order_table[order_number][1],(1<<value));

					//if these are all non-zero values
					if( blockA_column1_total && blockA_column2_total && blockB_column1_total && blockB_column2_total )
					{
						//and they happen to be all the instances of the candidate in both blocks...
						if( blockA_column1_total + blockA_column2_total == blockA_total && 
							blockB_column1_total + blockB_column2_total == blockB_total )
						{
							for( int clobber_value = 0; clobber_value < 3; clobber_value++ )
							{
								mymap[BLOCK_COLUMNS[BLOCK_COLUMN_FRIENDS[block_number][(friend_number+1)%2]][order_table[order_number][0]][clobber_value]] &= ~(1<<value);
								mymap[BLOCK_COLUMNS[BLOCK_COLUMN_FRIENDS[block_number][(friend_number+1)%2]][order_table[order_number][1]][clobber_value]] &= ~(1<<value);
							}
						}
					}
				}
			}
		}
	}
}

//this function seems to work.  i'll have to give it time to prove itself, but it's looking good.
void naked_subsets(int *mymap, Puzzle *p, int flags, int techniques)
{
	for( int i = 0; i < 81; i++ )
		mymap[i] = p->candidate_map[i];

	for( int set = 0; set < 512; set++ )//this will cycle through all possible subsets of our 9-bit space
	{
		bool proceed = false;
		int cardinality = BIT_COUNTER[set];

		//this is so i can finely control which techniques are being used.
		if( cardinality == 2 && (techniques & TECH_SUBSETS_NAKED_DOUBLES) )		proceed = true;
		if( cardinality == 3 && (techniques & TECH_SUBSETS_NAKED_TRIPLES) )		proceed = true;
		if( cardinality == 4 && (techniques & TECH_SUBSETS_NAKED_QUADS) )		proceed = true;
		if( cardinality > 4 && (techniques & TECH_SUBSETS_NAKED_OTHER) )		proceed = true;

		if( proceed )//but we only care about subsets with at least 2 members.  naked singles are already addressed.
		{
			for( int group = 0; group < 9; group++ )//we go through groups and positions as normal now
			{
				int row_matches = 0;//how many cells contain subsets of our set
				int column_matches = 0;
				int block_matches = 0;

				bool row_position[9] = {false, false, false, false, false, false, false, false, false};
				bool column_position[9] = {false, false, false, false, false, false, false, false, false};
				bool block_position[9] = {false, false, false, false, false, false, false, false, false};

				for( int position = 0; position < 9; position++ )
				{
					if( (p->candidate_map[ROWS[group][position]] | set) == set )//then this cell must contain a subset
						if( BIT_COUNTER[p->candidate_map[ROWS[group][position]]] >= 2 )//then we're in business!
						{
							row_matches++;
							row_position[position] = true;
						}

					if( (p->candidate_map[COLUMNS[group][position]] | set) == set )
						if( BIT_COUNTER[p->candidate_map[COLUMNS[group][position]]] >= 2 )
						{
							column_matches++;
							column_position[position] = true;
						}

					if( (p->candidate_map[BLOCKS[group][position]] | set) == set )
						if( BIT_COUNTER[p->candidate_map[BLOCKS[group][position]]] >= 2 )
						{
							block_matches++;
							block_position[position] = true;
						}
				}

				if( row_matches == cardinality )
					for( int position = 0; position < 9; position++ )
						if( row_position[position] == false )
							mymap[ROWS[group][position]] &= ~set;

				if( column_matches == cardinality )
					for( int position = 0; position < 9; position++ )
						if( column_position[position] == false )
							mymap[COLUMNS[group][position]] &= ~set;

				if( block_matches == cardinality )
					for( int position = 0; position < 9; position++ )
						if( block_position[position] == false )
							mymap[BLOCKS[group][position]] &= ~set;
			}
		}
	}
//debug.flush();
}

//i put in some debug code and found that, while my idea was correct, it wasn't complete.  so i added completeness tests and now this seems to work.
void hidden_subsets(int *mymap, Puzzle *p, int flags, int techniques)
{
	for( int i = 0; i < 81; i++ )
		mymap[i] = p->candidate_map[i];

	for( int set = 0; set < 512; set++ )//this will cycle through all possible subsets of our 9-bit space
	{
		bool proceed = false;
		int cardinality = BIT_COUNTER[set];

		//this is so i can finely control which techniques are being used.
		if( cardinality == 2 && (techniques & TECH_SUBSETS_HIDDEN_DOUBLES) )		proceed = true;
		if( cardinality == 3 && (techniques & TECH_SUBSETS_HIDDEN_TRIPLES) )		proceed = true;
		if( cardinality == 4 && (techniques & TECH_SUBSETS_HIDDEN_QUADS) )		proceed = true;
		if( cardinality > 4 && (techniques & TECH_SUBSETS_HIDDEN_OTHER) )		proceed = true;

		if( proceed )//but we only care about subsets with at least 2 members.  hidden singles are already addressed.
		{
			for( int group = 0; group < 9; group++ )//we go through groups and positions as normal now
			{
				int row_matches = 0;//how many cells contain subsets of our set
				int column_matches = 0;
				int block_matches = 0;

				bool row_position[9] = {false, false, false, false, false, false, false, false, false};
				bool column_position[9] = {false, false, false, false, false, false, false, false, false};
				bool block_position[9] = {false, false, false, false, false, false, false, false, false};

				for( int position = 0; position < 9; position++ )
				{
					if( ((p->candidate_map[ROWS[group][position]] & set) | set) == set )
						if( BIT_COUNTER[p->candidate_map[ROWS[group][position]] & set] )//just as long as it's not empty
						{
							row_matches++;
							row_position[position] = true;
						}

					if( ((p->candidate_map[COLUMNS[group][position]] & set) | set) == set )
						if( BIT_COUNTER[p->candidate_map[COLUMNS[group][position]] & set] )//we'll get to this part later.
						{
							column_matches++;
							column_position[position] = true;
						}

					if( ((p->candidate_map[BLOCKS[group][position]] & set) | set) == set )
						if( BIT_COUNTER[p->candidate_map[BLOCKS[group][position]] & set] )
						{
							block_matches++;
							block_position[position] = true;
						}
				}

				if( row_matches == cardinality )
				{
					int set_test = 0;
					for( int position = 0; position < 9; position++ )
						if( row_position[position] == true )
						{
							set_test |= (p->candidate_map[ROWS[group][position]] & set);
							if( BIT_COUNTER[p->candidate_map[ROWS[group][position]] & set] < 2 )
								set_test = ALL_CANDIDATES;//disqualify this guy because every cell involved should have at least two candidates from this set
						}
						
					if( set_test == set )
						for( int position = 0; position < 9; position++ )
							if( row_position[position] == true )
								mymap[ROWS[group][position]] &= set;
				}

				if( column_matches == cardinality )
				{
					int set_test = 0;
					for( int position = 0; position < 9; position++ )
						if( column_position[position] == true )
						{
							set_test |= (p->candidate_map[COLUMNS[group][position]] & set);
							if( BIT_COUNTER[p->candidate_map[COLUMNS[group][position]] & set] < 2 )
								set_test = ALL_CANDIDATES;
						}

					if( set_test == set )
						for( int position = 0; position < 9; position++ )
							if( column_position[position] == true )
								mymap[COLUMNS[group][position]] &= set;
				}

				if( block_matches == cardinality )
				{
					int set_test = 0;
					for( int position = 0; position < 9; position++ )
						if( block_position[position] == true )
						{
							set_test |= (p->candidate_map[BLOCKS[group][position]] & set);
							if( BIT_COUNTER[p->candidate_map[BLOCKS[group][position]] & set] < 2 )
								set_test = ALL_CANDIDATES;
						}

					if( set_test == set )
						for( int position = 0; position < 9; position++ )
							if( block_position[position] == true )
								mymap[BLOCKS[group][position]] &= set;
				}
			}
		}
	}
}

void recursively_color(Puzzle *p, int dimension, int position, int *visited, int *truth_table, bool truth, bool main_chain, int techniques)
{
	if( main_chain )
		p->book.tried[dimension][position] = true;

	visited[position] |= (1<<dimension);

	if( truth )
		truth_table[position] |= (1<<dimension);
	else
		truth_table[position] &= ~(1<<dimension);

	//you got here because you had hard links.  so i don't even have to check.
	if( p->book.hard_links[dimension][position].row != null )
		if( !(visited[p->book.hard_links[dimension][position].row] & (1<<dimension)) )
			recursively_color(p,dimension,p->book.hard_links[dimension][position].row,visited,truth_table,!truth,main_chain,techniques);
	if( p->book.hard_links[dimension][position].column != null )
		if( !(visited[p->book.hard_links[dimension][position].column] & (1<<dimension)) )
			recursively_color(p,dimension,p->book.hard_links[dimension][position].column,visited,truth_table,!truth,main_chain,techniques);
	if( p->book.hard_links[dimension][position].block != null )
		if( !(visited[p->book.hard_links[dimension][position].block] & (1<<dimension)) )
			recursively_color(p,dimension,p->book.hard_links[dimension][position].block,visited,truth_table,!truth,main_chain,techniques);
	if( p->book.hard_links[dimension][position].dimension != null && (techniques & TECH_COLORING_MEDUSA))
		if( !(visited[position] & (1<<(p->book.hard_links[dimension][position].dimension))) )
			recursively_color(p,p->book.hard_links[dimension][position].dimension,position,visited,truth_table,!truth,main_chain,techniques);

	main_chain = false;//we're leaving the main chain now, so let's make sure to communicate that.

///cell friends would come in handy here.
	if( truth )//if i'm true, i'll go ahead and set everything else in my block, column and row to be false.
	{
		//this handles blocks, rows and columns
		for( int friends = 0; friends < 20; friends++ )
		{
			if( p->book.hard_linked[dimension][CELL_FRIENDS[position][friends]] && (techniques & TECH_COLORING_JUMPER))
			{
				if( !(visited[CELL_FRIENDS[position][friends]] & (1<<dimension)) )
					recursively_color(p,dimension,CELL_FRIENDS[position][friends],visited,truth_table,false,main_chain,techniques);
			}
			else
				truth_table[CELL_FRIENDS[position][friends]] &= ~(1<<dimension);
		}

		//this handles the dimensions
		if( techniques & TECH_COLORING_MEDUSA )
			for( int friends = 0; friends < 9; friends++ )
			{
				if( friends != dimension )
				{
					if( p->book.hard_linked[friends][position] && (techniques & TECH_COLORING_JUMPER))
					{
						if( !(visited[CELL_FRIENDS[position][dimension]] & (1<<friends)) )
							recursively_color(p,friends,position,visited,truth_table,false,main_chain,techniques);
					}
					else
						truth_table[position] &= ~(1<<friends);
				}
			}
	}
}

void coloring_with_consequences(Puzzle *p, int candidate, int position, bool truth, int techniques, int *history, int flag)
{
	int truth_table[81];
	for( int i = 0; i < 81; i++ )
		truth_table[i] = p->candidate_map[i];

	int visited[81];

	bool updating;
	do{
		p->build_coloringbook(history);

		for( int c = 0; c < 81; c++ )
			visited[c] = 0;

		recursively_color(p,candidate,position,visited,truth_table,truth,true,techniques);

		int branch_map[81];
		for( int i = 0; i < 81; i++ )
			branch_map[i] = history[i] & truth_table[i];

		block_block_interactions(branch_map,branch_map,flag);
		block_rc_interactions(branch_map,branch_map,flag);
		hidden_singles(branch_map,branch_map,flag);
		///now to simulate naked singles
		for( int i = 0; i < 81; i++ )
			if( BIT_COUNTER[branch_map[i]] == 1 )
				for( int j = 0; j < 20; j++ )
					branch_map[CELL_FRIENDS[i][j]] &= ~branch_map[i];

		//contradiction checking.  if it's broken, tell me for debugging purposes and mute it.
		if( !everything_seems_ok(p->solution_map,branch_map) )
		{
			//debug << "everything is not ok\n";
			for( int i = 0; i < 81; i++ )
				branch_map[i] = 0;
		}

		updating = false;
		for( int i = 0; i < 81; i++ )
			if( branch_map[i] != history[i] )
			{
				history[i] = branch_map[i];
				updating = true;
			}
	}while( updating );
}
/**
I find it curious that looking at singles should be more effective than messing with block interactions.  i mean, i thought that getting rid of things
would always be good... but using the block techniques really hurt the performance of my implications functions... use of hidden singles didn't have
a noticeable effect, so i turned them off.  this solver solves 100%, as in 242/242, of the puzzles in Mensa's Absolutely Nasty Level 4 book.

it really does seem possible to eliminate too many candidates.
*/
void coloring(int *mymap, Puzzle *p, int flag, int techniques)
{
	for( int set = 0; set < 81; set++ )//we've been over this... equal opportunity axe.  remember?
		mymap[set] = ALL_CANDIDATES;

	for( int c = 0; c < 9; c++ )//clear the tried guy so i can keep up with where hard chains are
		for( int d = 0; d < 81; d++ )
			p->book.tried[c][d] = false;

	int t[81], f[81], tv[81], fv[81];
	for( int candidate = 0; candidate < 9; candidate++ )
	{
		for( int position = 0; position < 81; position++ )
		{
			if( p->book.hard_linked[candidate][position] && !p->book.tried[candidate][position] )
			{
				if( techniques & TECH_COLORING_UPDATE )
				{
					int th[81], fh[81];//true history and false history
					for( int i = 0; i < 81; i++ )
						th[i] = fh[i] = p->candidate_map[i];//copy the candidate map into the history maps

					coloring_with_consequences(p,candidate,position,true,techniques,th,flag);
					coloring_with_consequences(p,candidate,position,false,techniques,fh,flag);

					for( int c = 0; c < 81; c++ )
						mymap[c] &= (th[c] | fh[c]);//then by golly, it must be false.
				}
				else
				{
					for( int c = 0; c < 81; c++ )
					{
						t[c] = f[c] = ALL_CANDIDATES;
						tv[c] = fv[c] = 0;
					}

					recursively_color(p,candidate,position,tv,t,true,true,techniques);
					recursively_color(p,candidate,position,fv,f,false,true,techniques);

					for( int c = 0; c < 81; c++ )
						mymap[c] &= (t[c] | f[c]);
				}
			}
		}
	}
//debug.flush();
}

void recursively_force_chains(Puzzle *p, int *map, int position, int value, bool *visited)
{
	map[position] = value;
	visited[position] = true;
	int row = ROW_LOOKUP[position];
	int column = COLUMN_LOOKUP[position];
	int block = BLOCK_LOOKUP[position];

	for( int i = 0; i < 9; i++ )
	{
		if(!visited[ROWS[row][i]] && BIT_COUNTER[p->candidate_map[ROWS[row][i]]] == 2 && p->candidate_map[ROWS[row][i]] & value )
			recursively_force_chains(p,map,ROWS[row][i],p->candidate_map[ROWS[row][i]] & ~value,visited);
		if(!visited[COLUMNS[column][i]] && BIT_COUNTER[p->candidate_map[COLUMNS[column][i]]] == 2 && p->candidate_map[COLUMNS[column][i]] & value )
			recursively_force_chains(p,map,COLUMNS[column][i],p->candidate_map[COLUMNS[column][i]] & ~value,visited);
		if(!visited[BLOCKS[block][i]] && BIT_COUNTER[p->candidate_map[BLOCKS[block][i]]] == 2 && p->candidate_map[BLOCKS[block][i]] & value )
			recursively_force_chains(p,map,BLOCKS[block][i],p->candidate_map[BLOCKS[block][i]] & ~value,visited);
	}
}

void forcing_chains(int *mymap, Puzzle *p, int flag)
{
	for( int i = 0; i < 81; i++ )
		mymap[i] = p->candidate_map[i];

	for( int i = 0; i < 81; i++ )
	{
		if( BIT_COUNTER[p->candidate_map[i]] == 2 )
		{
			bool found_first = false;
			int first, second;
			for( int j = 0; j < 9; j++ )
			{
				if( p->candidate_map[i] & (1<<j) )
				{
					if( found_first )
						second = (1<<j);
					else
					{
						first =  (1<<j);
						found_first = true;
					}
				}
			}//now we've found the values of these two
			bool visited[81];
			int first_map[81], second_map[81];
			for( int j = 0; j < 81; j++ )
			{
				first_map[j] = second_map[j] = ALL_CANDIDATES;
				visited[j] = false;
			}
			recursively_force_chains(p,first_map,i,first,visited);
			for( int j = 0; j < 81; j++ )
				visited[j] = false;
			recursively_force_chains(p,second_map,i,second,visited);

			bool first_ok = everything_seems_ok(p->solution_map,first_map);
			bool second_ok = everything_seems_ok(p->solution_map,second_map);
			if( !first_ok && !second_ok )
				debug << "forcing chains valid branch thing is broken\n";
			else if( !second_ok )
				for( int j = 0; j < 81; j++ )
					mymap[j] = first_map[j];
			else if( !first_ok )
				for( int j = 0; j < 81; j++ )
					mymap[j] = second_map[j];
			else
				for( int j = 0; j < 81; j++ )
					if( first_map[j] == second_map[j] && BIT_COUNTER[first_map[j]] == 1 )
						mymap[j] = first_map[j];
		}
	}
//debug.flush();
}

void recursive_xy_chain(Puzzle *p, int *map, int position, int value, bool *visited)
{
	map[position] = value;
	visited[position] = true;
	int row = ROW_LOOKUP[position];
	int column = COLUMN_LOOKUP[position];
	int block = BLOCK_LOOKUP[position];

	//take me out of other cells in my group
	for( int i = 0; i < 9; i++ )
	{
		if( ROWS[row][i] != position )
			map[ROWS[row][i]] &= ~value;
		if( COLUMNS[column][i] != position )
			map[COLUMNS[column][i]] &= ~value;
		if( BLOCKS[block][i] != position )
			map[BLOCKS[block][i]] &= ~value;
	}

	for( int i = 0; i < 9; i++ )
	{
		if(!visited[ROWS[row][i]] && BIT_COUNTER[p->candidate_map[ROWS[row][i]]] == 2 && p->candidate_map[ROWS[row][i]] & value )
			recursively_force_chains(p,map,ROWS[row][i],p->candidate_map[ROWS[row][i]] & ~value,visited);
		if(!visited[COLUMNS[column][i]] && BIT_COUNTER[p->candidate_map[COLUMNS[column][i]]] == 2 && p->candidate_map[COLUMNS[column][i]] & value )
			recursively_force_chains(p,map,COLUMNS[column][i],p->candidate_map[COLUMNS[column][i]] & ~value,visited);
		if(!visited[BLOCKS[block][i]] && BIT_COUNTER[p->candidate_map[BLOCKS[block][i]]] == 2 && p->candidate_map[BLOCKS[block][i]] & value )
			recursively_force_chains(p,map,BLOCKS[block][i],p->candidate_map[BLOCKS[block][i]] & ~value,visited);
	}
}

void xy_chain(int *mymap, Puzzle *p, int flag)
{
	///it might be ok to use a modification of the old forcing chain code here... That's not looking like such a good idea anymore...
	for( int copy = 0; copy < 81; copy++ )
		mymap[copy] = p->candidate_map[copy];//because we'll be ANDing later

	for( int position = 0; position < 81; position++ )
	{
		if( BIT_COUNTER[p->candidate_map[position]] == 2 )
		{
			int i_choose_the_first_guy[81];
			int i_choose_the_second_guy[81];
			bool first_visited[81];
			bool second_visited[81];

			for( int set = 0; set < 81; set++ )
			{
				i_choose_the_first_guy[set] = i_choose_the_second_guy[set] = p->candidate_map[set];
				first_visited[set] = second_visited[set] = false;
			}

			int first_guy;
			int second_guy;
			bool first_guy_found = false;

			for( int j = 0; j < 9; j++ )
			{
				if( p->candidate_map[position] & (1<<j) )
				{
					if( first_guy_found )
						second_guy = (1<<j);
					else
					{
						first_guy =  (1<<j);
						first_guy_found = true;
					}
				}
			}

			recursive_xy_chain(p,i_choose_the_first_guy,position,first_guy, first_visited);
			recursive_xy_chain(p,i_choose_the_second_guy,position,second_guy, second_visited);

			bool first_ok = everything_seems_ok(p->solution_map,i_choose_the_first_guy);
			bool second_ok = everything_seems_ok(p->solution_map,i_choose_the_second_guy);
			if( !first_ok && !second_ok )
				debug << "xy chains valid branch thing is broken\n";
			else if( !second_ok )
				for( int sync = 0; sync < 81; sync++ )
					mymap[sync] &= i_choose_the_first_guy[sync];
			else if( !first_ok )
				for( int sync = 0; sync < 81; sync++ )
					mymap[sync] &= i_choose_the_second_guy[sync];
			else
				for( int sync = 0; sync < 81; sync++ )
					mymap[sync] &= (i_choose_the_first_guy[sync] | i_choose_the_second_guy[sync]);
		}
	}
//debug.flush();
}

void simple_fish(int *mymap, Puzzle *p, int flag, int techniques)//aka X-Wing, Swordfish, Jellyfish and Squirmbag
{
	for( int copy = 0; copy < 81; copy++ )
		mymap[copy] = 0;//we'll OR together all our candidate guys at the end

	//set up a little map to keep track of candidates for a while
	bool candid_map[9][81];
	for( int candidate = 0; candidate < 9; candidate++ )
		for( int position = 0; position < 81; position++ )
			if( p->candidate_map[position] & (1<<candidate) )
				candid_map[candidate][position] = true;
			else
				candid_map[candidate][position] = false;

	//use these to count the instances of each candidate in each row/column for use in qualifying for further examination
	int rows[9][9];
	int columns[9][9];
	for( int candidate = 0; candidate < 9; candidate++ )
	{
		for( int group = 0; group < 9; group++ )
		{
			rows[candidate][group] = 0;
			for( int position = 0; position < 9; position++ )
				if( candid_map[candidate][ROWS[group][position]] )//if that's true, there's something there and we'll count it
					rows[candidate][group]++;

			columns[candidate][group] = 0;
			for( int position = 0; position < 9; position++ )
				if( candid_map[candidate][COLUMNS[group][position]] )
					columns[candidate][group]++;
		}
	}

	//we're walking through all the permutations of 2, 3, 4 and 5 bits over our 9-bit space
	for( int set = 0; set < 512; set++ )
	{
		bool proceed = false;
		int cardinality = BIT_COUNTER[set];

		if( cardinality == 2 && (techniques & TECH_FISH_NORMAL_XWING) )					proceed = true;
		if( cardinality == 3 && (techniques & TECH_FISH_NORMAL_SWORDFISH) )				proceed = true;
		if( cardinality == 4 && (techniques & TECH_FISH_NORMAL_JELLYFISH) )				proceed = true;
		if( cardinality == 5 && (techniques & TECH_FISH_NORMAL_SQUIRMBAG) )				proceed = true;
		if( cardinality >= 6 && cardinality <= 8 && (techniques & TECH_FISH_NORMAL_OTHER) )		proceed = true;

		if( proceed )//this covers X-Wing, Swordfish, Jellyfish and Squirmbag (i've seen them with 6 and 7)
		{
			for( int candidate = 0; candidate < 9; candidate++ )
			{
				int row_participants = 0;
				int column_participants = 0;
				//go group by group looking for things represented in the set
				for( int check_for_participants = 0; check_for_participants < 9; check_for_participants++ )
				{
					//if you find something that's in the set, see if it has the right number of guys
					if( set & (1<<check_for_participants) )
					{
						//if a group has one instance, it can't even be an x-wing and it's not covered by the fish
						//if a group has more than [cardinality] instances, it occupies too many columns and is DQ
						if( rows[candidate][check_for_participants] >= 2 && rows[candidate][check_for_participants] <= cardinality )
							row_participants++;

						//we do this by rows *and* by columns because either one can initiate a fish
						if( columns[candidate][check_for_participants] >= 2 && columns[candidate][check_for_participants] <= cardinality )
							column_participants++;
					}
				}

				//if each of the rows designated by the [set] qualifies, we can mark them and check for a fish
				if( row_participants == cardinality )
				{
					bool mark_map[81];
					for( int i = 0; i < 81; i++ )
						mark_map[i] = false;

					//we mark the candidates in question on this and then check to see if they occupy the same number of columns
					for( int row = 0; row < 9; row++ )
						if( set & (1<<row) )
							for( int position = 0; position < 9; position++ )
								if( candid_map[candidate][ROWS[row][position]] )
									mark_map[ROWS[row][position]] = true;

					//check to see how many columns are occupied by these 'marked' cells
					int column_occupancy = 0;
					for( int group = 0; group < 9; group++ )
						for( int position = 0; position < 9; position++ )
							if( mark_map[COLUMNS[group][position]] )
								column_occupancy |= (1<<group);

					//if this is true, then column_occupancy == row_participants and we have our fish!
					if( BIT_COUNTER[column_occupancy] == cardinality )
					{
						if( flag & SHOW_HINT_MODE )
						{
							for( int i = 0; i < 81; i++ )
								if( mark_map[i] )
									p->hint[i] |= (1<<candidate);//so it can be targeted in medusa mode but still shows up on regular view mode
						}
						//time to cancel
						for( int group = 0; group < 9; group++ )
							if( column_occupancy & (1<<group) )
								for( int position = 0; position < 9; position++ )
									//if it's not part of the fish, cancel it
									if( !mark_map[COLUMNS[group][position]] )
									{
										/*if( candid_map[candidate][COLUMNS[group][position]] )
										{
debug << "[BY ROW][ELIMINATION] Candidate=" << candidate+1 << ". Cardinality=" << cardinality << ". Participants in rows ";
show_candidates(set);
debug << "\n";
										}*/
										candid_map[candidate][COLUMNS[group][position]] = false;
									}
					}
				}

				//same for columns
				if( column_participants == cardinality )
				{
					bool mark_map[81];
					for( int i = 0; i < 81; i++ )
						mark_map[i] = false;

					for( int column = 0; column < 9; column++ )
						if( set & (1<<column) )
							for( int position = 0; position < 9; position++ )
								if( candid_map[candidate][COLUMNS[column][position]] )
									mark_map[COLUMNS[column][position]] = true;

					int row_occupancy = 0;
					for( int group = 0; group < 9; group++ )
						for( int position = 0; position < 9; position++ )
							if( mark_map[ROWS[group][position]] )
								row_occupancy |= (1<<group);

					if( BIT_COUNTER[row_occupancy] == cardinality )
					{
						if( flag & SHOW_HINT_MODE )
						{
							for( int i = 0; i < 81; i++ )
								if( mark_map[i] )
									p->hint[i] |= (1<<candidate);
						}

						for( int group = 0; group < 9; group++ )
							if( row_occupancy & (1<<group) )
								for( int position = 0; position < 9; position++ )
									if( !mark_map[ROWS[group][position]] )
									{
										/*if( candid_map[candidate][ROWS[group][position]] )
										{
debug << "[BY COLUMN][ELIMINATION] Candidate=" << candidate+1 << ". Cardinality=" << cardinality << ". Participants in columns ";
show_candidates(set);
debug << "\n";
										}*/
										candid_map[candidate][ROWS[group][position]] = false;
									}
					}
				}
			}
		}
	}
//debug.flush();
	//reassemble the candid_map into a map to return
	for( int candidate = 0; candidate < 9; candidate++ )
		for( int position = 0; position < 81; position++ )
			if( candid_map[candidate][position] )
				mymap[position] |= (1<<candidate);

	///set up the counts for instances of each candidate in each row and column

	///count from 0 to 512 and pay attention when there are 2, 3, 4 or 5 bits.  those represent our "fish"

	///see if the bits correspond to active rows or columns.  if they do, mark those candidates and see if they exist in the same number of columns or rows
		///if they do, you have a(n) x-wing/swordfish/jellyfish/squirmbag/etc..

	///that's about all there is to it.  for instance, if you're checking for jellyfish, you'd check the rows/columns for 2 to 4 instances of the candidate

}

/**
i'm having ideas.  i think i'm going to use the same rules for everything... then i'll decide if it's a normal or extended polygon at the end by 
comparing the values of the cells.  if the cells all have the same value, it's normal, if not, it's extended.  so i can just use one set of rules
to find my polygons.

ok, after finding out that my polygons thing was buggy (turning up false negatives) i worked out a new set of constraints that seems complete and
seems to be working so far.  so i'll move on to something else now.
*/

void valid_corner( int current_position, int *breakmap, vector <int> *pending_friends, bool *valid )
{
/**
use a bool to see if the cell can find a valid configuration of peers, then check to make sure all the pending friends are in the valid map.
if both conditions are true, the cell is valid.  otherwise, it is invalid.

i think there's one case this won't pick up on... see, for regular polygons, everything is straightforward.  for extended polygons, things get
a little tricky.  what i was thinking of doing to check for partial matches was to ... ok, i just thought of a way i could solve this problem...

in order to check for valid partial matches, i should do the following.
1	list the cells in the row or column that have only two candidates and are not equal to current_position
2	look through those cells for ones that have exactly one candidate in common with cell i'm checking
		i can check by saying if( BIT_COUNTER[ map[current_position] & map[other_position] ] == 1 )
3	of the remaining cells, look at them two at a time and 
		if( BIT_COUNTER[ map[current_position] & (map[other_position_1] & map[other_position_1]) ] == 1 )
		then other_position_1 and other_position_2 are both potential candidates and they both go into the pending_friends thingie.
	the reason i don't have to check to make sure that (map[other_position_1] & map[other_position_1]) only have one in common is that each cell
	only has two candidates, so in order to have more than one in common, they'd have to be a naked double.  so they couldn't have anything in 
	common with current_position.

this looks like a good solution.  i'll go with it and see how it works.
NOTE remember to check cells before making comparisons to make sure they're all in different blocks.  if they're not, they can't participate.
*/

	int block_match_position;
	int block_match_count = 0;
	for( int i = 0; i < 9; i++ )
		if( current_position != BLOCKS[BLOCK_LOOKUP[current_position]][i] )
			if( breakmap[current_position] == breakmap[BLOCKS[BLOCK_LOOKUP[current_position]][i]] )
			{
				block_match_count++;
				block_match_position = BLOCKS[BLOCK_LOOKUP[current_position]][i];
			}

	bool block_match_row, block_match_column, block_match_general;
	if( block_match_count == 1 )
	{
		if( ROW_LOOKUP[current_position] == ROW_LOOKUP[block_match_position] )
		{
			block_match_row = true;
			block_match_column = block_match_general = false;

			int column_match_position;
			int column_match_count = 0;
			for( int i = 0; i < 9; i++ )
				if( current_position != COLUMNS[COLUMN_LOOKUP[current_position]][i] )
					if( breakmap[current_position] == breakmap[COLUMNS[COLUMN_LOOKUP[current_position]][i]] )
					{
						column_match_count++;
						column_match_position = COLUMNS[COLUMN_LOOKUP[current_position]][i];
					}

			if( column_match_count == 1 )
			{
				pending_friends->push_back(block_match_position);
				pending_friends->push_back(column_match_position);
			}
			else
			{
				///look for partial matches
			}
		}
		else if( COLUMN_LOOKUP[current_position] == COLUMN_LOOKUP[block_match_position] )
		{
			block_match_column = true;
			block_match_row = block_match_general = false;

			int row_match_position;
			int row_match_count = 0;
			for( int i = 0; i < 9; i++ )
				if( current_position != ROWS[ROW_LOOKUP[current_position]][i] )
					if( breakmap[current_position] == breakmap[ROWS[ROW_LOOKUP[current_position]][i]] )
					{
						row_match_count++;
						row_match_position = ROWS[ROW_LOOKUP[current_position]][i];
					}

			if( row_match_count == 1 )
			{
				pending_friends->push_back(block_match_position);
				pending_friends->push_back(row_match_position);
			}
			else
			{
				///look for partial matches
			}
		}
		else//it must be somewhere else in the block...
		{
			block_match_general = true;
			block_match_row = block_match_column = false;

			///check rows and columns for exact and partial matches... yeah.

		}
	}
}

void almost_deadly_patterns(int *mymap, Puzzle *p, int flags, int techniques)///this should handle normal and extended normal and one-sided deadly patterns
{
	for( int i = 0; i < 81; i++ )
		mymap[i] = p->candidate_map[i];

	for( int position = 0; position < 81; position++ )
	{
		if( BIT_COUNTER[p->candidate_map[position]] == 3 )
		{
			//build one mockup for each of the three guys to see who's absence exposes the deadly pattern
			for( int candidate = 0; candidate < 9; candidate++ )
			{
				if( p->candidate_map[position] & (1<<candidate) )
				{
					vector<int> interesting_friends;
					interesting_friends.push_back(position);
					vector<int> pending_friends;

					vector<int> valid_friends;//i have high hopes for this function.  i think it'll be a major improvement.
					valid_friends.resize(0);

					//so i'll step through the valid friends, copying them to the updating vector.  if i find that one isn't valid, i won't
					//copy it and i'll set the updating flag true.  then i'll do it again.  in that way, i'll "prune" the vector until it
					//stops updating either because there are no more valid friends or because i've collapsed onto a deadly pattern.
					vector<int> updating_friends;
					bool updating;

					int breakmap[81];
					bool pending[81];
					for( int i = 0; i < 81; i++ )
					{
						breakmap[i] = p->candidate_map[i];
						pending[i] = false;
					}

					if( techniques & TECH_UNIQUENESS_ONE_SIDED_ADP )//for one-sided stuff, get rid of this also if it exists
					{
						int block = BLOCK_LOOKUP[position];
						int matches = 0;
						for( int i = 0; i < 9; i++ )
							if( position != BLOCKS[block][i] )
								if( breakmap[position] == breakmap[BLOCKS[block][i]] )
									matches++;
						if( matches == 1 )//otherwise, i'm not sure what to do with it... and it seems to keep me out of trouble...
							for( int i = 0; i < 9; i++ )
							{
								if( breakmap[position] == breakmap[BLOCKS[block][i]] )
									if( position != BLOCKS[block][i] )//if i'm not looking at myself
										if( ROW_LOOKUP[position] == ROW_LOOKUP[BLOCKS[block][i]] ||
											COLUMN_LOOKUP[position] == COLUMN_LOOKUP[BLOCKS[block][i]])//i already know they share a block
											breakmap[BLOCKS[block][i]] &= ~(1<<candidate);
							}
					}

					breakmap[position] &= ~(1<<candidate);//now that the one-sided case is taken care of, i'll take care of me
					pending[position] = true;

					do{///NOTE I am not checking for one-sided things yet.  when i check for those, it'll be before getting in here.
						int current_position = interesting_friends[interesting_friends.size()-1];
						interesting_friends.pop_back();
						pending_friends.resize(0);

/**first, look for the match in the block.  
	if it's in the same row, do something
	if it's in the same column, do something else
	if it's in neither, do something else

NOTE i just realized i could just run general rules for these polygons and then check for normality or extendedness afterward by comparing the cells
involved.  so i'll try that.  i'll just run through these on general rules and see how that works.

TODO write a function that will check to see if a thing is a valid member of a polygon.  send it a participant vector that it can fill and overload
the function so that it can take a "this position is valid" map or not.  if not, it'll make one with all true values and send that.  that'll be the
thing to do for finding "interesting" positions.  so i think that can work just fine.*/
/**
Ok.  here's an exhaustive list of the possible situations we can have for a corner of a valid polygon.

exact match in this block (required)
	if block match is in this row
		there should be an exact match in this column or
		there should be two partial matches in this column
	if block match is in this column
		there should be an exact match in this row or
		there should be two partial matches in this row
	if block match is in neither this row nor this column
		there should be an exact match in this row and two partial matches in this column or
		there should be an exact match in this column and two partial matches in this row or
		there should be two partial matches in both this row and this column

so those are the seven types of corner for a valid polygon.
*/

						///figure out where the block match is
						

					}while(!interesting_friends.empty());//go until there are no more interesting friends to visit.

					///now make a mockup of the map and prune out guys who are not friends with valid friends.
					do{
						bool updating_friends[81];
						for( int i = 0; i < 81; i++ )
							updating_friends[i] = false;
						updating = false;

						//check against this to see if our friends are valid
						bool valid_members[81];
						for( int i = 0; i < 81; i++ )
							valid_members[i] = false;
						for( int i = 0; i < valid_friends.size(); i++ )
							valid_members[valid_friends[i]] = true;

						for( int check = 0; check < 81; check++ )
						{
							if( valid_members[check] )
							{
								int current_position = check;
	
								if( techniques & TECH_UNIQUENESS_ADP )
								{
									///check for the first case
									int exact_match_row = 0, exact_match_column = 0, exact_match_block = 0;
									for( int compare = 0; compare < 20; compare++ )
										if( breakmap[current_position] == breakmap[CELL_FRIENDS[current_position][compare]] )
										{
											//if it shares a block and a row or column, only register the block.  that'll make things simpler, i think
											if( 	(BLOCK_LOOKUP[current_position] == BLOCK_LOOKUP[CELL_FRIENDS[current_position][compare]] && 
												ROW_LOOKUP[current_position] == ROW_LOOKUP[CELL_FRIENDS[current_position][compare]]) || 
												(BLOCK_LOOKUP[current_position] == BLOCK_LOOKUP[CELL_FRIENDS[current_position][compare]] && 
												COLUMN_LOOKUP[current_position] == COLUMN_LOOKUP[CELL_FRIENDS[current_position][compare]]) )
											{
												if( valid_members[CELL_FRIENDS[current_position][compare]] )
												///this time, only proceed if this guy is a valid member
													exact_match_block++;
											}
											else if( ROW_LOOKUP[current_position] == ROW_LOOKUP[CELL_FRIENDS[current_position][compare]] )
											{
												if( valid_members[CELL_FRIENDS[current_position][compare]] )
													exact_match_row++;
											}
											else if( COLUMN_LOOKUP[current_position] == COLUMN_LOOKUP[CELL_FRIENDS[current_position][compare]] )
											{
												if( valid_members[CELL_FRIENDS[current_position][compare]] )
												{
													exact_match_column++;
												}
											}
										}
									if( (exact_match_block == 1 && exact_match_row == 1) || (exact_match_block == 1 && exact_match_column == 1) )
										updating_friends[current_position] = true;
								}

								if( techniques & TECH_UNIQUENESS_EXTENDED_ADP )
								{
									///check for the second case
									int exact_match_block = 0, exact_match_block_row = 0, exact_match_block_column = 0,
									exact_match_row = 0, exact_match_column = 0, partial_match_row = 0, partial_match_column = 0,
									partial_match_group = (1<<BLOCK_LOOKUP[current_position]);
									for( int compare = 0; compare < 20; compare++ )
									{
										partial_match_group = 0;
										if( breakmap[current_position] == breakmap[CELL_FRIENDS[current_position][compare]] && valid_members[CELL_FRIENDS[current_position][compare]] )
										{//if it's not in the block, i don't care.  otherwise this would be case 1 and i'm handling case 2 here
											if( BLOCK_LOOKUP[current_position] == BLOCK_LOOKUP[CELL_FRIENDS[current_position][compare]] && ROW_LOOKUP[current_position] == ROW_LOOKUP[CELL_FRIENDS[current_position][compare]] )
												exact_match_block_row++;
											else if( BLOCK_LOOKUP[current_position] == BLOCK_LOOKUP[CELL_FRIENDS[current_position][compare]] && COLUMN_LOOKUP[current_position] == COLUMN_LOOKUP[CELL_FRIENDS[current_position][compare]] )
												exact_match_block_column++;
											else if( BLOCK_LOOKUP[current_position] == BLOCK_LOOKUP[CELL_FRIENDS[current_position][compare]] )
												exact_match_block++;
											else if( ROW_LOOKUP[current_position] == ROW_LOOKUP[CELL_FRIENDS[current_position][compare]] )
												exact_match_row++;
											else if( COLUMN_LOOKUP[current_position] == COLUMN_LOOKUP[CELL_FRIENDS[current_position][compare]] )
												exact_match_column++;
										}
										else if( BIT_COUNTER[breakmap[CELL_FRIENDS[current_position][compare]]] == 2 && valid_members[CELL_FRIENDS[current_position][compare]] && !(partial_match_group & (1<<BLOCK_LOOKUP[CELL_FRIENDS[current_position][compare]])) && BLOCK_LOOKUP[current_position] != BLOCK_LOOKUP[CELL_FRIENDS[current_position][compare]])
										{
											//if this, then they only have one thing in common and i'm interested.
											if( ROW_LOOKUP[current_position] == ROW_LOOKUP[CELL_FRIENDS[current_position][compare]] )
											{
												partial_match_row++;
												partial_match_group |= (1<<BLOCK_LOOKUP[CELL_FRIENDS[current_position][compare]]);
											}
											else if( COLUMN_LOOKUP[current_position] == COLUMN_LOOKUP[CELL_FRIENDS[current_position][compare]] )
											{
												partial_match_column++;
												partial_match_group |= (1<<BLOCK_LOOKUP[CELL_FRIENDS[current_position][compare]]);
											}
										}
									}

									if( 	(exact_match_block_row == 1 && partial_match_column >= 2) || 
										(exact_match_block_row == 1 && exact_match_column == 1)   || 
										(exact_match_block_column == 1 && partial_match_row >= 2) || 
										(exact_match_block_column == 1 && exact_match_row == 1)   || 
										(exact_match_block == 1 && 
											((exact_match_row == 1 && partial_match_column >= 2) || 
											(exact_match_row == 1 && exact_match_column == 1)    || 
											(exact_match_column == 1 && partial_match_row >= 2))) )
										updating_friends[current_position] = true;
								}
							}
						}

						updating = false;
						valid_friends.resize(0);
						for( int i = 0; i < 81; i++ )
						{
							if( valid_members[i] && !updating_friends[i] )
								updating = true;
							if( updating_friends[i] )
								valid_friends.push_back(i);
						}
					}while(updating);//go until there are no more interesting friends to visit.

///make lists of the things they have in common and get rid of the candidate from that cell.
					if( !valid_friends.empty() )
					{
						if( techniques & TECH_UNIQUENESS_ONE_SIDED_ADP )//this is the one-sided case
						{
							///this is a lower complexity solution than running through 20x20 for their cell friends.
							bool one[81];
							bool two[81];
							bool found_one = false;
							int block = BLOCK_LOOKUP[position];
	
							for( int i = 0; i < 81; i++ )
								one[i] = two[i] = false;
	
							for( int i = 0; i < valid_friends.size(); i++ )
							{
								if( BIT_COUNTER[p->candidate_map[valid_friends[i]]] == 3 )
								{
									if( found_one )//then save it to two
									{
										for( int j = 0; j < 20; j++ )
											two[CELL_FRIENDS[valid_friends[i]][j]] = true;
									}
									else
									{
										for( int j = 0; j < 20; j++ )
											one[CELL_FRIENDS[valid_friends[i]][j]] = true;
										found_one = true;
									}
								}
							}

							for( int i = 0; i < 81; i++ )
								if( one[i] && two[i] )
									mymap[i] &= ~(1<<candidate);
						}
						else
						{
							for( int i = 0; i < valid_friends.size(); i++ )
								if( BIT_COUNTER[p->candidate_map[valid_friends[i]]] == 3 )
									mymap[valid_friends[i]] = (1<<candidate);
						}

						if( flags & SHOW_HINT_MODE )
							for( int i = 0; i < valid_friends.size(); i++ )
								p->hint[valid_friends[i]] |= (1<<10);
					}
				}
			}
		}
	}
//debug.flush();
}

/*
Bilocation Graphs (nonrepetitive and repetitive)

The Description:


The Algorithm:
*/
/**
i wonder if it'd be a good idea to make a node class down here and use a linked list to keep track of vertices and edges and stuff.
this could be a pretty simple algorithm if i do it correctly.
i could have things like these in a node class
bool visited
node next
node prev
int next_edge
int prev_edge

so here's a rough algorithm...
i start the vector? someplace where i'm on one of two instances of a candidate.  i jump to the other instance, if that spot is one of two instances and
i haven't visited it, then jump to it.  do this for the regular groups like row/column/box.  as you push guys, mark them visited.  as you pop guys,
mark them unvisited.  this is important so they can be picked up as part of another group later.  so run around like that.  say if the interesting
guy is either unvisited or he is the origin, jump to him.  if you get back to the origin, take a look at your nodes.  for each node, set the candidates
for that node equal to next_edge | prev_edge.  that node has to be one or the other and in cases where next_edge == prev_edge, it'll take care of itself

there's a catch... for non-repetitive bilocation graphs, there should be no node for which next_edge == prev_edge.  for the repetitive type, there 
should be exactly one node for which next_edge == prev_edge.  I don't know why there can't be more than one.  i'll try to come up with some test cases
to figure out what having more than one node of that type would mean.  right now, i don't see why that couldn't work.
*/

/**
this is looking more and more like something that should be done with recursion.  if i could send an edge vector, that'd be nice.  i'd have to make a
class called edge... it'd have source and sink integers as well as an edge integer.  when you make a connection, just add an edge to the vector and 
mark the guys involved on the "visiting" bool and that's it.  if you have to back out, pop the vector and unvisit the point you're leaving.  if you
find your way back to the origin, return true and it'll come out of recursion, return true to the caller and you can analyze the vector.

to analyze the vector... i think the easiest thing to do would be to clear away the candidates from everything that was visited in the vector... then
to OR equal those positions with the edges between them.  because that's essentially what it boils down to.

before doing this, you have to make sure that there is, at most, one case in which two (consecutive?) edges have the same weight/color/whatever.
*/
class BiCell///i don't think this class is going to last much longer...
{
public:
	int row_contact[9];
	int column_contact[9];
	int block_contact[9];

	void init()
	{
		for( int i = 0; i < 9; i++ )
			row_contact[i] = column_contact[i] = block_contact[i] = null;
	}
};

class Edge
{
public:
	int source;
	int sink;
	int edge;

	Edge(int source, int sink, int edge)
	{
		this->source = source;
		this->sink = sink;
		this->edge = edge;
	}

	Edge()
	{
		Edge(null,null,null);
	}
};

class Link
{
public:
	vector<int> sink;
	vector<int> edge;

	Link()
	{
		sink.resize(0);
		edge.resize(0);
	}

	void push(int sink, int edge)
	{
		this->sink.push_back(sink);
		this->edge.push_back(edge);
	}

	void pop()
	{
		sink.pop_back();
		edge.pop_back();
	}

	int size()
	{
		return sink.size();
	}

	void resize(int size)
	{
		sink.resize(size);
		edge.resize(size);
	}
};

void recursive_bilocation( Puzzle *p, Link *graph, vector<Edge> *edges, int *mymap, bool *visited, int position, int techniques, int flags )
{
	/*if( !edges->empty() )
	{
		for( int i = 0; i < edges->size(); i++ )
			debug << (*edges)[i].source+1 << "(" << BITS_LOOKUP[(*edges)[i].edge]+1 << ")" << (*edges)[i].sink+1 << " ";
		debug << "\n\n";
	}*/

///NOTE this return condition has nothing to do with correctness.  it just takes a really long time to run this algorithm, so i cut it off there.
	if( edges->size() >= 10 )
		return;

/** NOTE i can reduce the complexity here significantly if i just make a list of links.  that way, if a cell only has one link for one candidate, i
don't run around 27 possibilities trying to find it.  i should just make a vector of links and then i'd just run through the size of the vector.  i
really think i could cut the time this takes by a large factor and possibly by enough to increase the number of edges possible

hmm... and it doesn't seem to be helping as much as i thought it'd be... such that i think i perceive a slight speedup... but i really can't be sure
*/
	if( !visited[position] )///you should only be looking from the origin if you're starting from there
	{
		visited[position] = true;
		for( int link = 0; link < graph[position].size(); link++ )
		{
			edges->push_back(Edge(position,graph[position].sink[link],graph[position].edge[link]));
			recursive_bilocation(p,graph,edges,mymap,visited,graph[position].sink[link],techniques,flags);
			edges->pop_back();
		}
		visited[position] = false;
	}
	else
	{
		if( edges->size() < 3 )///if there are fewer than three edges, it can't possibly be what we're looking for
			return;

		if( (*edges)[edges->size()-1].sink != (*edges)[0].source )
			return;

		///if you've already been here, it looks like a loop, so check it out and see if it's valid.

//debug << "Possible Cycle Found\n";
		bool valid = true;
		bool consecutive_found = false;
		int last_edge = (*edges)[edges->size()-1].edge;
		for( int i = 0; i < edges->size(); i++ )
		{
			if( last_edge == (*edges)[i].edge )
			{
				if( consecutive_found )
					valid = false;
				else
					consecutive_found = true;
			}
			last_edge = (*edges)[i].edge;
		}

		if( valid )
		{
			if( techniques & TECH_COLORING_BILOCATION )
			{
				bool something_changed = false;
				if( consecutive_found )
				{
/*debug << "Found a Repetitive Cycle\n";
for( int i = 0; i < edges->size(); i++ )
	debug << (*edges)[i].source+1 << "(" << BITS_LOOKUP[(*edges)[i].edge]+1 << ")" << (*edges)[i].sink+1 << " ";
debug << "\n\n";*/
					/*debug << "Found a Repetitive Cycle!\n";
					debug << "Cycle=";
					for( int i = 0; i < edges.size(); i++ )
						debug << edges[i].source+1 << "(" << BITS_LOOKUP[edges[i].edge] << ")" << edges[i].sink+1 << " ";
					debug << "\n";*/

					int last_edge = (*edges)[edges->size()-1].edge;
					for( int i = 0; i < edges->size(); i++ )
					{
						if( last_edge == (*edges)[i].edge )
						{
							if( mymap[(*edges)[i].source] != last_edge )
								something_changed = true;
							mymap[(*edges)[i].source] = last_edge;
						}
						last_edge = (*edges)[i].edge;
					}
				}
				else
				{
/*debug << "Found a Non-Repetitive Cycle\n";
for( int i = 0; i < edges->size(); i++ )
	debug << (*edges)[i].source+1 << "(" << BITS_LOOKUP[(*edges)[i].edge]+1 << ")" << (*edges)[i].sink+1 << " ";
debug << "\n\n";*/
					for( int i = 0; i < edges->size(); i++ )
					{
						if( mymap[(*edges)[i].sink] != ((*edges)[i].edge | (*edges)[(i+1)%edges->size()].edge) )
							something_changed = true;
						mymap[(*edges)[i].sink] = ((*edges)[i].edge | (*edges)[(i+1)%edges->size()].edge);
					}
				}

				if( flags & SHOW_HINT_MODE && something_changed )
				{
					for( int i = 0; i < edges->size(); i++ )
						p->hint[(*edges)[i].source] = true;
				}
			}

			if( techniques & TECH_COLORING_BIVALUE && !consecutive_found )///consecutives break this technique, supposedly
			{
/*debug << "Bivalue Cycle Found\n";
for( int i = 0; i < edges->size(); i++ )
	debug << (*edges)[i].source+1 << "(" << BITS_LOOKUP[(*edges)[i].edge]+1 << ")" << (*edges)[i].sink+1 << " ";
debug << "\n\n";*/
				bool something_changed = false;
				///get rid of the other instances of this candidate in the group along the edge
				for( int cancel = 0; cancel < edges->size(); cancel++ )
				{
					///i don't use else if because the source and sink could be in the same block && (row || column)
					if( ROW_LOOKUP[(*edges)[cancel].source] == ROW_LOOKUP[(*edges)[cancel].sink] )
					{
						int row = ROW_LOOKUP[(*edges)[cancel].source];
						for( int find = 0; find < 9; find++ )
							if( (*edges)[cancel].source != ROWS[row][find] && (*edges)[cancel].sink != ROWS[row][find] )
							{
								if( mymap[ROWS[row][find]] & (*edges)[cancel].edge )
									something_changed = true;
								mymap[ROWS[row][find]] &= ~(*edges)[cancel].edge;
							}
					}

					if( COLUMN_LOOKUP[(*edges)[cancel].source] == COLUMN_LOOKUP[(*edges)[cancel].sink] )
					{
						int column = COLUMN_LOOKUP[(*edges)[cancel].source];
						for( int find = 0; find < 9; find++ )
							if( (*edges)[cancel].source != COLUMNS[column][find] && (*edges)[cancel].sink != COLUMNS[column][find] )
							{
								if( mymap[COLUMNS[column][find]] & (*edges)[cancel].edge )
									something_changed = true;
								mymap[COLUMNS[column][find]] &= ~(*edges)[cancel].edge;
							}
					}

					if( BLOCK_LOOKUP[(*edges)[cancel].source] == BLOCK_LOOKUP[(*edges)[cancel].sink] )
					{
						int block = BLOCK_LOOKUP[(*edges)[cancel].source];
						for( int find = 0; find < 9; find++ )
							if( (*edges)[cancel].source != BLOCKS[block][find] && (*edges)[cancel].sink != BLOCKS[block][find] )
							{
								if( mymap[BLOCKS[block][find]] & (*edges)[cancel].edge )
									something_changed = true;
								mymap[BLOCKS[block][find]] &= ~(*edges)[cancel].edge;
							}
					}
				}

				if( flags & SHOW_HINT_MODE && something_changed )
				{
					for( int i = 0; i < edges->size(); i++ )
						p->hint[(*edges)[i].source] = true;
				}
			}
		}
	}
}

void bilocation_and_bivalue(int *mymap, Puzzle *p, int flags, int techniques)
{
	for( int i = 0; i < 81; i++ )
		mymap[i] = p->candidate_map[i];

	Link graph[81];
	for( int position = 0; position < 81; position++ )
	{
//debug << "position=" << position+1 << "\n";
		if( techniques & TECH_COLORING_BILOCATION )
		{
			for( int candidate = 0; candidate < 9; candidate++ )
			{
//debug << "\tcandidate=" << candidate+1 << "\n";
				if( p->candidate_map[position] & (1<<candidate) )
				{
					if( row_contains(p->candidate_map,ROW_LOOKUP[position],(1<<candidate)) == 2 )
						for( int find = 0; find < 9; find++ )
							if( position != ROWS[ROW_LOOKUP[position]][find] )
								if( p->candidate_map[ROWS[ROW_LOOKUP[position]][find]] & (1<<candidate) )
									graph[position].push(ROWS[ROW_LOOKUP[position]][find],(1<<candidate));
	
					if( column_contains(p->candidate_map,COLUMN_LOOKUP[position],(1<<candidate)) == 2 )
						for( int find = 0; find < 9; find++ )
							if( position != COLUMNS[COLUMN_LOOKUP[position]][find] )
								if( p->candidate_map[COLUMNS[COLUMN_LOOKUP[position]][find]] & (1<<candidate) )
									graph[position].push(COLUMNS[COLUMN_LOOKUP[position]][find],(1<<candidate));
	
					if( block_contains(p->candidate_map,BLOCK_LOOKUP[position],(1<<candidate)) == 2 )
						for( int find = 0; find < 9; find++ )
							if( position != BLOCKS[BLOCK_LOOKUP[position]][find] )
								if( p->candidate_map[BLOCKS[BLOCK_LOOKUP[position]][find]] & (1<<candidate) )
									graph[position].push(BLOCKS[BLOCK_LOOKUP[position]][find],(1<<candidate));
				}
//debug << "\t\trow_contact=" << graph[position].row_contact[candidate]+1 << "\n";
//debug << "\t\tcolumn_contact=" << graph[position].column_contact[candidate]+1 << "\n";
//debug << "\t\tblock_contact=" << graph[position].block_contact[candidate]+1 << "\n\n";
			}
		}

		if( techniques & TECH_COLORING_BIVALUE )
		{
			if( BIT_COUNTER[p->candidate_map[position]] == 2 )///remember, we're looking for bivalue cells
			{
				int row = ROW_LOOKUP[position];
				int column = COLUMN_LOOKUP[position];
				int block = BLOCK_LOOKUP[position];

				for( int find = 0; find < 9; find++ )
				{
					if( position != ROWS[row][find] )///first of all, i have to make sure he isn't me
						if( BIT_COUNTER[p->candidate_map[ROWS[row][find]]] == 2 )///if he is also a bivalue cell
							if( BIT_COUNTER[p->candidate_map[position] & p->candidate_map[ROWS[row][find]]] )///and we have things in common
								for( int common = 0; common < 9; common++ )///then let's find the things we have in common
									if( (p->candidate_map[position] & p->candidate_map[ROWS[row][find]]) & (1<<common) )
										graph[position].push(ROWS[row][find],(1<<common));///and record them

					if( position != COLUMNS[column][find] )
						if( BIT_COUNTER[p->candidate_map[COLUMNS[column][find]]] == 2 )
							if( BIT_COUNTER[p->candidate_map[position] & p->candidate_map[COLUMNS[column][find]]] )
								for( int common = 0; common < 9; common++ )
									if( (p->candidate_map[position] & p->candidate_map[COLUMNS[column][find]]) & (1<<common) )
										graph[position].push(COLUMNS[column][find],(1<<common));

					if( position != BLOCKS[block][find] )
						if( BIT_COUNTER[p->candidate_map[BLOCKS[block][find]]] == 2 )
							if( BIT_COUNTER[p->candidate_map[position] & p->candidate_map[BLOCKS[block][find]]] )
								for( int common = 0; common < 9; common++ )
									if( (p->candidate_map[position] & p->candidate_map[BLOCKS[block][find]]) & (1<<common) )
										graph[position].push(BLOCKS[block][find],(1<<common));
				}
			}
		}
	}

	bool visited[81];

	vector<Edge> edges;
	for( int position = 0; position < 81; position++ )
	{
		edges.resize(0);

		for( int i = 0; i < 81; i++ )
			visited[i] = false;

		recursive_bilocation(p,graph,&edges,mymap,visited,position,techniques,flags);
	}
//debug.flush();
}

void aligned_subset_exclusion()
{
}

class Map
{
public:
	int p[81];

	Map(int *map)
	{
		for( int i = 0; i < 81; i++ )
			p[i] = map[i];
	}

	Map()
	{
	}
};

class Change
{
public:
	int pivot;
	int color;
	bool changed[20];//one for each 'cell friend'

	Change()
	{
	}
};

void recursive_brute_force(Puzzle *p,int *map,vector<int> *vacancy_list,vector<Change> *changes,vector<Map> *solutions,int depth)
{
	if( vacancy_list->size() == depth )///you can't get here unless the solution is valid
	{
		solutions->push_back(Map(map));
	}
	else
	{
		for( int candidate = 0; candidate < 9; candidate++ )///try every available guy
		{
			if( map[(*vacancy_list)[depth]] & (1<<candidate) )
			{
				changes->push_back(Change());///so we can roll back the changes instead of making new maps all the time
				(*changes)[depth].color = (1<<candidate);
				for( int i = 0; i < 20; i++ )
					if( map[CELL_FRIENDS[(*vacancy_list)[depth]][i]] & (1<<candidate) )
					{
						map[CELL_FRIENDS[(*vacancy_list)[depth]][i]] &= ~(1<<candidate);
						(*changes)[depth].changed[i] = true;
					}
					else
						(*changes)[depth].changed[i] = false;
				(*changes)[depth].pivot = map[(*vacancy_list)[depth]];

				map[(*vacancy_list)[depth]] = (1<<candidate);

				recursive_brute_force(p,map,vacancy_list,changes,solutions,depth+1);

				map[(*vacancy_list)[depth]] = (*changes)[depth].pivot;
				for( int i = 0; i < 20; i++ )
					if( (*changes)[depth].changed[i] )
						map[CELL_FRIENDS[(*vacancy_list)[depth]][i]] |= (1<<candidate);
				changes->pop_back();
			}
		}
	}
}

int brute_force(int *mymap, Puzzle *p)
{
	vector<int> vacancies;
	for( int i = 0; i < 81; i++ )
	{
		if( BIT_COUNTER[p->candidate_map[i]] )
			vacancies.push_back(i);
		mymap[i] = p->candidate_map[i];
	}
	vector<Map> solutions;
	vector<Change> changes;
	recursive_brute_force(p,mymap,&vacancies,&changes,&solutions,0);

	if( solutions.size() )
		for( int i = 0; i < 81; i++ )
			mymap[i] = solutions[0].p[i];

	return solutions.size();
}

void draw_puzzle(int ybox, int xbox)
{
	for( int y = 0; y < 10; y++ )
		for( int x = 0; x < 10; x++ )
			if( x % 3 == 0 || y % 3 == 0 )
			{
				attron(COLOR_PAIR(1));
				mvaddch(y*ybox,x*xbox,'+');
				attroff(COLOR_PAIR(1));
			}
			else
				mvaddch(y*ybox,x*xbox,'+');
	for( int y = 0; y < 10; y++ )
		for( int x = 0; x < 9; x++ )
			if( y % 3 == 0 )
			{
				attron(COLOR_PAIR(1));
				mvhline(y*ybox,x*xbox+1,'-',xbox-1);
				attroff(COLOR_PAIR(1));
			}
			else
				mvhline(y*ybox,x*xbox+1,'-',xbox-1);
	for( int y = 0; y < 9; y++ )
		for( int x = 0; x < 10; x++ )
			if( x % 3 == 0 )
			{
				attron(COLOR_PAIR(1));
				mvvline(y*ybox+1,x*xbox,'|',ybox-1);
				attroff(COLOR_PAIR(1));
			}
			else
				mvvline(y*ybox+1,x*xbox,'|',ybox-1);
}

void fill_blocks(int ybox, int xbox, int curs_y, int curs_x, Puzzle *p, int flags)
{
	for( int y = 0; y < 9; y++ )
	{
		for( int x = 0; x < 9; x++ )
		{
			/*mvprintw(y*ybox+1,x*xbox+1,"row %i",ROW_LOOKUP[y*9+x]);
			mvprintw(y*ybox+2,x*xbox+1,"col %i",COLUMN_LOOKUP[y*9+x]);		//debugging code
			mvprintw(y*ybox+3,x*xbox+1,"blk %i",BLOCK_LOOKUP[y*9+x]);*/

			if( p->original_map[y*9+x] != 0 && !(flags & DEBUG_MODE || flags & EXTREME_DEBUG_MODE) )
			{
				if( curs_x == x && curs_y == y )
					attron(COLOR_PAIR(6));
				else
					attron(COLOR_PAIR(2));

				mvaddch(y*ybox+1,x*xbox+1,' ');
				mvaddch(y*ybox+1,x*xbox+2,' ');
				mvaddch(y*ybox+1,x*xbox+3,' ');
				mvaddch(y*ybox+1,x*xbox+4,' ');
				mvaddch(y*ybox+1,x*xbox+5,' ');
				mvaddch(y*ybox+1,x*xbox+6,' ');
				mvaddch(y*ybox+1,x*xbox+7,' ');
				mvaddch(y*ybox+2,x*xbox+1,' ');
				mvaddch(y*ybox+2,x*xbox+2,' ');
				mvaddch(y*ybox+2,x*xbox+3,' ');
				mvaddch(y*ybox+2,x*xbox+4,char(p->original_map[y*9+x]+48));
				mvaddch(y*ybox+2,x*xbox+5,' ');
				mvaddch(y*ybox+2,x*xbox+6,' ');
				mvaddch(y*ybox+2,x*xbox+7,' ');
				mvaddch(y*ybox+3,x*xbox+1,' ');
				mvaddch(y*ybox+3,x*xbox+2,' ');
				mvaddch(y*ybox+3,x*xbox+3,' ');
				mvaddch(y*ybox+3,x*xbox+4,' ');
				mvaddch(y*ybox+3,x*xbox+5,' ');
				mvaddch(y*ybox+3,x*xbox+6,' ');
				mvaddch(y*ybox+3,x*xbox+7,' ');

				if( curs_x == x && curs_y == y )
					attroff(COLOR_PAIR(6));
				else
					attroff(COLOR_PAIR(2));
			}
			else if( p->solution_map[y*9+x] != 0 && !(flags & DEBUG_MODE || flags & EXTREME_DEBUG_MODE) )
			{
				if( curs_x == x && curs_y == y )
					attron(COLOR_PAIR(7));
				else
					attron(COLOR_PAIR(3));

				mvaddch(y*ybox+1,x*xbox+1,' ');
				mvaddch(y*ybox+1,x*xbox+2,' ');
				mvaddch(y*ybox+1,x*xbox+3,' ');
				mvaddch(y*ybox+1,x*xbox+4,' ');
				mvaddch(y*ybox+1,x*xbox+5,' ');
				mvaddch(y*ybox+1,x*xbox+6,' ');
				mvaddch(y*ybox+1,x*xbox+7,' ');
				mvaddch(y*ybox+2,x*xbox+1,' ');
				mvaddch(y*ybox+2,x*xbox+2,' ');
				mvaddch(y*ybox+2,x*xbox+3,' ');
				mvaddch(y*ybox+2,x*xbox+4,char(p->solution_map[y*9+x]+48));
				mvaddch(y*ybox+2,x*xbox+5,' ');
				mvaddch(y*ybox+2,x*xbox+6,' ');
				mvaddch(y*ybox+2,x*xbox+7,' ');
				mvaddch(y*ybox+3,x*xbox+1,' ');
				mvaddch(y*ybox+3,x*xbox+2,' ');
				mvaddch(y*ybox+3,x*xbox+3,' ');
				mvaddch(y*ybox+3,x*xbox+4,' ');
				mvaddch(y*ybox+3,x*xbox+5,' ');
				mvaddch(y*ybox+3,x*xbox+6,' ');
				mvaddch(y*ybox+3,x*xbox+7,' ');

				if( curs_x == x && curs_y == y )
					attroff(COLOR_PAIR(7));
				else
					attroff(COLOR_PAIR(3));
			}
			else if( p->suggestion_map[y*9+x] != 0 && !(flags & DEBUG_MODE || flags & EXTREME_DEBUG_MODE) )
			{
				if( curs_x == x && curs_y == y )
					attron(COLOR_PAIR(8));
				else
					attron(COLOR_PAIR(4));

				mvaddch(y*ybox+1,x*xbox+1,' ');
				mvaddch(y*ybox+1,x*xbox+2,' ');
				mvaddch(y*ybox+1,x*xbox+3,' ');
				mvaddch(y*ybox+1,x*xbox+4,' ');
				mvaddch(y*ybox+1,x*xbox+5,' ');
				mvaddch(y*ybox+1,x*xbox+6,' ');
				mvaddch(y*ybox+1,x*xbox+7,' ');
				mvaddch(y*ybox+2,x*xbox+1,' ');
				mvaddch(y*ybox+2,x*xbox+2,' ');
				mvaddch(y*ybox+2,x*xbox+3,' ');
				mvaddch(y*ybox+2,x*xbox+4,char(p->suggestion_map[y*9+x]+48));
				mvaddch(y*ybox+2,x*xbox+5,' ');
				mvaddch(y*ybox+2,x*xbox+6,' ');
				mvaddch(y*ybox+2,x*xbox+7,' ');
				mvaddch(y*ybox+3,x*xbox+1,' ');
				mvaddch(y*ybox+3,x*xbox+2,' ');
				mvaddch(y*ybox+3,x*xbox+3,' ');
				mvaddch(y*ybox+3,x*xbox+4,' ');
				mvaddch(y*ybox+3,x*xbox+5,' ');
				mvaddch(y*ybox+3,x*xbox+6,' ');
				mvaddch(y*ybox+3,x*xbox+7,' ');

				if( curs_x == x && curs_y == y )
					attroff(COLOR_PAIR(8));
				else
					attroff(COLOR_PAIR(4));
			}
			else//we'll default to the candidate map
			{
				//i want highlights to work in extreme debugging mode
				if( p->hint[y*9+x] && !(flags & DEBUG_MODE) )
				{
					if( curs_x == x && curs_y == y )
						attron(COLOR_PAIR(12));
					else
						attron(COLOR_PAIR(11));
				}
				else if( p->highlight_map[y*9+x] && !(flags & DEBUG_MODE) )
				{
					if( curs_x == x && curs_y == y )
						attron(COLOR_PAIR(10));
					else
						attron(COLOR_PAIR(9));
				}
				else if( curs_x == x && curs_y == y )
					attron(COLOR_PAIR(5));

				mvaddch(y*ybox+1,x*xbox+1,' ');
				if( p->candidate_map[y*9+x] & (1<<0) )	mvaddch(y*ybox+1,x*xbox+2,'1');
				else					mvaddch(y*ybox+1,x*xbox+2,' ');
				mvaddch(y*ybox+1,x*xbox+3,' ');
				if( p->candidate_map[y*9+x] & (1<<1) )	mvaddch(y*ybox+1,x*xbox+4,'2');
				else					mvaddch(y*ybox+1,x*xbox+4,' ');
				mvaddch(y*ybox+1,x*xbox+5,' ');
				if( p->candidate_map[y*9+x] & (1<<2) )	mvaddch(y*ybox+1,x*xbox+6,'3');
				else					mvaddch(y*ybox+1,x*xbox+6,' ');
				mvaddch(y*ybox+1,x*xbox+7,' ');
				mvaddch(y*ybox+2,x*xbox+1,' ');
				if( p->candidate_map[y*9+x] & (1<<3) )	mvaddch(y*ybox+2,x*xbox+2,'4');
				else					mvaddch(y*ybox+2,x*xbox+2,' ');
				mvaddch(y*ybox+2,x*xbox+3,' ');
				if( p->candidate_map[y*9+x] & (1<<4) )	mvaddch(y*ybox+2,x*xbox+4,'5');
				else					mvaddch(y*ybox+2,x*xbox+4,' ');
				mvaddch(y*ybox+2,x*xbox+5,' ');
				if( p->candidate_map[y*9+x] & (1<<5) )	mvaddch(y*ybox+2,x*xbox+6,'6');
				else					mvaddch(y*ybox+2,x*xbox+6,' ');
				mvaddch(y*ybox+2,x*xbox+7,' ');
				mvaddch(y*ybox+3,x*xbox+1,' ');
				if( p->candidate_map[y*9+x] & (1<<6) )	mvaddch(y*ybox+3,x*xbox+2,'7');
				else					mvaddch(y*ybox+3,x*xbox+2,' ');
				mvaddch(y*ybox+3,x*xbox+3,' ');
				if( p->candidate_map[y*9+x] & (1<<7) )	mvaddch(y*ybox+3,x*xbox+4,'8');
				else					mvaddch(y*ybox+3,x*xbox+4,' ');
				mvaddch(y*ybox+3,x*xbox+5,' ');
				if( p->candidate_map[y*9+x] & (1<<8) )	mvaddch(y*ybox+3,x*xbox+6,'9');
				else					mvaddch(y*ybox+3,x*xbox+6,' ');
				mvaddch(y*ybox+3,x*xbox+7,' ');

				if( p->hint[y*9+x] && !(flags & DEBUG_MODE) )
				{
					if( curs_x == x && curs_y == y )
						attroff(COLOR_PAIR(12));
					else
						attroff(COLOR_PAIR(11));
				}
				if( p->highlight_map[y*9+x] && !(flags & DEBUG_MODE) )
				{
					if( curs_x == x && curs_y == y )
						attroff(COLOR_PAIR(10));
					else
						attroff(COLOR_PAIR(9));
				}
				else if( curs_x == x && curs_y == y )
					attroff(COLOR_PAIR(5));
			}
		}
	}
}

void process_and_display(Puzzle *p,int cursor_position_x,int cursor_position_y,int *techniques,int user_actions,int tech_options_position,int highlight_these,int *trio)
{
	if( user_actions & SINGLE_PASS_MODE )
		solver_and_menu(user_actions, techniques, tech_options_position, p);
	else
		do{
			solver_and_menu(user_actions, techniques, tech_options_position, p);
		}while(p->update());

	//here, if hint mode is selected, then make a copy of the current candidate map, clear the candidates and compare the two.
	//if they differ, declare that moves can be made using the selected techniques.
	if( user_actions & HINT_MODE )
	{
		int progress = 0;
		int unsolved = 0;
		int update_test[81];
		for( int copy = 0; copy < 81; copy++ )
			update_test[copy] = p->candidate_map[copy];
		p->prep_candidate_map();
		p->clear_suggestion_map();
		for( int compare = 0; compare < 81; compare++ )
		{
			if( BIT_COUNTER[p->candidate_map[compare]] > 1 )
				unsolved++;
			if( update_test[compare] != p->candidate_map[compare] )
				progress++;
		}
		mvprintw(38,30,"%i of %i UNSOLVED CELLS CAN BE UPDATED   ",progress,unsolved);
	}
	else
		mvprintw(38,30,"                                         ");

	if( user_actions & UPDATE_MODE )//this lets the user narrow down which techniques are being used to solve
	{
		//user_actions &= ~UPDATE_MODE;
		for( int position = 0; position < 81; position++ )
			p->disqualification_map[position] |= ALL_CANDIDATES & ~p->candidate_map[position];
	}

	if( user_actions & AUTOSOLVE )
	{
		p->autosolve();
		//user_actions &= ~AUTOSOLVE;//this was getting me in trouble, so i moved it down about ten lines
	}

	if( valid_solution(p) )
	{
		mvprintw(30,75,"Status: Solved  ");
		trio[0]++;
	}
	else if( everything_seems_ok(p->solution_map, p->candidate_map) )
	{
		mvprintw(30,75,"Status: Unsolved");
		trio[1]++;
	}
	else if( !(user_actions & AUTOSOLVE) )//this was bugging the shit out of me.  so just don't say it's invalid after an autosolve.
	{
		mvprintw(30,75,"Status: Invalid ");
		trio[2]++;
		debug << "Invalid\n";
	}
	//user_actions &= ~AUTOSOLVE;//ugh...

	///display
		///the number of cells in the original map
		///the number of solved cells
		///the number of unsolved cells
	int factory_placements = 0;	//original placements
	int aftermarket_placements = 0;	//solved cells
	for( int check = 0; check < 81; check++ )
	{
		if( p->original_map[check] )
			factory_placements ++;
		if( p->solution_map[check] )
			aftermarket_placements ++;
	}
	mvprintw(27,75,"Original: %i  ",factory_placements);
	mvprintw(28,75,"Solved:   %i  ",aftermarket_placements-factory_placements);
	mvprintw(29,75,"Unsolved: %i  ",81-aftermarket_placements);

	for( int position = 0; position < 81; position++ )
		p->candidate_map[position] &= ~(ALL_CANDIDATES & highlight_these);

	draw_puzzle(ybox, xbox);
	if( user_actions & MEDUSA_MODE )
		medusa_map(ybox, xbox, cursor_position_y, cursor_position_x, p, user_actions);
	else
		fill_blocks(ybox, xbox, cursor_position_y, cursor_position_x, p, user_actions);

	p->clear_hints();

	refresh();
}

void medusa_map(int ybox, int xbox, int curs_y, int curs_x, Puzzle *p, int flags)
{
	for( int guy = 0; guy < 9; guy++ )
	{
		int xbase = ((guy%3)*3)*xbox;
		int ybase = ((guy/3)*3)*ybox;
		for( int y = 0; y < 9; y++ )
		{
			for( int x = 0; x < 9; x++ )
			{
				mvaddch(ybase+(y+1)+(y/3),xbase+(x+1)*2+(x/3)*2-1,' ');

				if( p->hint[y*9+x] & (1<<guy) )
				{
					if( curs_x == x && curs_y == y )
						attron(COLOR_PAIR(12));
					else
						attron(COLOR_PAIR(11));
				}
				else if( (p->highlight_map[y*9+x] & (1<<guy)) && !(flags & DEBUG_MODE) )
				{
					if( curs_x == x && curs_y == y )
						attron(COLOR_PAIR(10));
					else
						attron(COLOR_PAIR(9));
				}
				else if( curs_x == x && curs_y == y )
				{
					if( BIT_COUNTER[p->candidate_map[y*9+x]] == 1 && (p->candidate_map[y*9+x] & (1<<guy)) )
						attron(COLOR_PAIR(11));
					else
						attron(COLOR_PAIR(5));
				}

				if( p->candidate_map[y*9+x] & (1<<guy) )	mvprintw(ybase+(y+1)+(y/3),xbase+(x+1)*2+(x/3)*2,"%i",guy+1);
				else						mvaddch(ybase+(y+1)+(y/3),xbase+(x+1)*2+(x/3)*2,' ');

				if( p->hint[y*9+x] & (1<<guy) )
				{
					if( curs_x == x && curs_y == y )
						attroff(COLOR_PAIR(12));
					else
						attroff(COLOR_PAIR(11));
				}
				if( (p->highlight_map[y*9+x] & (1<<guy)) && !(flags & DEBUG_MODE) )
				{
					if( curs_x == x && curs_y == y )
						attroff(COLOR_PAIR(10));
					else
						attroff(COLOR_PAIR(9));
				}
				else if( curs_x == x && curs_y == y )
				{
					if( BIT_COUNTER[p->candidate_map[y*9+x]] == 1 && (p->candidate_map[y*9+x] & (1<<guy)) )
						attroff(COLOR_PAIR(11));
					else
						attroff(COLOR_PAIR(5));
				}

				mvaddch(ybase+(y+1)+(y/3),xbase+(x+1)*2+(x/3)*2+1,' ');
			}
		}
	}
}

bool everything_seems_ok(int *solution, int *map)
{
///there are three main things i'm checking for
	//more than one instance of a candidate appears in one group of the solution, return false
	//if a naked single is equal to a solved cell, return false
	//if there's a group where an instances of each candidate does not appear in either the solution or the candidacy, return false
	//if a cell is not solved and has no candidates, return false

	for( int group = 0; group < 9; group++ )
	{
		int row_solution_tracker = 0;
		int column_solution_tracker = 0;
		int block_solution_tracker = 0;
		int row_instance_tracker = 0;
		int column_instance_tracker = 0;
		int block_instance_tracker = 0;

		for( int position = 0; position < 9; position++ )
		{
			///rows
			if( solution[ROWS[group][position]] )//if it's solved
				row_instance_tracker |= (1<<(solution[ROWS[group][position]]-1));
			else//take a look at the pencilmarks/candidate_map
				row_instance_tracker |= map[ROWS[group][position]];

			if( solution[ROWS[group][position]] )
			{
				if( row_solution_tracker & (1<<(solution[ROWS[group][position]]-1)) )
					return false;
				else
					row_solution_tracker |= (1<<(solution[ROWS[group][position]]-1));
			}

			if( BIT_COUNTER[map[ROWS[group][position]]] == 1 )
			{
				if( row_solution_tracker & map[ROWS[group][position]] )
					return false;
				else
					row_solution_tracker |= map[ROWS[group][position]];
			}

			if( !solution[ROWS[group][position]] && !BIT_COUNTER[map[ROWS[group][position]]] )
				return false;

			///columns
			if( solution[COLUMNS[group][position]] )
				column_instance_tracker |= (1<<(solution[COLUMNS[group][position]]-1));
			else
				column_instance_tracker |= map[COLUMNS[group][position]];

			if( solution[COLUMNS[group][position]] )
			{
				if( column_solution_tracker & (1<<(solution[COLUMNS[group][position]]-1)) )
					return false;
				else
					column_solution_tracker |= (1<<(solution[COLUMNS[group][position]]-1));
			}

			if( BIT_COUNTER[map[COLUMNS[group][position]]] == 1 )
			{
				if( column_solution_tracker & map[COLUMNS[group][position]] )
					return false;
				else
					column_solution_tracker |= map[COLUMNS[group][position]];
			}

			if( !solution[COLUMNS[group][position]] && !BIT_COUNTER[map[COLUMNS[group][position]]] )
				return false;

			///blocks
			if( solution[BLOCKS[group][position]] )
				block_instance_tracker |= (1<<(solution[BLOCKS[group][position]]-1));
			else
				block_instance_tracker |= map[BLOCKS[group][position]];

			if( solution[BLOCKS[group][position]] )
			{
				if( block_solution_tracker & (1<<(solution[BLOCKS[group][position]]-1)) )
					return false;
				else
					block_solution_tracker |= (1<<(solution[BLOCKS[group][position]]-1));
			}

			if( BIT_COUNTER[map[BLOCKS[group][position]]] == 1 )
			{
				if( block_solution_tracker & map[BLOCKS[group][position]] )
					return false;
				else
					block_solution_tracker |= map[BLOCKS[group][position]];
			}

			if( !solution[BLOCKS[group][position]] && !BIT_COUNTER[map[BLOCKS[group][position]]] )
				return false;
		}

		//if this is true, then there are groups where
		if( row_instance_tracker != ALL_CANDIDATES || column_instance_tracker != ALL_CANDIDATES || block_instance_tracker != ALL_CANDIDATES )
			return false;
	}
	return true;
}

bool valid_solution(Puzzle *p)
{
	for( int group = 0; group < 9; group++ )
	{
		int check = 0;
		for( int position = 0; position < 9; position++ )
		{
			if( p->solution_map[ROWS[group][position]] )
				check |= (1<<(p->solution_map[ROWS[group][position]]-1));
			else if( BIT_COUNTER[p->candidate_map[ROWS[group][position]]] == 1 )
				check |= p->candidate_map[ROWS[group][position]];
		}
		if( check != ALL_CANDIDATES )
			return false;

		check = 0;
		for( int position = 0; position < 9; position++ )
		{
			if( p->solution_map[COLUMNS[group][position]] )
				check |= (1<<(p->solution_map[COLUMNS[group][position]]-1));
			else if( BIT_COUNTER[p->candidate_map[COLUMNS[group][position]]] == 1 )
				check |= p->candidate_map[COLUMNS[group][position]];
		}
		if( check != ALL_CANDIDATES )
			return false;

		check = 0;
		for( int position = 0; position < 9; position++ )
		{
			if( p->solution_map[BLOCKS[group][position]] )
				check |= (1<<(p->solution_map[BLOCKS[group][position]]-1));
			else if( BIT_COUNTER[p->candidate_map[BLOCKS[group][position]]] == 1 )
				check |= p->candidate_map[BLOCKS[group][position]];
		}
		if( check != ALL_CANDIDATES )
			return false;
	}

	return true;
}

void show_option(int user_actions, int techniques, int tech_position, int my_position)
{
	if( user_actions & TECH_MODE && tech_position == my_position )	attron(COLOR_PAIR(BLACK_ON_WHITE));
	if( techniques & TECH[my_position].responsibilities )
		mvprintw(TECH[my_position].x,TECH[my_position].y,"[X]%s",TECH[my_position].title);
	else
		mvprintw(TECH[my_position].x,TECH[my_position].y,"[ ]%s",TECH[my_position].title);
	if( user_actions & TECH_MODE && tech_position == my_position )	attroff(COLOR_PAIR(BLACK_ON_WHITE));
}

void solver_and_menu(int user_actions, int *techniques, int tech_options_position, Puzzle *p)
{
	int *hidden_singles_map = new int[81];
	int *block_rc_interactions_map = new int[81];
	int *block_block_interactions_map = new int[81];
	int *naked_subsets_map = new int[81];
	int *hidden_subsets_map = new int[81];
	int *coloring_map = new int[81];
	int *coloring_medusa_map = new int[81];
	int *forcing_chain_map = new int[81];
	int *xy_chain_map = new int[81];
	int *simple_fish_map = new int[81];
	int *almost_deadly_polygon_map = new int[81];
	int *almost_deadly_polygon_one_sided_map = new int[81];
	int *bilocation_map = new int[81];
	int *brute_force_map = new int[81];

	///basic eliminations
	show_option(user_actions,techniques[TECH[0].responsibility_class],tech_options_position,0);
	if( techniques[TECH[0].responsibility_class] & TECH[0].responsibilities )	p->prep_candidate_map();
	else										p->reset_candidate_map();

	int map[81];
	for( int i = 0; i < 81; i++ )
		map[i] = p->candidate_map[i];

	///block row/column interactions
	show_option(user_actions,techniques[TECH[1].responsibility_class],tech_options_position,1);
	if( techniques[TECH[1].responsibility_class] & TECH[1].responsibilities )
	{
		block_rc_interactions(block_rc_interactions_map, p->candidate_map, user_actions);
		for( int i = 0; i < 81; i++ )
			map[i] &= block_rc_interactions_map[i];
	}

	///block block interactions
	show_option(user_actions,techniques[TECH[2].responsibility_class],tech_options_position,2);
	if( techniques[TECH[2].responsibility_class] & TECH[2].responsibilities )
	{
		block_block_interactions(block_block_interactions_map, p->candidate_map, user_actions);
		for( int i = 0; i < 81; i++ )
			map[i] &= block_block_interactions_map[i];
	}

	///subsets
	show_option(user_actions,techniques[TECH[3].responsibility_class],tech_options_position,3);

	///naked subsets
	show_option(user_actions,techniques[TECH[4].responsibility_class],tech_options_position,4);
	if( techniques[TECH[4].responsibility_class] & TECH[4].responsibilities )
	{
		naked_subsets(naked_subsets_map,p,user_actions,techniques[TECH[4].responsibility_class]);
		for( int i = 0; i < 81; i++ )
			map[i] &= naked_subsets_map[i];
	}

	///hidden subsets
	show_option(user_actions,techniques[TECH[5].responsibility_class],tech_options_position,5);
	if( techniques[TECH[5].responsibility_class] & TECH[5].responsibilities )
	{
		hidden_subsets(hidden_subsets_map,p,user_actions,techniques[TECH[5].responsibility_class]);
		for( int i = 0; i < 81; i++ )
			map[i] &= hidden_subsets_map[i];
	}

	show_option(user_actions,techniques[TECH[6].responsibility_class],tech_options_position,6);
	show_option(user_actions,techniques[TECH[7].responsibility_class],tech_options_position,7);

	///hidden singles
	show_option(user_actions,techniques[TECH[8].responsibility_class],tech_options_position,8);
	if( techniques[TECH[8].responsibility_class] & TECH[8].responsibilities )
	{
		hidden_singles(hidden_singles_map,p->candidate_map,user_actions);
		for( int i = 0; i < 81; i++ )
			map[i] &= hidden_singles_map[i];
	}

	show_option(user_actions,techniques[TECH[9].responsibility_class],tech_options_position,9);
	show_option(user_actions,techniques[TECH[10].responsibility_class],tech_options_position,10);
	show_option(user_actions,techniques[TECH[11].responsibility_class],tech_options_position,11);
	show_option(user_actions,techniques[TECH[12].responsibility_class],tech_options_position,12);
	show_option(user_actions,techniques[TECH[13].responsibility_class],tech_options_position,13);
	show_option(user_actions,techniques[TECH[14].responsibility_class],tech_options_position,14);
	show_option(user_actions,techniques[TECH[15].responsibility_class],tech_options_position,15);
	show_option(user_actions,techniques[TECH[16].responsibility_class],tech_options_position,16);
	show_option(user_actions,techniques[TECH[17].responsibility_class],tech_options_position,17);
	show_option(user_actions,techniques[TECH[18].responsibility_class],tech_options_position,18);
	show_option(user_actions,techniques[TECH[19].responsibility_class],tech_options_position,19);
	show_option(user_actions,techniques[TECH[20].responsibility_class],tech_options_position,20);

	///coloring
	show_option(user_actions,techniques[TECH[21].responsibility_class],tech_options_position,21);
	if( techniques[TECH[21].responsibility_class] & TECH[21].responsibilities )
	{
		p->build_coloringbook();
		if( (techniques[TECH[21].responsibility_class] & TECH_COLORING_MEDUSA) && 
			(techniques[TECH[21].responsibility_class] & TECH_COLORING_UPDATE) )
		{
			coloring(coloring_medusa_map,p,user_actions,techniques[TECH[21].responsibility_class] & ~TECH_COLORING_UPDATE);
			coloring(coloring_map,p,user_actions,techniques[TECH[21].responsibility_class]);

			for( int i = 0; i < 81; i++ )
				coloring_map[i] &= coloring_medusa_map[i];
		}
		else
			coloring(coloring_map,p,user_actions,techniques[TECH[21].responsibility_class]);

		for( int i = 0; i < 81; i++ )
			map[i] &= coloring_map[i];
	}

	show_option(user_actions,techniques[TECH[22].responsibility_class],tech_options_position,22);
	show_option(user_actions,techniques[TECH[23].responsibility_class],tech_options_position,23);
	show_option(user_actions,techniques[TECH[24].responsibility_class],tech_options_position,24);
	show_option(user_actions,techniques[TECH[25].responsibility_class],tech_options_position,25);

	///fish
	show_option(user_actions,techniques[TECH[26].responsibility_class],tech_options_position,26);

	///normal fish
	show_option(user_actions,techniques[TECH[27].responsibility_class],tech_options_position,27);
	if( techniques[TECH[27].responsibility_class] & TECH[27].responsibilities )
	{
		simple_fish(simple_fish_map,p,user_actions,techniques[TECH[27].responsibility_class]);
		for( int i = 0; i < 81; i++ )
			map[i] &= simple_fish_map[i];
	}

	///finned fish
	show_option(user_actions,techniques[TECH[28].responsibility_class],tech_options_position,28);

	///sashimi fish
	show_option(user_actions,techniques[TECH[29].responsibility_class],tech_options_position,29);

	show_option(user_actions,techniques[TECH[30].responsibility_class],tech_options_position,30);
	show_option(user_actions,techniques[TECH[31].responsibility_class],tech_options_position,31);
	show_option(user_actions,techniques[TECH[32].responsibility_class],tech_options_position,32);
	show_option(user_actions,techniques[TECH[33].responsibility_class],tech_options_position,33);
	show_option(user_actions,techniques[TECH[34].responsibility_class],tech_options_position,34);
	show_option(user_actions,techniques[TECH[35].responsibility_class],tech_options_position,35);
	show_option(user_actions,techniques[TECH[36].responsibility_class],tech_options_position,36);
	show_option(user_actions,techniques[TECH[37].responsibility_class],tech_options_position,37);
	show_option(user_actions,techniques[TECH[38].responsibility_class],tech_options_position,38);
	show_option(user_actions,techniques[TECH[39].responsibility_class],tech_options_position,39);
	show_option(user_actions,techniques[TECH[40].responsibility_class],tech_options_position,40);
	show_option(user_actions,techniques[TECH[41].responsibility_class],tech_options_position,41);
	show_option(user_actions,techniques[TECH[42].responsibility_class],tech_options_position,42);
	show_option(user_actions,techniques[TECH[43].responsibility_class],tech_options_position,43);
	show_option(user_actions,techniques[TECH[44].responsibility_class],tech_options_position,44);
	show_option(user_actions,techniques[TECH[45].responsibility_class],tech_options_position,45);
	show_option(user_actions,techniques[TECH[46].responsibility_class],tech_options_position,46);
	show_option(user_actions,techniques[TECH[47].responsibility_class],tech_options_position,47);
	show_option(user_actions,techniques[TECH[48].responsibility_class],tech_options_position,48);
	show_option(user_actions,techniques[TECH[49].responsibility_class],tech_options_position,49);

	///forcing chains
	show_option(user_actions,techniques[TECH[50].responsibility_class],tech_options_position,50);
	if( techniques[TECH[50].responsibility_class] & TECH[50].responsibilities )
	{
		forcing_chains(forcing_chain_map,p,user_actions);
		for( int i = 0; i < 81; i++ )
			map[i] &= forcing_chain_map[i];
	}

	///xy-chains
	show_option(user_actions,techniques[TECH[51].responsibility_class],tech_options_position,51);
	if( techniques[TECH[51].responsibility_class] & TECH[51].responsibilities )
	{
		xy_chain(xy_chain_map,p,user_actions);
		for( int i = 0; i < 81; i++ )
			map[i] &= xy_chain_map[i];
	}

	///almost deadly polygons
	show_option(user_actions,techniques[TECH[52].responsibility_class],tech_options_position,52);
	if( techniques[TECH[52].responsibility_class] & TECH[52].responsibilities )
	{
		if( techniques[TECH[52].responsibility_class] & TECH_UNIQUENESS_ONE_SIDED_ADP )
		{//the one-sided part conflicts with the normal part, so i'll just call it twice.  i'm thinking of doing this with medusa and imp coloring
			almost_deadly_patterns(almost_deadly_polygon_one_sided_map,p,user_actions,techniques[TECH[52].responsibility_class] & ~TECH_UNIQUENESS_ONE_SIDED_ADP);
			almost_deadly_patterns(almost_deadly_polygon_map,p,user_actions,techniques[TECH[52].responsibility_class]);

			for( int i = 0; i < 81; i++ )
				almost_deadly_polygon_map[i] &= almost_deadly_polygon_one_sided_map[i];
		}
		else
			almost_deadly_patterns(almost_deadly_polygon_map,p,user_actions,techniques[TECH[52].responsibility_class]);

		for( int i = 0; i < 81; i++ )
			map[i] &= almost_deadly_polygon_map[i];
	}

	show_option(user_actions,techniques[TECH[53].responsibility_class],tech_options_position,53);
	show_option(user_actions,techniques[TECH[54].responsibility_class],tech_options_position,54);
	show_option(user_actions,techniques[TECH[55].responsibility_class],tech_options_position,55);

	///bilocation graph
	show_option(user_actions,techniques[TECH[56].responsibility_class],tech_options_position,56);
	if( techniques[TECH[56].responsibility_class] & TECH[56].responsibilities )
	{
		bilocation_and_bivalue(bilocation_map,p,user_actions,techniques[TECH[56].responsibility_class]&~TECH_COLORING_BIVALUE);
		for( int i = 0; i < 81; i++ )
			map[i] &= bilocation_map[i];
	}
	///bivalue graph
	show_option(user_actions,techniques[TECH[57].responsibility_class],tech_options_position,57);
	if( techniques[TECH[57].responsibility_class] & TECH[57].responsibilities )
	{
		bilocation_and_bivalue(bilocation_map,p,user_actions,techniques[TECH[57].responsibility_class]&~TECH_COLORING_BILOCATION);
		for( int i = 0; i < 81; i++ )
			map[i] &= bilocation_map[i];
	}

	///i put this here to make brute forcing less expensive when used in conjunction with other techniques
	for( int i = 0; i < 81; i++ )
		p->candidate_map[i] &= map[i];

	///brute force
	show_option(user_actions,techniques[TECH[58].responsibility_class],tech_options_position,58);
	if( techniques[TECH[58].responsibility_class] & TECH[58].responsibilities )
	{
		brute_force(brute_force_map,p);
		for( int i = 0; i < 81; i++ )
			p->candidate_map[i] &= brute_force_map[i];
	}

	///naked singles
	if( techniques[TECH[7].responsibility_class] & TECH[7].responsibilities )	p->generate_suggestion_map();
	else										p->clear_suggestion_map();


	int y_place = 32;
	//if the user is not in any of these modes, he is in solution mode
	if( !(user_actions & (USER_MAKE_MAP|DISQUALIFICATION_MODE|TECH_MODE|HIGHLIGHT_MODE|COLOR_MODE)) )
		mvprintw(y_place,75,"[X] Solution Mode");
	else
		mvprintw(y_place,75,"[ ] Solution Mode");

	if( user_actions & USER_MAKE_MAP )
		mvprintw(y_place+1,75,"[X] Map (E)dit Mode");
	else
		mvprintw(y_place+1,75,"[ ] Map (E)dit Mode");

	if( user_actions & DISQUALIFICATION_MODE )
		mvprintw(y_place+2,75,"[X] Pencil Mark Mode (x)");
	else
		mvprintw(y_place+2,75,"[ ] Pencil Mark Mode (x)");

	if( user_actions & HIGHLIGHT_MODE )
		mvprintw(y_place+3,75,"[X] Candidate (h)ighlight Mode");
	else
		mvprintw(y_place+3,75,"[ ] Candidate (h)ighlight Mode");

	if( user_actions & TECH_MODE )
		mvprintw(y_place+4,75,"[X] (T)echnique Selection Mode");
	else
		mvprintw(y_place+4,75,"[ ] (T)echnique Selection Mode");

	if( user_actions & HINT_MODE )
		mvprintw(y_place+5,75,"[X] (H)int Mode");
	else
		mvprintw(y_place+5,75,"[ ] (H)int Mode");

	if( user_actions & MEDUSA_MODE )
		mvprintw(y_place+6,75,"[X] (m)edusa Mode");
	else
		mvprintw(y_place+6,75,"[ ] (m)edusa Mode");

	if( user_actions & SINGLE_PASS_MODE )
		mvprintw(y_place+7,75,"[X] (b)reak Mode");
	else
		mvprintw(y_place+7,75,"[ ] (b)reak Mode");

	if( user_actions & COLOR_MODE )
		mvprintw(y_place+8,75,"[X] (c)olor Mode");
	else
		mvprintw(y_place+8,75,"[ ] (c)olor Mode");

	if( user_actions & SHOW_HINT_MODE )
		mvprintw(y_place+9,75,"[X] Sh(o)w Hints Mode");
	else
		mvprintw(y_place+9,75,"[ ] Sh(o)w Hints Mode");

	if( user_actions & DEBUG_MODE )
		mvprintw(y_place+10,75,"[X] Debug Mode");
	else
		mvprintw(y_place+10,75,"              ");

	if( user_actions & EXTREME_DEBUG_MODE )
		mvprintw(y_place+11,75,"[X] Extreme Debug Mode");
	else
		mvprintw(y_place+11,75,"                      ");

	delete(hidden_singles_map);
	delete(block_rc_interactions_map);
	delete(block_block_interactions_map);
	delete(naked_subsets_map);
	delete(hidden_subsets_map);
	delete(coloring_map);
	delete(coloring_medusa_map);
	delete(forcing_chain_map);
	delete(xy_chain_map);
	delete(simple_fish_map);
	delete(almost_deadly_polygon_map);
	delete(almost_deadly_polygon_one_sided_map);
	delete(bilocation_map);
	delete(brute_force_map);
}
