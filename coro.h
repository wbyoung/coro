//  
//  coro.h
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

#ifndef CORO_H
#define CORO_H

#include <sys/types.h>

typedef struct coro coro;
typedef void (*coro_start)(coro *self, void *context);

/*!
 \brief		Create a coroutine
 \details	Create a coroutine and specify the starting function and context.
			You must release the coroutine when it is no longer being used.
 */
coro *coro_new(coro_start start, void *context);

/*!
 \brief		Retain a coroutine
 \details	Retain a coroutine.
 */
coro *coro_retain(coro *self);

/*!
 \brief		Release a coroutine
 \details	Release a coroutine.
 */
void coro_release(coro *self);

/*!
 \brief		Calls a coroutine
 \details	Suspends the receiving coroutine and resumes the calling coroutine.
			The next coroutine will be able to yeild or return to resume the
			receiving coroutine.
 */
void coro_call(coro *self, coro *next);

/*!
 \brief		Yield to the calling coroutine
 \details	Suspends the receiving coroutine and resumes the calling coroutine.
 */
void coro_yield(coro *self);

/*!
 \brief		Returns from a coroutine
 \details	Suspends the receiving coroutine and resumes the calling coroutine.
			The receiving coroutine is reset so that any future calls or yields
			into the coroutine actually start it from it's intial entry point
			(with the original context).
 */
void coro_return(coro *self);

/*!
 \brief		Yield to another coroutine
 \details	Suspends the receiving coroutine and resumes the next coroutine.
			This is a more powerful than using call to enter into another coroutine.
			It leaves you in charge of the control flow, so you should be careful
			when using it. The next coroutine does not have the ability to use return
			or yield to return to the receiving coroutine. The coroutine can use yield
			and return if and only if another coroutine called into it at some point.
			Otheriwse, the only valid control flow out of the coroutine is preforming
			another yieldto.
 */
void coro_yieldto(coro *self, coro *next);

/*!
 \brief		Runs a coroutine
 \details	Runs a coroutine. Note that this is similar to doing a call after
			creating a new coroutine with no callback function. If you plan to
			continuously invoke coroutines in this way, it is actually more
			efficient to create a single coroutine and use call rather than run.
 */
void coro_run(coro *any);

/*!
 \brief		Remaining stack space
 \details	Gets the remaining stack space for the coroutine. This should be
			used for debugging purposes only.
 */
size_t coro_stack_remaining(coro *self);

#endif
