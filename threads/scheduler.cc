// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"
#include "stats.h"

static int
LOneCompare (Thread* x,Thread *y){
    if(x->GetBurstTime() > y->GetBurstTime()) return 1;
    else if(x->GetBurstTime() < y->GetBurstTime()) return -1;
    else{
        if(x->getID() < y->getID()) return -1;
        else return 1; 
    }
    return 0;
}

static int
LTwoCompare (Thread* x,Thread *y){
    if(x->GetPriority() > y->GetPriority()) return -1;
    else if(x->GetPriority() < y->GetPriority()) return 1;
    else{
        if(x->getID() < y->getID()) return -1;
        else return 1;
    }
    return 0; 
} 

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{ 
    //readyList = new List<Thread *>;
    L1queue = new SortedList<Thread *>(LOneCompare);
    L2queue = new SortedList<Thread *>(LTwoCompare);
    L3queue = new List<Thread *>;  
    toBeDestroyed = NULL;
    aging = false;
} 

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{ 
    //delete readyList;
    delete L1queue;
    delete L2queue;
    delete L3queue; 
} 

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
	//cout << "Putting thread on ready list: " << thread->getName() << endl ;
    
    Statistics *stats = kernel->stats;
    //put to L1 queue
	//cout<<"ThreadPriority:"<<thread->GetPriority()<<"\n";
	// cout << "Thread " << thread->getID() <<" Ready To Run\n";

    if(thread->GetPriority() >= 100 && thread->GetPriority() <= 150){
        if (!kernel->scheduler->L1queue->IsInList(thread))
        {
            cout<<"Tick["<<stats->totalTicks<<"]: Thread["<<thread->getID()<<"] is inserted into queue L[1]\n";
            
            L1queue->Insert(thread);
            if (kernel->currentThread->getID() > 1 && !L1queue->IsInList(kernel->currentThread))
            {
                kernel->interrupt->yieldOnReturn = true;
            }
        }
    }//put to L2 queue
    else if(thread->GetPriority() >= 50 && thread->GetPriority() <= 99){
        cout<<"Tick["<<stats->totalTicks<<"]: Thread["<<thread->getID()<<"] is inserted into queue L[2]\n";
        L2queue->Insert(thread);
    }//put to L3 queue
    else{
        cout<<"Tick["<<stats->totalTicks<<"]: Thread["<<thread->getID()<<"] is inserted into queue L[3]\n";
        L3queue->Append(thread);
    }
    thread->setStatus(READY);
    thread->SetWaitTime(0);
    //readyList->Append(thread);
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
    ASSERT(kernel->interrupt->getLevel() == IntOff);
    Statistics *stats = kernel->stats;

    if(!L1queue->IsEmpty()){
        cout<<"Tick["<<stats->totalTicks<<"]: Thread["<<L1queue->Front()->getID()<<"] is removed from queue L[1]\n";
        return L1queue->RemoveFront();
    }
    else if(!L2queue->IsEmpty()){
        cout<<"Tick["<<stats->totalTicks<<"]: Thread["<<L2queue->Front()->getID()<<"] is removed from queue L[2]\n";
        return L2queue->RemoveFront();
    }
    else if(!L3queue->IsEmpty()){
        cout<<"Tick["<<stats->totalTicks<<"]: Thread["<<L3queue->Front()->getID()<<"] is removed from queue L[3]\n";
        return L3queue->RemoveFront();
    }
    else
    {
        return NULL;
    }

    /*if (readyList->IsEmpty()) {
		return NULL;
    } else {
    	return readyList->RemoveFront();
    }*/
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    // cout << "In scheduler::Run\n";
    Thread *oldThread = kernel->currentThread;
    
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	 toBeDestroyed = oldThread;
    }
    
    if (oldThread->space != NULL) {	// if this thread is a user program,
        oldThread->SaveUserState(); 	// save the user's CPU registers
	oldThread->space->SaveState();
    }
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << oldThread->getName());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	oldThread->space->RestoreState();
    }
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        delete toBeDestroyed;
	toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyList->Apply(ThreadPrint);
}

void
Scheduler::IncreaseWaitTime()
{
    
    ListIterator<Thread *> *iter1 =  new ListIterator<Thread *>(L1queue);
    ListIterator<Thread *> *iter2 =  new ListIterator<Thread *>(L2queue);
    ListIterator<Thread *> *iter3 =  new ListIterator<Thread *>(L3queue);
    Statistics *stats = kernel->stats;
    int oldpriority;
	//cout<<"In IncreaseWaitTime\n";
    //L1
    for(;!iter1->IsDone();iter1->Next()){
        iter1->Item()->SetWaitTime(iter1->Item()->GetWaitTime()+1);
        if(iter1->Item()->GetWaitTime() >= PeriodToAging){
            aging = true;
            oldpriority = iter1->Item()->GetPriority();
			// cout<<"***********************************"<<oldpriority<<"******************************"<<"L1"<<"\n";
            iter1->Item()->SetPriority(oldpriority+Aging);
            cout<<"Tick["<<stats->totalTicks<<"]: Thread["<<iter1->Item()->getID()<<"] changes its priority from ["<<
            oldpriority<<"] to ["<<iter1->Item()->GetPriority()<<"]\n";
            iter1->Item()->SetWaitTime(0);
        }
    }
    //L2
    for(;!iter2->IsDone();iter2->Next()){
        iter2->Item()->SetWaitTime(iter2->Item()->GetWaitTime()+1);
        if(iter2->Item()->GetWaitTime() >= PeriodToAging){
            aging = true;
            oldpriority = iter2->Item()->GetPriority();
			// cout<<"***********************************"<<oldpriority<<"******************************"<<"L2"<<"\n";
            iter2->Item()->SetPriority(oldpriority+Aging);
            cout<<"Tick["<<stats->totalTicks<<"]: Thread["<<iter2->Item()->getID()<<"] changes its priority from ["<<
            oldpriority<<"] to ["<<iter2->Item()->GetPriority()<<"]\n";
            L2queue->Remove(iter2->Item());
            cout<<"Tick["<<stats->totalTicks<<"]: Thread["<<iter2->Item()->getID()<<"] is removed from queue L[2]\n";
            ReadyToRun(iter2->Item());
        }
    }    
    //L3
    for(;!iter3->IsDone();iter3->Next()){
        iter3->Item()->SetWaitTime(iter3->Item()->GetWaitTime()+1);
        if(iter3->Item()->GetWaitTime() >= PeriodToAging && iter3->Item()->getID() > 1){
            aging = true;
            oldpriority = iter3->Item()->GetPriority();
			// cout<<"***********************************"<<oldpriority<<"******************************"<<"L3"<<"\n";
            iter3->Item()->SetPriority(oldpriority+Aging);
            cout<<"Tick["<<stats->totalTicks<<"]: Thread["<<iter3->Item()->getID()<<"] changes its priority from ["<<
            oldpriority<<"] to ["<<iter3->Item()->GetPriority()<<"]\n";
            L3queue->Remove(iter3->Item());
            cout<<"Tick["<<stats->totalTicks<<"]: Thread["<<iter3->Item()->getID()<<"] is removed from queue L[3]\n";
            ReadyToRun(iter3->Item());
        }
    }
}