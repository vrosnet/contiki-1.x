/**
 * \file
 * Event kernel, signal dispatcher and handler of uIP events.
 * \author Adam Dunkels <adam@dunkels.com> 
 *
 * The Dispatcher module is the event kernel in Contiki and handles
 * processes, signals and uIP events. All process execution is
 * initiated by the Dispatcher.
 *
 *
 *
 * The Dispatcher is the initiator of all program execution in
 * Contiki. After the system has been initialized by the boot up code,
 * the dispatcher_run() function is called. This function never
 * returns, but will sit in a loop in which it does two things.
 * 
 * - Pulls the first signal of the signal queue and dispatches this to
 *   all listening processes.
 *
 * - Executes the "idle" handlers of all processes that have
 *   registered.
 *
 * Only one signal is processes at a time, and the idle handlers of
 * all processes are called between two signals are handled.
 *
 *
 *
 * A process is defined by a signal handler, a uIP event handler, and
 * an idle handler. The signal handler is called when a signal has
 * been emitted, for which the process is currently listening. The uIP
 * event handler is called when the uIP TCP/IP stack has an event to
 * deliver to the process. Such events can be that new data has
 * arrived on a connection, that previously sent data has been
 * acknowledged or that a connection has been closed. The idle handler
 * is periodically called by the system.
 *
 * \note The name "idle handler" is a misnomer, since the idle handler
 * will be called even though the system is not idle.
 *
 * A process is started by calling the dispatcher_start()
 * function. This function must be called before any other Dispatcher
 * function is called. When the function returns, the new process is
 * running. The function dispatcher_exit() is used to tell the
 * Dispatcher that a process has exited. This function must be called
 * by the process itself, and must be called the process unloads
 * itself.
 *
 * \note It is not possible to call dispatcher_exit() on behalf of
 * another process - instead, emit the signal dispatcher_signal_quit
 * with the process as a receiver. The other process should then
 * listen for this signal, and call dispatcher_exit() when the signal
 * is received. 
 *
 *
 *
 * The Dispatcher can pass signals between different
 * processes. Signals are simple messages that consist of a signal
 * number and a generic data pointer called the signal data. The
 * signal data can be used to pass messages between processes. In
 * order for a signal to be delivered to a process, the process must
 * be listening for the signal number.
 *
 * When a process is running, the function dispatcher_listen() can be
 * called to register the process as a listener for a signal. When a
 * signal for which the process has registered itself as a listener is
 * emitted by another process, the process' signal handler will be
 * invoked. The signal number and the signal data are passed as
 * function parameters to the signal handler. The signal handler must
 * check the signal number and do whatever it should do based on the
 * value of the signal number.
 *
 * Every process listens to the dispatcher_signal_quit signal by
 * default, and the signal handler function must check for this
 * signal. If this signal is received, the process must do any
 * necessary clean-ups (i.e., close open windows, deallocate allocated
 * memory, etc.) call process_exit(), and call the LOADER_UNLOAD()
 * function.
 *
 * \note It is not possible to unregister a listening signal.
 *
 *
 *
 * If a process has registered an idle handler, the Dispatcher will
 * call it as often as possible. The idle handler can be used to
 * implement timer based functionality (by checking the ek_clock()
 * function), or other background processing. The idle handler must
 * return to the caller within a short time, or otherwise the system
 * will feel sluggish.
 *
 *
 *
 * The uIP TCP/IP stack will call the Dispatcher when a uIP event has
 * occured. The Dispatcher will find the right process for which the
 * event is intended and call the process' uIP handler function. 
 */

/*
 * Copyright (c) 2002-2003, Adam Dunkels.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution. 
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the "ek" event kernel.
 *
 * $Id: dispatcher.c,v 1.19 2003/09/07 18:13:50 adamdunkels Exp $
 *
 */

#include "ek.h"
#include "dispatcher.h"

#include "uip.h"

#include "uip-signal.h"

/**
 * \internal Holds the currently running process' ID
 *
 */
ek_id_t dispatcher_current;

/**
 * \internal Pointer to the currently running process structure.
 *
 */
