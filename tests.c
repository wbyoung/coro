// 
//  tests.c
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
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

static const int k_fibonnaci_max = 20;
static const int k_two_coro_max = 5;

extern unsigned int coro_allocs;
extern unsigned int coro_deallocs;

typedef struct {
	coro *main;
	coro *first;
	coro *second;
} task_context;

void simple_coro(coro *self) {
	printf("simple coro\n");
	coro_return(self);
}

void fibonacci_coro(coro *self, int *result) {
	int previous = 0;
	int current = 1;

	*result = 1;
	coro_yield(self);

	while (1) {
		*result = current + previous;
		previous = current;
		current = *result;
		coro_yield(self);
	}
}

void nest_inner_coro(coro *self) {
	printf(".");
	coro_return(self);
}

void nest_outer_coro(coro *self) {
	while (true) {
		printf("outer coro, two inners: ");
		coro *inner = coro_new((void *)nest_inner_coro, NULL);
		coro_call(self, inner);
		coro_run(inner);
		coro_release(inner);
		printf("\nouter coro, complete\n");
		coro_yield(self);
	}
}

#define STACK_CHECK(coro) do { \
	if (stack != coro_stack_remaining(context->coro)) { \
		fprintf(stderr, "abort: stack is shrinking\n"); abort(); \
	} \
} while (0)

void first_coro(coro *self, task_context *context) {
	int num = 1;
	int stack = coro_stack_remaining(context->first);
	while (true) {
		STACK_CHECK(first);
		printf("coro one: %d\n", num++);
		coro_yieldto(self, context->second);
		if (num > k_two_coro_max) { coro_yieldto(self, context->main); } // finished
	}
}

void second_coro(coro *self, task_context *context) {
	int num = 1;
	int stack = coro_stack_remaining(context->second);
	while (true) {
		STACK_CHECK(second);
		printf("coro two: %d\n", num++);
		coro_yieldto(self, context->first);
		if (num > k_two_coro_max) { coro_yieldto(self, context->main); } // finished
	}
}

int main() {
	coro *self = coro_new(NULL, NULL);

	// simple coro
	// ------------------------------------------------------------------------
	// this is really just a standard routine, but must return via the
	// coro_return instead of completing. it should be possible to call it
	// multiple times. expected output is two printouts.
	// ------------------------------------------------------------------------
	coro *simple = coro_new((void *)simple_coro, NULL);
	coro_run(simple);
	coro_call(self, simple);
	coro_release(simple);
	
	
	// fibonacci coro test
	// ------------------------------------------------------------------------
	// test the simple fibonacci number generator. the expected output is the
	// fibonnacci sequence up until the max.
	// ------------------------------------------------------------------------
	int number = 0;
	coro *fib = coro_new((void *)fibonacci_coro, &number);
	while (number < k_fibonnaci_max) {
		coro_run(fib);
		printf("fibonacci: %i\n", number);
	}
	coro_release(fib);
	
	
	// depth coro test
	// ------------------------------------------------------------------------
	// test yielding in nested coros. the expected output is self explanitory.
	// ------------------------------------------------------------------------
	coro *nest = coro_new((void *)nest_outer_coro, NULL);
	coro_run(nest);
	coro_release(nest);
	
	
	// alternating coro test
	// ------------------------------------------------------------------------
	// the first and second coroutines yield to each other until one of them
	// reaches a specified maximum value. at that point, it will yeild back to
	// the main coroutine (this main function). the context that is passed to
	// both coroutines includes pointers to all three coroutines so the routine
	// can use what it wants to. the expected output is both routines printing
	// all numbers until the max starting with the first routine.
	// ------------------------------------------------------------------------
	task_context *context = &(task_context){};
	context->main = coro_retain(self);
	context->first = coro_new((void *)first_coro, context);
	context->second = coro_new((void *)second_coro, context);
	coro_call(context->main, context->first);
	coro_release(context->main);
	coro_release(context->first);
	coro_release(context->second);
	
	coro_release(self);
	if (coro_allocs) {
		printf("coros cleaned up (%i/%i)\n", coro_deallocs, coro_allocs);
	}
	printf("tests successful\n");
}
