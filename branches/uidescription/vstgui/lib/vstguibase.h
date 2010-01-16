//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework not only for VST plugins : 
//
// Version 4.0
//
//-----------------------------------------------------------------------------
// VSTGUI LICENSE
// (c) 2009, Steinberg Media Technologies, All Rights Reserved
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
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A  PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#ifndef __vstguibase__
#define __vstguibase__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//-----------------------------------------------------------------------------
// VSTGUI Version
//-----------------------------------------------------------------------------
#define VSTGUI_VERSION_MAJOR  4
#define VSTGUI_VERSION_MINOR  0

//-----------------------------------------------------------------------------
// Platform definitions
//-----------------------------------------------------------------------------
#if WIN32
	#define WINDOWS 1
#elif __APPLE_CC__
	#include <AvailabilityMacros.h>
	#ifndef MAC_OS_X_VERSION_10_5
		#define MAC_OS_X_VERSION_10_5 1050
	#endif
	#ifndef MAC_COCOA
		#define MAC_COCOA (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5)
	#endif
	#ifndef MAC
		#define MAC 1
	#endif
	#if !__LP64__ && !defined (MAC_CARBON)
		#define MAC_CARBON 1
		#ifndef TARGET_API_MAC_CARBON
			#define TARGET_API_MAC_CARBON 1
		#endif
		#ifndef __CF_USE_FRAMEWORK_INCLUDES__
			#define __CF_USE_FRAMEWORK_INCLUDES__ 1
		#endif
	#endif
#endif

#if WINDOWS
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0600
	#endif
#endif

#ifdef UNICODE
	#undef UNICODE
#endif
#define UNICODE 1

//----------------------------------------------------
// Deprecation setting
//----------------------------------------------------
#ifndef VSTGUI_ENABLE_DEPRECATED_METHODS
#define VSTGUI_ENABLE_DEPRECATED_METHODS 1
#endif

#ifndef DEPRECATED_ATTRIBUTE
#define DEPRECATED_ATTRIBUTE
#endif

#if VSTGUI_ENABLE_DEPRECATED_METHODS
#define VSTGUI_DEPRECATED(x)	DEPRECATED_ATTRIBUTE	x
#else
#define VSTGUI_DEPRECATED(x)
#endif

//----------------------------------------------------
// Helper makros
//----------------------------------------------------
#define	VSTGUI_MAKE_STRING_PRIVATE_DONT_USE(x)	# x
#define	VSTGUI_MAKE_STRING(x)					VSTGUI_MAKE_STRING_PRIVATE_DONT_USE(x)

#if DEVELOPMENT
	#ifndef DEBUG
	#define DEBUG	1
	#endif
#endif

namespace VSTGUI {

#if DEBUG
#define CLASS_METHODS(name, parent)             \
	virtual bool isTypeOf (const char* s) const \
		{ return (!strcmp (s, (#name))) ? true : parent::isTypeOf (s); }\
	virtual const char* getClassName () const { return (#name); } \
	virtual CBaseObject* newCopy () const { return new name (*this); }
#define CLASS_METHODS_NOCOPY(name, parent)             \
	virtual bool isTypeOf (const char* s) const \
		{ return (!strcmp (s, (#name))) ? true : parent::isTypeOf (s); }\
	virtual const char* getClassName () const { return (#name); } \
	virtual CBaseObject* newCopy () const { return 0; }
#else
#define CLASS_METHODS(name, parent)             \
	virtual bool isTypeOf (const char* s) const \
		{ return (!strcmp (s, (#name))) ? true : parent::isTypeOf (s); } \
	virtual CBaseObject* newCopy () const { return (CBaseObject*)new name (*this); }
#define CLASS_METHODS_NOCOPY(name, parent)             \
	virtual bool isTypeOf (const char* s) const \
		{ return (!strcmp (s, (#name))) ? true : parent::isTypeOf (s); } \
	virtual CBaseObject* newCopy () const { return 0; }
#endif
#define CLASS_METHODS_VIRTUAL(name, parent)             \
	virtual bool isTypeOf (const char* s) const \
		{ return (!strcmp (s, (#name))) ? true : parent::isTypeOf (s); } \
	virtual CBaseObject* newCopy () const = 0;

typedef double CCoord;	///< coordinate type

//-----------------------------------------------------------------------------
// @brief Message Results
//-----------------------------------------------------------------------------
enum CMessageResult 
{
	kMessageUnknown = 0,
	kMessageNotified = 1
};

//-----------------------------------------------------------------------------
// CBaseObject Declaration
//! @brief Base Object with reference counter
//-----------------------------------------------------------------------------
class CBaseObject
{
public:
	CBaseObject () : nbReference (1) {}
	virtual ~CBaseObject () {}

	//-----------------------------------------------------------------------------
	/// @name Reference Counting Methods
	//-----------------------------------------------------------------------------
	//@{
	virtual void forget () { nbReference--; if (nbReference == 0) delete this; }	///< decrease refcount and delete object if refcount == 0
	virtual void remember () { nbReference++; }										///< increase refcount
	virtual long getNbReference () const { return nbReference; }					///< get refcount
	//@}
	
	//-----------------------------------------------------------------------------
	/// @name Message Methods
	//-----------------------------------------------------------------------------
	//@{
	virtual CMessageResult notify (CBaseObject* sender, const char* message) { return kMessageUnknown; }
	//@}

	/// @cond ignore
	virtual bool isTypeOf (const char* s) const { return (!strcmp (s, "CBaseObject")); }
	virtual CBaseObject* newCopy () const { return 0; }
	/// @endcond

	#if DEBUG
	virtual const char* getClassName () const { return "CBaseObject"; }
	#endif
	
private:
	long nbReference;
};

//-----------------------------------------------------------------------------
class CBaseObjectGuard
{
public:
	CBaseObjectGuard (CBaseObject* _obj) : obj (_obj) { if (obj) obj->remember (); }
	~CBaseObjectGuard () { if (obj) obj->forget (); }
protected:
	CBaseObject* obj;
};

} // namespace

//-----------------------------------------------------------------------------
#include "vstguidebug.h"

#endif