struct dispatcher_proc *dispatcher_procs;

static struct dispatcher_proc *curproc;
static ek_id_t ids = 1;

/**
 * The "quit" signal.
 *
 * All processes listens to this signal by default, but each program
 * must implement the signal handler for the signal by itself.
 */
ek_signal_t dispatcher_signal_quit;

static ek_signal_t lastsig = 1;

/**
 * \internal Structure for holding a TCP port and a process ID.
 */
struct listenport {
  u16_t port;
  ek_id_t id;
};
static struct listenport listenports[UIP_LISTENPORTS];

#if CC_FUNCTION_POINTER_ARGS
#else /* CC_FUNCTION_POINTER_ARGS */
ek_signal_t dispatcher_sighandler_s;
ek_data_t dispatcher_sighandler_data;

void *dispatcher_uipcall_state;

#endif /* CC_FUNCTION_POINTER_ARGS */      


/**
 * \internal Structure used for keeping the queue of active signals.
 */
struct signal_data {
  ek_signal_t s;
  ek_data_t data;
  ek_id_t id;
};

static ek_num_signals_t nsignals, fsignal;
static struct signal_data signals[EK_CONF_NUMSIGNALS];

/*-----------------------------------------------------------------------------------*/
/**
 * Allocates a signal number.
 *
 * \return The allocated signal number or EK_SIGNAL_NONE if no signal
 * number could be allocated.
 */
/*-----------------------------------------------------------------------------------*/
ek_signal_t
dispatcher_sigalloc(void)
{
  return lastsig++;
}
/*-----------------------------------------------------------------------------------*/
/**
 * Starts a new process.
 *
 * Is called by a program in order to start a new process for the
 * program. It must be called before any other dispatcher or CTK
 * functions.
 *
 * \param p A pointer to a dispatcher_proc struct that must be found
 * in the process own memory space.
 *
 * \return The process identifier for the new process or EK_ID_NONE
 * if the process could not be started.
 */
/*-----------------------------------------------------------------------------------*/
ek_id_t
dispatcher_start(CC_REGISTER_ARG struct dispatcher_proc *p)
{
  ek_id_t id;
  struct dispatcher_proc *q;
  
 again:

  do {
    id = ids++;
  } while(id == EK_ID_NONE);
  
  /* Check if this ID is use. */
  for(q = dispatcher_procs; q != NULL; q = q->next) {
    if(id == q->id) {      
      if(id == EK_ID_NONE) {
	/* We have tried all the available process IDs, and did not
	   find any free ones. So we return with an error. */
	return EK_ID_NONE;
      } 
      goto again;
    }
  }

  /* Put first on the procs list.*/
  p->next = dispatcher_procs;
  dispatcher_procs = p;

  
  p->id = id;

  /* Make sure we know which processes we are running at the
     moment. */
  dispatcher_current = id;
  curproc = p;
  
  /* All processes must listen to the dispatcher_signal_quit
     signal. */
  dispatcher_listen(dispatcher_signal_quit);
  return id;
}
/*-----------------------------------------------------------------------------------*/
/**
 * Causes the process to exit.
 *
 * Must be called by the process itself before it unloads itself, or
 * the system will crash.
 *
 * \param p A pointer to the process' dispatcher_proc struct that was
 * started with dispatcher_start().
 */
/*-----------------------------------------------------------------------------------*/
void
dispatcher_exit(CC_REGISTER_ARG struct dispatcher_proc *p)
{
  struct dispatcher_proc *q;
  static unsigned char i;
  struct listenport *l;

#ifdef WITH_UIP
  /* If this process has any listening TCP ports, we remove them. */
  l = listenports;
  for(i = 0; i < UIP_LISTENPORTS; ++i) {
    if(l->id == p->id) {
      uip_unlisten(l->port);
      l->port = 0;
      l->id = EK_ID_NONE;
    }
    ++l;
  }
#endif /* WITH_UIP */
  
  /* Remove process from the process list. */
  if(p == dispatcher_procs) {
    dispatcher_procs = dispatcher_procs->next;    
  } else {
    for(q = dispatcher_procs; q != NULL; q = q->next) {
      if(q->next == p) {
	q->next = p->next;
	break;
      }
    }
  }

  dispatcher_current = EK_ID_NONE;
  curproc = NULL;
}
/*-----------------------------------------------------------------------------------*/
/**
 * The real-time clock.
 *
 * This function may be used by programs for timing.
 *
 * \return The current wall-clock time in an archicture specific time
 * unit.
 */
