/**
 * @file move.h
 * @author Adrian Del Maestro
 * @date 10.21.2008
 *
 * @brief Move class definitions.
 */

#include "common.h"

#ifndef MOVE_H 
#define MOVE_H

/* #define DEBUG_WORM */
/* #define DEBUG_MOVE */

class Path;
class ActionBase;

// ========================================================================  
// MoveBase Class
// ========================================================================  
/** 
 * The base class that all moves will be derived from.
 * 
 * There will be a bunch of different types of moves, those which
 * move individual particles, the center of mass of an entire worldine
 * loop etc.  They will all share the basic functionality defined here.
 */
class MoveBase {

    public:
        MoveBase (Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=ANY, bool _varLength=false);
        virtual ~MoveBase();
    
        ensemble operateOnConfig;       ///< What configurations do we operate on?
        bool variableLength;            ///< Does the move have a variable length?
        std::string name1;

        /** return the move name */
        virtual std::string getName() {return "base";}

        /** Get the acceptance ratio. */
        double getAcceptanceRatio() {
            return (numAttempted == 0 ? 0.0 : 1.0*numAccepted/(1.0*numAttempted));
        }

        /** Get the total acceptance ratio. */
        double getTotAcceptanceRatio() {
            return (totAttempted == 0 ? 0.0 : 1.0*totAccepted/(1.0*totAttempted));
        }

        /** Get the acceptance ratio by level. */
        double getAcceptanceRatioLevel(int n) { 
            return (numAttemptedLevel(n) == 0 ? 0.0 : 
                    1.0*numAcceptedLevel(n)/(1.0*numAttemptedLevel(n)));
        }

        /** Get the number of moves attempted */
        int getNumAttempted() { return numAttempted; }
        /** Get the number of moves accepted */
        int getNumAccepted() { return numAccepted; }
        /** Get the number of moves attempted by level */
        int getNumAttemptedLevel(int n) { return numAttemptedLevel(n); }
        /** Get the number of moves accepted by level */
        int getNumAcceptedLevel(int n) { return numAcceptedLevel(n); }

        /** Attempt the move (will be overloaded). */
        virtual bool attemptMove() = 0; 

        /** Reset the total accepted counter */
        void resetTotAccept() { totAccepted = totAttempted = 0; }
        /** Reset the number accepted counter */
        void resetAccept() { numAccepted = numAttempted = 0; }

    protected:
        friend class PathIntegralMonteCarlo;    // Friends for I/O

        Path &path;                     ///< A reference to the paths
        ActionBase *actionPtr;          ///< A base pointer to the action
        MTRand &random;                 ///< A reference to the RNG

        bool success;                   ///< Did we sucessfully perform a move?

        uint32 numAccepted;             ///< The number of accepted moves
        uint32 numAttempted;            ///< The number of attempted moves
        int numToMove;                  ///< The number of particles moved
        int numLevels;                  // The 2^numLevels = num slices moved

        static uint32 totAccepted;      ///< The total number of  moves accepted
        static uint32 totAttempted;     ///< The total number of  moves attempted

	DynamicArray <uint32,1> numAcceptedLevel;  ///< The number of moves accepted at each level
	DynamicArray <uint32,1> numAttemptedLevel; ///< The number of moves attempted at each level

	DynamicArray <dVec,1> originalPos;     ///< The original particle positions
	DynamicArray <dVec,1> newPos;          ///< New particle positions
        
        std::vector <iVec> winding;          ///< The winding std::vectors         
        std::vector <double> cumrho0;        ///< Used for tower-sampling winding sectors

        int maxWind;                    ///< The largest winding number
        int numWind;                    ///< The total number of winding std::vectors

        double oldAction;               ///< The original potential action
        double newAction;               ///< The new potential action
        double deltaAction;             ///< The action difference

        double sqrt2LambdaTau;          ///< sqrt(2 * Lambda * tau)
        double sqrtLambdaTau;           ///< sqrt(Lambda * tau)

        beadLocator nBeadIndex;         ///< Neighbor bead index
        dVec neighborPos;               ///< Staging neighbor position
        dVec newRanPos;                 ///< Staing random position

        /** Keep the move */
        virtual void keepMove();        
        /** undo the move */
        virtual void undoMove() = 0;    

        /* Returns a new bead position based on the staging algorithm */
        dVec newStagingPosition(const beadLocator &, const beadLocator &, const int, const int);
        dVec newStagingPosition(const beadLocator &, const beadLocator &, const int, const int, iVec &);

