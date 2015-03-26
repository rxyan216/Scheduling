/* mykernel2.c: your portion of the kernel
 *
 *	Below are procedures that are called by other parts of the kernel. 
 *	Your ability to modify the kernel is via these procedures.  You may
 *	modify the bodies of these procedures any way you wish (however,
 *	you cannot change the interfaces).  
 * 
 */

#include "aux.h"
#include "sys.h"
#include "mykernel2.h"

#define TIMERINTERVAL 1 	/* in ticks (tick = 10 msec) */
#define L             10000

/*	A sample process table.  You may change this any way you wish. 
 */

static struct {
	int valid;		/* is this entry valid: 1 = yes, 0 = no */
	int pid;		/* process id (as provided by kernel) */
        int requestFlag;
        int ticks;
        int passValue;
} proctab[MAXPROCS];

static int begin;
static int end;
static int count;
static int rr;
static int remainingTicks;
static int remainingPassValue;

/*	InitSched () is called when kernel starts up.  First, set the
 *	scheduling policy (see sys.h). Make sure you follow the rules
 *	below on where and how to set it.  Next, initialize all your data
 *	structures (such as the process table).  Finally, set the timer
 *	to interrupt after a specified number of ticks.  
 */

void InitSched ()
{
	int i;

	/* First, set the scheduling policy. You should only set it
	 * from within this conditional statement.  While you are working
	 * on this assignment, GetSchedPolicy will return NOSCHEDPOLICY,
	 * and so the condition will be true and you may set the scheduling
	 * policy to whatever you choose (i.e., you may replace ARBITRARY). 
	 * After the assignment is over, during the testing phase, we will
	 * have GetSchedPolicy return the policy we wish to test, and so
	 * the condition will be false and SetSchedPolicy will not be
	 * called, thus leaving the policy to whatever we chose to test.  
	 */
	if (GetSchedPolicy () == NOSCHEDPOLICY) {	/* leave as is */
		SetSchedPolicy (PROPORTIONAL);		/* set policy here */
	}

	/* Initialize all your data structures here */
	for (i = 0; i < MAXPROCS; i++) {
		proctab[i].valid = 0;
	}

        begin = 0;
        end = 0;
        count = 0;
        rr = 0;
        remainingTicks = 1000;
        remainingPassValue = 0;

	/* Set the timer last */
	SetTimer (TIMERINTERVAL);

}


/*	StartingProc (pid) is called by the kernel when the process
 *	identified by pid is starting.  This allows you to record the
 *	arrival of a new process in the process table, and allocate
 *	any resources (if necessary).  Returns 1 if successful, 0 otherwise. 
 */

int StartingProc (pid)
	int pid;
{
        int i;

        /* For FIFO and LIFO */

        if (GetSchedPolicy () == FIFO || GetSchedPolicy () == LIFO) {
                if (count != MAXPROCS) {
                        proctab[end].valid = 1;
                        proctab[end].pid = pid;
                        end = (end + 1) % MAXPROCS;
                        count++;
                        DoSched();
                        return (1);
                }

                Printf ("Error in StartingProc: no free table entries\n");
                return (0);
        }

        /* For Arbitrary, RoundRobin and Proportional */

        for (i = 0; i < MAXPROCS; i++) {
                if (! proctab[i].valid) {
                        proctab[i].valid = 1;
                        proctab[i].pid = pid;
                        proctab[i].requestFlag = 0;
                        return (1);
                }
        }

        Printf ("Error in StartingProc: no free table entries\n");
        return (0);
}
			

/*	EndingProc (pid) is called by the kernel when the process
 *	identified by pid is ending.  This allows you to update the
 *	process table accordingly, and deallocate any resources (if
 *	necessary). Returns 1 if successful, 0 otherwise.  
 */