/*-----------------------------------------------------------------------------------*/
ek_clock_t
ek_clock(void)
{
  return clock();
}
#ifdef WITH_UIP
/*-----------------------------------------------------------------------------------*/
/**
 * Starts to connect to a remote host using TCP.
 *
 * This function should be called to connect to a remote host using
 * the reliable TCP protocol. It is a wrapper to the uIP function
 * uip_connect(), with the difference the dispatcher_connect() sends a
 * uip_signal_poll signal to the TCP/IP driver which makes the
 * connection request to go out immediately instead of being delayed
 * for up to 0.5 seconds.
 *
 * This function also registers a pointer to the allocated
 * connection. This pointer will be passed as an argument to the
 * process' uIP handler function for every uIP event.
 *
 * \note The port parameter must be given in network byte order, which
 * requires either the HTONS() or the htons() functions to be used for
 * converting from host byte order to network byte order as in the
 * example below:
 \code
   u16_t ipaddr[2];
   struct uip_conn *conn;

   uip_ipaddr(ipaddr, 192,168,2,5);
   conn = dispatcher_connect(ipaddr, HTONS(80), NULL);
   if(conn == NULL) {
      error("Could not allocate connection.");
   }
 \endcode
 *
 * \param ripaddr A pointer to a packed repressentation of the IP
 * address of the host to which to connect.
 *
 * \param port The TCP port number on the remote host in network byte
 * order.
 *
 * \param appstate The generic pointer that is to be associated with
 * the uIP connection.
 *
 * \return The connection identifier, or NULL if no connection
 * identifier could be allocated.
 */
/*-----------------------------------------------------------------------------------*/
struct uip_conn *
dispatcher_connect(u16_t *ripaddr, u16_t port, void *appstate)
{
  struct uip_conn *c;

  c = uip_connect(ripaddr, port);
  if(c == NULL) {
    return NULL;
  }

  dispatcher_emit(uip_signal_poll, c, DISPATCHER_BROADCAST);

  dispatcher_markconn(c, appstate);
  
  return c;
}
/*-----------------------------------------------------------------------------------*/
/**
 * \internal The uIP callback function.
 *
 * This function is the uIP callback function and is called by the uIP
 * code whenever a uIP event occurs. This funcion will find out to
 * which process the event should be handed over to, and calls the
 * appropriate process' uIP callback function if such a function has
 * been registered.
 *
 */
/*-----------------------------------------------------------------------------------*/
void
dispatcher_uipcall(void)     
{
  register struct dispatcher_proc *p;
  register struct dispatcher_uipstate *s;
  static u8_t i;
  struct listenport *l;


  s = (struct dispatcher_uipstate *)uip_conn->appstate;

  /* If this is a connection request for a listening port, we must
     mark the connection with the right process ID. */
  if(uip_connected()) {
    l = &listenports[0];
    for(i = 0; i < UIP_LISTENPORTS; ++i) {
      if(l->port == uip_conn->lport &&
	 l->id != EK_ID_NONE) {
	s->id = l->id;
	s->state = NULL;
	break;
      }
      ++l;
    }
  }
  

  for(p = dispatcher_procs; p != NULL; p = p->next) {
    if(p->id == s->id &&
       p->uiphandler != NULL) {
      dispatcher_current = p->id;
      curproc = p;
#if CC_FUNCTION_POINTER_ARGS
      p->uiphandler(s->state);
#else /* CC_FUNCTION_POINTER_ARGS */
      dispatcher_uipcall_state = s->state;
      p->uiphandler();
#endif /* CC_FUNCTION_POINTER_ARGS */            
      return;
    }
  }
  /* If we get here, the process that created the connection has
     exited, so we abort this connection. */
  uip_abort();
}
#endif /* WITH_UIP */
/*-----------------------------------------------------------------------------------*/
/**
 * Opens a TCP port for incoming requests.
 *
 * 
 *
 * \param port The TCP port number that should be opened for
 * listening, in network byte order.
 */