        /** Obtain a winding sector for a stage-like move */
        iVec sampleWindingSector(const beadLocator &, const beadLocator &, const int, double &);

        /** Find the winding number for a path between two beads */
        iVec getWindingNumber(const beadLocator &, const beadLocator &);

        /* Return a new bead position which samples the free particle density matrix */
        dVec newFreeParticlePosition(const beadLocator &);

        /* Returns a new bead position based on the bisection algorithm */
        dVec newBisectionPosition(const beadLocator&, const int);   

        double newK,oldK;               ///< The old and new kinetic action
        double newV,oldV;               ///< The old and new potential action

        /* Debugging methods */
        void printMoveState(std::string);
        void checkMove(int,double);
};

// ========================================================================  
// Displace Move Class 
// ========================================================================  
/**
 * A derived class which performs a simple single slice displacement move.
 */
class DisplaceMove: public MoveBase {

    public:
        DisplaceMove(Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=ANY);
        ~DisplaceMove();
    
        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        beadLocator beadIndex;  // The index of the bead being moved
        void undoMove();        // revert everything back
};


// ========================================================================
// End Staging Move Class
// ========================================================================
/* A derived class which performs a staiging move on the end of a path.
 */
class EndStagingMove: public MoveBase {
    
    public:
        EndStagingMove(Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=ANY);
        ~EndStagingMove();

        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        bool leftMoving;        // True if update moves left to right
        beadLocator leftBead;   // The left most bead being moved
        beadLocator rightBead;  // The left most bead being moved
        void undoMove();        // revert everything back
};


// ========================================================================
// End Staging Move Class
// ========================================================================
/* A derived class which performs a staiging move on the end of a path.
 */
class MidStagingMove: public MoveBase {
    
    public:
        MidStagingMove(Path &, ActionBase *, MTRand &,
                ensemble _operateOnConfig=ANY);
        ~MidStagingMove();

        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}
    
    private:
        beadLocator leftBead;   // The left most bead being moved
        beadLocator rightBead;  // The left most bead being moved
        beadLocator midBeadL;   // Bead on left of break to be updated
        beadLocator midBeadR;   // Bead on right of break to be updated
        void undoMove();        // revert everything back
};


// ========================================================================
// SwapBreak Move Class
// ========================================================================
/* A derived class which swaps the location of a broken worldline.
*/
class SwapBreakMove: public MoveBase {
    
    public:
        SwapBreakMove(Path &, ActionBase *, MTRand &,
                ensemble _operateOnConfig=ANY);
        ~SwapBreakMove();

        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        void undoMove(){};
    
};

// ========================================================================  
// CenterOfMass Move Class 
// ========================================================================  
/** 
 * A derived class which performs a simple displacement of the center of
 * mass of the entire wordline for a particle.
 */
class CenterOfMassMove: public MoveBase {

    public:
        CenterOfMassMove(Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=ANY);
        ~CenterOfMassMove();

        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        beadLocator startBead,endBead;  // The start and end beads
        void undoMove();                // revert everything back
};


// ========================================================================  
// Staging Move Class 
// ========================================================================  
/** 
 * A derived class which performs a staging move, which exactly samples
 * the kinetic action.
 */
class StagingMove: public MoveBase {

    public:
        StagingMove(Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=ANY);
        ~StagingMove();

        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        beadLocator startBead,endBead;      // The start and end of the stage

        void undoMove();                    // Undo the move

        int stageLength;                    // The length of the stage
};

// ========================================================================  
// Bisection Move Class 
// ========================================================================  
/** A derived class which performs a bisection move, which exactly samples
 * the kinetic action.
 */
class BisectionMove: public MoveBase {

    public:
        BisectionMove(Path &, ActionBase *, MTRand &,
                ensemble _operateOnConfig=ANY);
        ~BisectionMove();
    
        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
	DynamicArray <bool,1> include;             // Which beads have been included?

        beadLocator startBead,endBead;

        int numActiveBeads;                 // The total number of active beads
        int level;                          // The current level
        int shift;                          // The distance between slices at a given level

        double oldDeltaAction;              // The old and new action differences

        void keepMove();                    // Keep the move
        void undoMove();                    // Undo the move
};

// ========================================================================  
// Open Move Class 
// ========================================================================  
/** 
 * A derived class which performs an open move, creating a worm with a well
 * defined head and tail.
 */
class OpenMove: public MoveBase {