int EndingProc (pid)
	int pid;
{
        int i;

        /* For FIFO */

        if (GetSchedPolicy () == FIFO) {
                if (proctab[begin].valid && proctab[begin].pid == pid) {
                        proctab[begin].valid = 0;
                        begin = (begin + 1) % MAXPROCS;
                        count--;
                        return (1);
                }

                Printf ("Error in EndingProc: can't find process %d\n", pid);
                return (0);
        }

        /* For LIFO */

        if (GetSchedPolicy () == LIFO) {
                if (proctab[(end - 1 + MAXPROCS) % MAXPROCS].valid &&
                    proctab[(end - 1 + MAXPROCS) % MAXPROCS].pid == pid) {
                        proctab[(end - 1 + MAXPROCS) % MAXPROCS].valid = 0;
                        end = (end - 1 + MAXPROCS) % MAXPROCS;
                        count--;
                        return (1);
                }

                Printf ("Error in EndingProc: can't find process %d\n", pid);
                return (0);
        }

        /* For Proportional */

        if (GetSchedPolicy () == PROPORTIONAL) {
                for (i = 0; i < MAXPROCS; i++) {
                        if (proctab[i].valid && proctab[i].pid==pid) {
                                proctab[i].valid = 0;
                                if (proctab[i].requestFlag) {
                                        remainingTicks += proctab[i].ticks;
                                        for (i = 0; i < MAXPROCS; i++) {
                                                if (proctab[i].valid && proctab[i].requestFlag) {
                                                        proctab[i].passValue = 0;
                                                }
                                        }
                                        remainingPassValue = 0;
                                }
                                return (1);
                        }
                }

                Printf ("Error in EndingProc: can't find process %d\n", pid);
                return (0);
        }

        /* For Arbitrary and RoundRobin */

	for (i = 0; i < MAXPROCS; i++) {
		if (proctab[i].valid && proctab[i].pid == pid) {
			proctab[i].valid = 0;
			return (1);
		}
	}

	Printf ("Error in EndingProc: can't find process %d\n", pid);
	return (0);
}


/*	SchedProc () is called by kernel when it needs a decision for
 *	which process to run next.  It calls the kernel function
 *	GetSchedPolicy () which will return the current scheduling policy
 *	which was previously set via SetSchedPolicy (policy).  SchedProc ()
 *	should return a process id, or 0 if there are no processes to run. 
 */

int SchedProc ()
{
	int i;
        int minPassValue;
        int index;
        int requestedNum;
        int unrequestedNum;

	switch (GetSchedPolicy ()) {

	case ARBITRARY:

		for (i = 0; i < MAXPROCS; i++) {
			if (proctab[i].valid) {
				return (proctab[i].pid);
			}
		}
		break;

	case FIFO:

                if (count != 0) {
		        return (proctab[begin].pid);
                }

		break;

	case LIFO:

                if (count != 0) {
		        return (proctab[(end - 1 + MAXPROCS) % MAXPROCS].pid);
                }

		break;

	case ROUNDROBIN:

                for (i = 0; i < MAXPROCS; i++) {
                        rr = (rr + 1) % MAXPROCS;
                        if (proctab[rr].valid) {
                                return (proctab[rr].pid);
                        }
                }

		break;

	case PROPORTIONAL:

                for (requestedNum = 0, unrequestedNum = 0, i = 0; i < MAXPROCS; i++) {
                        if (proctab[i].valid) {
                                if(proctab[i].requestFlag) {
                                        requestedNum++;
                                }
                                else {
                                        unrequestedNum++;
                                }
                        }
                }

                if (remainingTicks == 0 || requestedNum != 0 && unrequestedNum == 0) {
                        for (i = 0; i < MAXPROCS; i++) {
                                if (proctab[i].valid && proctab[i].requestFlag) {
                                        minPassValue = proctab[i].passValue;
                                        index = i;
                                        break;
                                }
                        }
                        for (; i < MAXPROCS; i++) {
                                if (proctab[i].valid &&
                                    proctab[i].requestFlag &&
                                    proctab[i].passValue < minPassValue) {
                                        minPassValue = proctab[i].passValue;
                                        index = i;
                                }
                        }
                        if (minPassValue + L * 1000 / proctab[index].ticks < 0) {
                                for (i = 0; i < MAXPROCS; i++) {
                                        if (proctab[i].valid && proctab[i].requestFlag) {
                                                proctab[i].passValue -= minPassValue;
                                        }
                                }
                        }
                        proctab[index].passValue += L * 1000 / proctab[index].ticks;
                        return (proctab[index].pid);
                }

                else if (remainingTicks == 1000) {
                        for (i = 0; i < MAXPROCS; i++) {
                                rr = (rr + 1) % MAXPROCS;
                                if (proctab[rr].valid && ! proctab[rr].requestFlag) {
                                        return (proctab[rr].pid);
                                }
                        }
                }

                else {
                        minPassValue = remainingPassValue;
                        for (i = 0; i < MAXPROCS; i++) {
                                if (proctab[i].valid &&
                                    proctab[i].requestFlag &&
                                    proctab[i].passValue < minPassValue) {
                                        minPassValue = proctab[i].passValue;
                                        index = i;
                                }
                        }
                        if (minPassValue == remainingPassValue) {
                                if (minPassValue + L * 1000 / remainingTicks < 0) {
                                        for (i = 0; i < MAXPROCS; i++) {
                                                if (proctab[i].valid && proctab[i].requestFlag) {
                                                        proctab[i].passValue -= minPassValue;
                                                }
                                        }
                                        remainingPassValue -= minPassValue;
                                }
                                remainingPassValue += L * 1000 / remainingTicks;
                                for (i = 0; i < MAXPROCS; i++) {
                                        rr = (rr + 1) % MAXPROCS;
                                        if (proctab[rr].valid && ! proctab[rr].requestFlag) {
                                                return (proctab[rr].pid);
                                        }
                                }
                        }
                        else {
                                if (minPassValue + L * 1000 / proctab[index].ticks < 0) {
                                        for (i = 0; i < MAXPROCS; i++) {
                                                if (proctab[i].valid && proctab[i].requestFlag) {
                                                        proctab[i].passValue -= minPassValue;
                                                }
                                        }
                                        remainingPassValue -= minPassValue;
                                }
                                proctab[index].passValue += L * 1000 / proctab[index].ticks;
                                return (proctab[index].pid);
                        }
                }

		break;

	}
	
	return (0);
}