/*-----------------------------------------------------------------------------------*/
void
dispatcher_uiplisten(u16_t port)
{
  static unsigned char i;
  struct listenport *l;

  l = listenports;
  for(i = 0; i < UIP_LISTENPORTS; ++i) {
    if(l->port == 0) {
      l->port = port;
      l->id = dispatcher_current;
#ifdef WITH_UIP
      uip_listen(port);
#endif /* WITH_UIP */      
      break;
    }
    ++l;
  }

}
/*-----------------------------------------------------------------------------------*/
/**
 * Associates a generic pointer with a uIP connection.
 *
 * This function is used for registering a pointer to a uIP
 * connection. This pointer will be passed as an argument to the
 * process' uIP handler function for every uIP event.
 *
 * \param conn The uIP connection to which the pointer is to be
 * associated.
 *
 * \param appstate The generic pointer that is to be associated with
 * the uIP connection.
 */
/*-----------------------------------------------------------------------------------*/
void
dispatcher_markconn(struct uip_conn *conn,
		    void *appstate)
{
  struct dispatcher_uipstate *s;

  s = (struct dispatcher_uipstate *)conn->appstate;
  s->id = dispatcher_current;
  s->state = appstate;
}
/*-----------------------------------------------------------------------------------*/
/**
 * Registers the calling process as a listener for a signal.
 *
 * \param s The signal to which the process should listen.
 *
 */
/*-----------------------------------------------------------------------------------*/
void
dispatcher_listen(ek_signal_t s)
{
  if(curproc != NULL) {
    curproc->signals[s] = 1;
  }
}
/*-----------------------------------------------------------------------------------*/
/**
 * Finds the process structure for a specific process ID.
 *
 * \param id The process ID for the process.
 *
 * \return The process structure for the process, or NULL if there
 * process ID was not found.
 */
/*-----------------------------------------------------------------------------------*/
struct dispatcher_proc *
dispatcher_process(ek_id_t id)
{
  struct dispatcher_proc *p;
  for(p = dispatcher_procs; p != NULL; p = p->next) {
    if(p->id == id) {
      return p;
    }
  }
  return NULL;
}
/*-----------------------------------------------------------------------------------*/
/**
 * Initializes the dispatcher module.
 *
 * Must be called during the initialization of Contiki.
 *
 */
/*-----------------------------------------------------------------------------------*/
void
dispatcher_init(void)
{
  static unsigned char i;
  
  for(i = 0; i < UIP_LISTENPORTS; ++i) {
    listenports[i].port = 0;
  }

  lastsig = 1;
  dispatcher_signal_quit = dispatcher_sigalloc();

  nsignals = fsignal = 0;

  arg_init();
}
/*-----------------------------------------------------------------------------------*/
/**
 * \internal Delivers a signal to specific process.
 *
 * \param s The signal number
 * \param data The signal data pointer
 * \param id The process to which the signal should be delivered
 */
/*-----------------------------------------------------------------------------------*/
static void CC_FASTCALL
deliver(ek_signal_t s, ek_data_t data,
	ek_id_t id)
{
  struct dispatcher_proc *p;
  for(p = dispatcher_procs; p != NULL; p = p->next) {      
    if((id == DISPATCHER_BROADCAST ||
	p->id == id) &&
       p->signals[s] != 0 &&
       p->signalhandler != NULL) {
      dispatcher_current = p->id;
      curproc = p;
#if CC_FUNCTION_POINTER_ARGS
      p->signalhandler(s, data);
#else /* CC_FUNCTION_POINTER_ARGS */
      dispatcher_sighandler_s = s;
      dispatcher_sighandler_data = data;
      p->signalhandler();
#endif /* CC_FUNCTION_POINTER_ARGS */      
    }
  }  
}
/*-----------------------------------------------------------------------------------*/
/**
 * Emits a signal, and delivers the signal immediately.
 *
 * This function emits a signal and calls the listening processes'
 * signal handlers immediately, before returning to the caller. This
 * function requires more call stack space than the dispatcher_emit()
 * function and should be used with care, and only in situtations
 * where the exact implications are known.
 *
 * In most situations, the dispatcher_emit() function should be used
 * instead.
 *
 * \param s The signal to be emitted.
 *
 * \param data The auxillary data to be sent with the signal
 *
 * \param id The process ID to which the signal should be emitted, or
 * DISPATCHER_BROADCAST if the signal should be emitted to all
 * processes listening for the signal.
 *
 */
