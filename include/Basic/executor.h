#ifndef executor_H
#define executor_H

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		11-7-1996
 RCS:		$Id: executor.h,v 1.21 2007-10-30 16:53:35 cvskris Exp $
________________________________________________________________________

-*/

#include "task.h"
#include "namedobj.h"
#include <iosfwd>

template <class T> class ObjectSet;

/*!\brief specification to enable chunkwise execution a process.

Interface enabling separation of the control of execution of any process from
what actually is going on. The work is done by calling the doStep() method
until either ErrorOccurred or Finished is returned. To enable logging and/or
communication with the user, two types of info can be made available (the
methods will be called before the step is executed). Firstly, a message.
Secondly, info on the progress.
It is common that Executors are combined to a new Executor object. This is
the most common reason why totalNr() can change.

If doStep returns -1 (Failure) the error message should be in message().

The execute() utility executes the process while logging message() etc. to
a stream. Useful in batch situations.

*/

class Executor : public SequentialTask, public NamedObject
{
public:
			Executor( const char* nm )
			: NamedObject(nm)
			, prestep(this), poststep(this)	{}
    virtual		~Executor()			{}

    virtual int		doStep();

    static const int	ErrorOccurred;		//!< -1
    static const int	Finished;		//!< 0
    static const int	MoreToDo;		//!< 1
    static const int	WarningAvailable;	//!< 2

    virtual bool	execute(std::ostream* log=0,bool isfirst=true,
	    			bool islast=true,int delaybetwnstepsinms=0);

    Notifier<Executor>	prestep;
    Notifier<Executor>	poststep; //!< Only when MoreToDo will be returned.

};


/*!\brief Executor consisting of other executors.

Executors may be added on the fly while processing. Depending on the
parallel flag, the executors are executed in the order in which they were added
or in parallel (but still single-threaded).

*/


class ExecutorGroup : public Executor
{
public:
    			ExecutorGroup( const char* nm, bool parallel=false );
    virtual		~ExecutorGroup();
    virtual void	add( Executor* );
    			/*!< You will become mine!! */

    virtual const char*	message() const;
    virtual int		totalNr() const;
    virtual int		nrDone() const;
    virtual const char*	nrDoneText() const;
    
    int			nrExecutors() { return executors_.size(); }
    Executor*		getExecutor(int idx) { return executors_[idx]; }

    void		setNrDoneText(const char* txt) { nrdonetext_ = txt; }
    			//!< If set, will use this and the counted nrdone

protected:

    virtual int		nextStep();
    virtual bool	goToNextExecutor();
    void		findNextSumStop();

    int			sumstart_;
    int			sumstop_;
    const bool		parallel_;
    int			currentexec_;
    BufferString	nrdonetext_;
    ObjectSet<Executor>& executors_;
    TypeSet<int>	executorres_;

};

#endif
