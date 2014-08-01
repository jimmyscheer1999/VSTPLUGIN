//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework for VST plugins
//
// Version 4.2
//
//-----------------------------------------------------------------------------
// VSTGUI LICENSE
// (c) 2013, Steinberg Media Technologies, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "cvstguitimer.h"

#if WINDOWS
#include <windows.h>
#include <list>
namespace VSTGUI {
static std::list<CVSTGUITimer*> gTimerList;
} // namespace

#endif

#if DEBUG
#define DEBUGLOG	0
#endif

namespace VSTGUI {

//-----------------------------------------------------------------------------
IdStringPtr CVSTGUITimer::kMsgTimer = "timer fired";

//-----------------------------------------------------------------------------
CVSTGUITimer::CVSTGUITimer (CBaseObject* timerObject, int32_t fireTime, bool doStart)
: fireTime (fireTime)
#if !VSTGUI_HAS_FUNCTIONAL
, timerObject (timerObject)
#endif
, platformTimer (0)
{
#if VSTGUI_HAS_FUNCTIONAL
	callbackFunc = [timerObject](CVSTGUITimer* timer) {
		timerObject->notify (timer, kMsgTimer);
	};
#endif
	if (doStart)
		start ();
}

#if VSTGUI_HAS_FUNCTIONAL
//-----------------------------------------------------------------------------
CVSTGUITimer::CVSTGUITimer (const CallbackFunc& callback, int32_t fireTime, bool doStart)
: fireTime (fireTime)
, platformTimer (0)
, callbackFunc (callback)
{
	if (doStart)
		start ();
}

//-----------------------------------------------------------------------------
CVSTGUITimer::CVSTGUITimer (CallbackFunc&& callback, int32_t fireTime, bool doStart)
: fireTime (fireTime)
, platformTimer (0)
, callbackFunc (std::move (callback))
{
	if (doStart)
		start ();
}
#endif

//-----------------------------------------------------------------------------
CVSTGUITimer::~CVSTGUITimer ()
{
	stop ();
}

//-----------------------------------------------------------------------------
bool CVSTGUITimer::start ()
{
	if (platformTimer == 0)
	{
		#if MAC
		CFRunLoopTimerContext timerContext = {0};
		timerContext.info = this;
		platformTimer = CFRunLoopTimerCreate (kCFAllocatorDefault, CFAbsoluteTimeGetCurrent () + fireTime * 0.001f, fireTime * 0.001f, 0, 0, timerCallback, &timerContext);
		if (platformTimer)
		{
		#if MAC_OS_X_VERSION_MIN_REQUIRED > MAC_OS_X_VERSION_10_8
			if (CFRunLoopTimerSetTolerance)
				CFRunLoopTimerSetTolerance ((CFRunLoopTimerRef)platformTimer, fireTime * 0.0001f);
		#endif
			CFRunLoopAddTimer (CFRunLoopGetCurrent (), (CFRunLoopTimerRef)platformTimer, kCFRunLoopCommonModes);
		}

		#elif WINDOWS
		platformTimer = (void*)SetTimer ((HWND)NULL, (UINT_PTR)0, fireTime, TimerProc);
		if (platformTimer)
			gTimerList.push_back (this);
		#endif
		
		#if DEBUGLOG
		DebugPrint ("Timer started (0x%x)\n", timerObject);
		#endif
	}
	return (platformTimer != 0);
}

//-----------------------------------------------------------------------------
bool CVSTGUITimer::stop ()
{
	if (platformTimer)
	{
		#if MAC
		CFRunLoopTimerInvalidate ((CFRunLoopTimerRef)platformTimer);
		CFRelease ((CFRunLoopTimerRef)platformTimer);

		#elif WINDOWS
		KillTimer ((HWND)NULL, (UINT_PTR)platformTimer);
		std::list<CVSTGUITimer*>::iterator it = gTimerList.begin ();
		while (it != gTimerList.end ())
		{
			if ((*it) == this)
			{
				gTimerList.remove (*it);
				break;
			}
			it++;
		}
		#endif
		platformTimer = 0;
		#if DEBUGLOG
		DebugPrint ("Timer stopped (0x%x)\n", timerObject);
		#endif
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool CVSTGUITimer::setFireTime (int32_t newFireTime)
{
	if (fireTime != newFireTime)
	{
		bool wasRunning = stop ();
		fireTime = newFireTime;
		if (wasRunning)
			return start ();
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
void CVSTGUITimer::fire ()
{
	CBaseObjectGuard guard (this);
#if VSTGUI_HAS_FUNCTIONAL
	if (callbackFunc)
		callbackFunc (this);
#else
	if (timerObject)
		timerObject->notify (this, kMsgTimer);
#endif
}

#if MAC
//-----------------------------------------------------------------------------
void CVSTGUITimer::timerCallback (CFRunLoopTimerRef t, void *info)
{
	CVSTGUITimer* timer = (CVSTGUITimer*)info;
	timer->fire ();
}

#elif WINDOWS
//------------------------------------------------------------------------
VOID CALLBACK CVSTGUITimer::TimerProc (HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	std::list<CVSTGUITimer*>::iterator it = gTimerList.begin ();
	while (it != gTimerList.end ())
	{
		if ((UINT_PTR)((*it)->platformTimer) == idEvent)
		{
			(*it)->fire ();
			break;
		}
		it++;
	}
}
#endif

} // namespace