/*-----------------------------------------------------------------------------------*/
void
dispatcher_fastemit(ek_signal_t s, ek_data_t data,
		    ek_id_t id)
{
  ek_id_t pid;
  struct dispatcher_proc *p;

  pid = dispatcher_current;
  p = curproc;

  deliver(s, data, id);
  
  dispatcher_current = pid;
  curproc = p;  
}
/*-----------------------------------------------------------------------------------*/
/**
 * Process the next signal in the signal queue and deliver it to
 * listening processes.
 *
 */
/*-----------------------------------------------------------------------------------*/
void
dispatcher_process_signal(void)
{ 
  static ek_signal_t s;
  static ek_data_t data;
  static ek_id_t id;
  
  /* If there are any signals in the queue, take the first one and
     walk through the list of processes to see if the signal should be
     delivered to any of them. If so, we call the signal handler
     function for the process. We only process one signal at a time
     and call the idle handlers inbetween. */

  if(nsignals > 0) {
    /* There are signals that we should deliver. */
    s = signals[fsignal].s;
    
    data = signals[fsignal].data;
    id = signals[fsignal].id;

    /* Since we have seen the new signal, we move pointer upwards
       and decrese number. */
    fsignal = (fsignal + 1) % EK_CONF_NUMSIGNALS;
    --nsignals;

    deliver(s, data, id);
  }

}
/*-----------------------------------------------------------------------------------*/
/**
 * Call each process' idle handler.
 */
/*-----------------------------------------------------------------------------------*/
void
dispatcher_process_idle(void)
{
  struct dispatcher_proc *p;
  
  /* Call idle handlers. */
  for(p = dispatcher_procs; p != NULL; p = p->next) {
    if(p->idle != NULL) {
      dispatcher_current = p->id;
      curproc = p;
      p->idle();
    }
  }
  
}
/*-----------------------------------------------------------------------------------*/
/**
 * Run the system - process signals and call idle handlers.
 *
 * This function should be called after all other initialization
 * stuff, and will never return.
 */
/*-----------------------------------------------------------------------------------*/
void
dispatcher_run(void)
{
  while(1) {
    /* Process one signal */
    dispatcher_process_signal();

    /* Run "idle" handlers */
    dispatcher_process_idle();
  }
}
/*-----------------------------------------------------------------------------------*/
/**
 * Emits a signal to a process.
 *
 * \param s The signal to be emitted.
 *
 * \param data The auxillary data to be sent with the signal
 *
 * \param id The process ID to which the signal should be emitted, or
 * DISPATCHER_BROADCAST if the signal should be emitted to all
 * processes listening for the signal.
 *
 * \retval EK_ERR_OK The signal could be emitted.
 *
 * \retval EK_ERR_FULL The signal queue was full and the signal could
 * not be emitted
 */
/*-----------------------------------------------------------------------------------*/
ek_err_t
dispatcher_emit(ek_signal_t s, ek_data_t data, ek_id_t id)
{
  static unsigned char snum;
  
  if(nsignals == EK_CONF_NUMSIGNALS) {
    return EK_ERR_FULL;
  }
  
  if(s != EK_SIGNAL_NONE) {
    snum = (fsignal + nsignals) % EK_CONF_NUMSIGNALS;
    signals[snum].s = s;
    signals[snum].data = data;
    signals[snum].id = id;
    ++nsignals;
  }
  
  return EK_ERR_OK;
}
/*-----------------------------------------------------------------------------------*/