    public:
        OpenMove(Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=DIAGONAL, bool _varLength=true);
        ~OpenMove();

        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        beadLocator headBead, tailBead; // The temporary head and tail locatores
        int gapLength;                  // The proposed WL length to remove 
        int numLevels;                  // The 2^numLevels = num slices moved

        void undoMove();                // Undo a move
        void keepMove();                // keep the move

};

// ========================================================================  
// Canonical Open Move Class 
// ========================================================================  
/** 
 * A derived class which performs a canonical open move, creating a worm with a well
 * defined head and tail on the same time slice
 */
class CanonicalOpenMove: public MoveBase {

    public:
        CanonicalOpenMove(Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=DIAGONAL, bool _varLength=true);
        ~CanonicalOpenMove();

        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        beadLocator headBead, tailBead; // The temporary head and tail locatores
        int gapLength;                  // The proposed WL length to remove 
        int numLevels;                  // The 2^numLevels = num slices moved

        void undoMove();                // Undo a move
        void keepMove();                // keep the move

};

// ========================================================================  
// Close Move Class 
// ========================================================================  
/** 
 * A derived class which performs a close move, creating a diagonal world
 * line configuration.
 */
class CloseMove: public MoveBase {

    public:
        CloseMove(Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=OFFDIAGONAL, bool _varLength=true);
        ~CloseMove();

        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        beadLocator headBead,tailBead;  // The temporary head and tail slices
        int numLevels;                      // The 2^numLevels = num slices moved

	DynamicArray <int,1> oldBeadOn;        // The old and new bead states

        void undoMove();                // Undo a move
        void keepMove();                // keep the move
};

// ========================================================================  
// Canonical Close Move Class 
// ========================================================================  
/** 
 * A derived class which performs a canonical close move, creating a diagonal world
 * line configuration.
 */
class CanonicalCloseMove: public MoveBase {

    public:
        CanonicalCloseMove(Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=OFFDIAGONAL, bool _varLength=true);
        ~CanonicalCloseMove();

        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        beadLocator headBead,tailBead;  // The temporary head and tail slices
        int numLevels;                      // The 2^numLevels = num slices moved

	DynamicArray <int,1> oldBeadOn;        // The old and new bead states

        void undoMove();                // Undo a move
        void keepMove();                // keep the move
};
// ========================================================================  
// Insert Move Class 
// ========================================================================  
/** 
 * A derived class which performs an insert move, creating an off-diagonal 
 * world line configuration with a single worm.
 */
class InsertMove: public MoveBase {

    public:
        InsertMove(Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=DIAGONAL, bool _varLength=true);
        ~InsertMove();
    
        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        beadLocator headBead,tailBead;  // The temporary head and tail beads

        int wormLength;                 // The proposed WL length to close
        int numLevels;                  // The 2^numLevels = num slices moved

        void undoMove();                // Undo a move
        void keepMove();                // keep the move
};

// ========================================================================  
// Remove Move Class 
// ========================================================================  
/** 
 * A derived class which performs a remove move, creating a diagonal 
 * world line configuration by destroying a single worm.
 */
class RemoveMove: public MoveBase {

    public:
        RemoveMove(Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=OFFDIAGONAL, bool _varLength=true);
        ~RemoveMove();

        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        int numLevels;                  // The 2^numLevels = num slices moved

        void undoMove();                // Undo a move
        void keepMove();                // keep the move
};

// ========================================================================  
// Advance Head Move Class 
// ========================================================================  
/** 
 * A derived class which performs an advance head move, causing the head of 
 * a worm in a off-diagonal configuration to advance in imaginary time.
 */
class AdvanceHeadMove: public MoveBase {

    public:
        AdvanceHeadMove(Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=OFFDIAGONAL, bool _varLength=true);
        ~AdvanceHeadMove();
        
        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        beadLocator headBead;           // The temporary new head

        int advanceLength;              // The proposed WL length to advance
        int numLevels;                  // The 2^numLevels = num slices moved

        void undoMove();                // Undo a move
        void keepMove();                // keep the move

        //beadLocator startBead;          
	DynamicArray <dVec,1> newPos;                  // The modified particle positions
	DynamicArray <unsigned int,1> oldBeadOn;       // The old and new bead states

};

// ========================================================================  
// Advance Tail Move Class 
// ========================================================================  
/** 
 * A derived class which performs an advance tail move, causing the tail of 
 * a worm in a off-diagonal configuration to advance in imaginary time, resulting
 * in a shorter worm.
 */
class AdvanceTailMove: public MoveBase {