/*	HandleTimerIntr () is called by the kernel whenever a timer
 *	interrupt occurs.  
 */

void HandleTimerIntr ()
{
	SetTimer (TIMERINTERVAL);

	switch (GetSchedPolicy ()) {	/* is policy preemptive? */

	case ROUNDROBIN:		/* ROUNDROBIN is preemptive */
	case PROPORTIONAL:		/* PROPORTIONAL is preemptive */

		DoSched ();		/* make scheduling decision */
		break;

	default:			/* if non-preemptive, do nothing */
		break;
	}
}

/*	MyRequestCPUrate (pid, m, n) is called by the kernel whenever a process
 *	identified by pid calls RequestCPUrate (m, n). This is a request for
 *	a fraction m/n of CPU time, effectively running on a CPU that is m/n
 *	of the rate of the actual CPU speed.  m of every n quantums should
 *	be allocated to the calling process.  Both m and n must be greater
 *	than zero, and m must be less than or equal to n.  MyRequestCPUrate
 *	should return 0 if successful, i.e., if such a request can be
 *	satisfied, otherwise it should return -1, i.e., error (including if
 *	m < 1, or n < 1, or m > n). If MyRequestCPUrate fails, it should
 *	have no effect on scheduling of this or any other process, i.e., as
 *	if it were never called.  
 */

int MyRequestCPUrate (pid, m, n)
	int pid;
	int m;
	int n;
{
        int i;

        if (m < 1 || n < 1 || 1000 * m / n == 0) {
                return (-1);
        }

        for (i = 0; i < MAXPROCS; i++) {
                if (proctab[i].valid && proctab[i].pid == pid) {
                        if (proctab[i].requestFlag) {
                                if (remainingTicks + proctab[i].ticks - 1000 * m / n < 0) {
                                        return (-1);
                                }
                                else {
                                        remainingTicks += proctab[i].ticks - 1000 * m / n;
                                        proctab[i].ticks = 1000 * m / n;
                                }
                        }
                        else {
                                if (remainingTicks - 1000 * m / n < 0) {
                                        return (-1);
                                }
                                else {
                                        remainingTicks -= 1000 * m / n;
                                        proctab[i].requestFlag = 1;
                                        proctab[i].ticks = 1000 * m / n;
                                }
                        }
                        break;
                }
        }

        for (i = 0; i < MAXPROCS; i++) {
                if (proctab[i].valid && proctab[i].requestFlag) {
                        proctab[i].passValue = 0;
                }
        }

        remainingPassValue = 0;

	return (0);
}
