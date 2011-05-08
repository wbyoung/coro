// 
//  coro.c
//  Coroutines
//  
//  Copyright (c) 2011, FadingRed LLC
//  All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
//  following conditions are met:
//  
//    - Redistributions of source code must retain the above copyright notice, this list of conditions and the
//      following disclaimer.
//    - Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//      following disclaimer in the documentation and/or other materials provided with the distribution.
//    - Neither the name of the FadingRed LLC nor the names of its contributors may be used to endorse or promote
//      products derived from this software without specific prior written permission.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
//  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
//  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
//  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  
//  Based on: https://github.com/stevedekorte/coroutine
//  Portions of code copyright (c) 2002, 2003 Steve Dekorte
// 

#include <coro.h>

#define _XOPEN_SOURCE
#include <ucontext.h>
#undef _XOPEN_SOURCE

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stddef.h>
#include <signal.h>

#define CORO_DEFAULT_STACK_SIZE (128*1024)
#undef CORO_TRACK_ALLOCATIONS

unsigned int coro_allocs;
unsigned int coro_deallocs;
#ifdef CORO_TRACK_ALLOCATIONS
#define CORO_ALLOC do { coro_allocs += 1; } while (0)
#define CORO_DEALLOC do { coro_deallocs += 1; } while (0)
#else
#define CORO_ALLOC
#define CORO_DEALLOC
#endif

struct coro {
	unsigned long refcount;
	coro *caller;
	void *context;
	void *stack;
	size_t stack_size;
	ucontext_t env;
	coro_start start;
};


// private functions
// ------------------------------------------------------------------

void coro_setup(coro *self);
void coro_set_caller(coro *self, coro *caller);


// general coro object
// ------------------------------------------------------------------

coro *coro_alloc(void) {
	CORO_ALLOC;
	return (coro *)calloc(1, sizeof(coro));
}

void coro_dealloc(coro *self) {
	coro_release(self->caller);
	free(self->stack);
	free(self);
	CORO_DEALLOC;
}

coro *coro_new(coro_start start, void *context) {
	coro *self = coro_alloc();
	
	self->refcount = 1;
	if (start) {
		self->context = context;
		self->start = start;
		self->stack_size = CORO_DEFAULT_STACK_SIZE;
		self->stack = calloc(1, self->stack_size);
		coro_setup(self);
	}
	return self;
}

coro *coro_retain(coro *self) {
	if (self) {
		self->refcount += 1;
	}
	return self;
}

void coro_release(coro *self) {
	if (self) {
		self->refcount -= 1;
		if (self->refcount <= 0) {
			coro_dealloc(self);
		}
	}
}

void coro_set_caller(coro *self, coro *caller) {
	if (!self) { return; }
	if (self->caller != caller) {
		coro_release(self->caller);
		self->caller = coro_retain(caller);
	}
}


// control flow
// ------------------------------------------------------------------

void coro_yield_error(void);
void coro_yield_error(void) {}

void coro_yieldto_context(coro *self, coro *next, coro *caller, ucontext_t *context) {
	if (!next) {
		fprintf(stderr,
			"coroutine error: "
			"attempt to yield to undefined coro. "
			"break on coro_yield_error to debug\n");
		coro_yield_error();
		abort();
	}
	if (caller) { // only set if provided
		coro_set_caller(next, caller);
	}
	swapcontext(context, &next->env);
}

void coro_run(coro *next) {
	coro *caller = coro_new(NULL, NULL);
	coro_call(caller, next);
	coro_release(caller);
}

void coro_call(coro *self, coro *next) {
	coro_yieldto_context(self, next, self, &self->env);
}

void coro_yieldto(coro *self, coro *next) {
	coro_yieldto_context(self, next, NULL, &self->env);
}

void coro_yield(coro *self) {
	coro_yieldto_context(self, self->caller, NULL, &self->env);
}

void coro_return(coro *self) {
	ucontext_t current = self->env;
	coro *next = self->caller;
	coro_setup(self);
	coro_yieldto_context(self, next, NULL, &current);
}


// context handling
// ------------------------------------------------------------------

void coro_return_error(void);
void coro_return_error(void) {}

void coro_entry(coro *self) {
	self->start(self, self->context);
	fprintf(stderr,
		"coroutine error: "
		"returned from a coroutine, use coro_return to return. "
		"break on coro_return_error to debug\n");
	coro_return_error();
	abort();
}

#ifdef __x86_64__
void coro_entry_64(int selfHi, int selfLo) {
	coro_entry((coro *)(((long long)selfHi << 32) | (long long)selfLo));
}
#endif

void coro_setup(coro *self) {
	ucontext_t *ucp = &self->env;
	getcontext(ucp);

	ucp->uc_stack.ss_sp = self->stack;
	ucp->uc_stack.ss_size = self->stack_size;
	ucp->uc_stack.ss_flags = 0;
	ucp->uc_link = NULL;

#ifdef __x86_64__
	unsigned int selfHi = (unsigned int)((long long)self >> 32);
	unsigned int selfLo = (unsigned int)((long long)self & 0xFFFFFFFF);
	makecontext(ucp, (void(*)(void))coro_entry_64, 2, selfHi, selfLo);
#else
	makecontext(ucp, (void(*)(void))coro_entry, 1, self);
#endif
}


// debugging information
// ------------------------------------------------------------------

char *coro_current_sp(void) __attribute__ ((noinline));
char *coro_current_sp(void) {
	return &(char){0};
}

size_t coro_stack_remaining(coro *self) {
	unsigned char dummy;
	ptrdiff_t p1 = (ptrdiff_t)(&dummy);
	ptrdiff_t p2 = (ptrdiff_t)coro_current_sp();
	ptrdiff_t start = ((ptrdiff_t)self->stack);
	ptrdiff_t end = start + self->stack_size;
	int stackMovesUp = p2 > p1;

	if (stackMovesUp) { return end - p1; } // like x86
	else { return p1 - start; } // like ppc
}