    public:
        AdvanceTailMove(Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=OFFDIAGONAL, bool _varLength=true);
        ~AdvanceTailMove();
    
        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        beadLocator tailBead;           // The temporary new tail

        int advanceLength;              // The proposed WL length to advance
        int numLevels;                  // The 2^numLevels = num slices moved

        void undoMove();                // Undo a move
        void keepMove();                // keep the move
};

// ========================================================================  
// Recede Head Move Class 
// ========================================================================  
/** 
 * A derived class which performs a recede move on the head, causing a worm 
 * head to propagate backwards in imaginary time by removing beads and links.
 */
class RecedeHeadMove: public MoveBase {

    public:
        RecedeHeadMove(Path &, ActionBase *, MTRand &,
                ensemble _operateOnConfig=OFFDIAGONAL, bool _varLength=true);
        ~RecedeHeadMove();
    
        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        beadLocator headBead;           // The proposed new head position
        int recedeLength;               // The number of slices to recede by
        int numLevels;                  // The 2^numLevels = num slices moved

        void undoMove();                // Undo a move
        void keepMove();                // keep the move
};

// ========================================================================  
// Recede Tail Move Class 
// ========================================================================  
/** 
 * A derived class which performs a recede move on the tail, causing a worm 
 * tail to propagate backwards in imaginary time by adding beads and links.
 */
class RecedeTailMove: public MoveBase {

    public:
        RecedeTailMove(Path &, ActionBase *, MTRand &,
                ensemble _operateOnConfig=OFFDIAGONAL, bool _varLength=true);
        ~RecedeTailMove();
        
        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:
        beadLocator tailBead;           // The proposed new head position
        int recedeLength;               // The number of slices to recede by
        int numLevels;                  // The 2^numLevels = num slices moved

        void undoMove();                // Undo a move
        void keepMove();                // keep the move
};

// ========================================================================  
// Swap Move Base Class 
// ========================================================================  
/** 
 * A derived class which forms the base of a swap head and swap tail move
 * class.
 */
class SwapMoveBase: public MoveBase {

    public:
        SwapMoveBase(Path &, ActionBase *, MTRand &, 
                ensemble _operateOnConfig=OFFDIAGONAL);
        ~SwapMoveBase();

    protected:
        int swapLength;                     ///< The length of worldLine to be moved
        int numLevels;                      ///< The number of bisection levels
        unsigned int sizeCDF;               ///< The size of the cumulative distribution function

        std::vector <double> cumulant;           ///< The cumulant array used in selecting a pivot

        beadLocator pivot;                  ///< The pivot bead
        beadLocator swap;                   ///< The swap bead

        double SigmaSwap;                   ///< Probability normalization factor

        /* Returns the normalization factor for the probability dist. */
        double getNorm(const beadLocator&, const int sign=1);
        
        /* Gets the bead where the swap will pivot. */
        beadLocator selectPivotBead();

        /* An overloaded version which also gets the winding sector */
        beadLocator selectPivotBead(iVec&);
};

// ========================================================================  
// Swap Head Move Class 
// ========================================================================  
/** 
 * A derived class which performs a swap head move, which mixes up worldlines
 * by reconnecting the worm head and is essential for systems with permutation 
 * symmetry (such as bosons).
 */
class SwapHeadMove: public SwapMoveBase {

    public:
        SwapHeadMove(Path &, ActionBase *, MTRand &,
                ensemble _operateOnConfig=OFFDIAGONAL);
        ~SwapHeadMove();

        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:

        double SigmaHead;       // The probability normalization factor
        beadLocator nextSwap;   // Used for re-linking

        void undoMove();        // Undo a move
        void keepMove();        // keep the move
};

// ========================================================================  
// Swap Tail Move Class 
// ========================================================================  
/** 
 * A derived class which performs a swap tail move, which mixes up worldlines
 * by reconnecting the worm tail and is essential for systems with permutation 
 * symmetry (such as our bosons).
 */
class SwapTailMove: public SwapMoveBase {

    public:
        SwapTailMove(Path &, ActionBase *, MTRand &,
                ensemble _operateOnConfig=OFFDIAGONAL);
        ~SwapTailMove();
        
        bool attemptMove();
        static const std::string name;
        std::string getName() {return name;}

    private:

        double SigmaTail;               // The probability normalization factor
        beadLocator prevSwap;           // Used for re-linking

        void undoMove();                // Undo a move
        void keepMove();                // keep the move
};

#endif
