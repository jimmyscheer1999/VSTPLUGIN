//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework not only for VST plugins : 
//
// xmlparser
//
//-----------------------------------------------------------------------------
// VSTGUI LICENSE
// (c) 2008, Steinberg Media Technologies, All Rights Reserved
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

#include "xmlparser.h"

extern "C" {
#define XML_STATIC 1
#define XML_NS 1
#define XML_DTD 1
#define XML_CONTEXT_BYTES 1024
#if MAC && defined (__BIG_ENDIAN__)
	#define BYTEORDER 4321
#else
	#define BYTEORDER 1234
#endif
#define HAVE_MEMMOVE	1
#include "expat/expat.h"
}

#define PARSER ((XML_ParserStruct*)parser)

namespace VSTGUI {
namespace Xml {

//------------------------------------------------------------------------
static void XMLCALL gStartElementHandler (void* userData, const char* name, const char** atts)
{
	Parser* parser = (Parser*)userData;
	IHandler* handler = parser ? parser->getHandler () : 0;
	if (handler)
		handler->startXmlElement (parser, name, atts);
}

//------------------------------------------------------------------------
static void XMLCALL gEndElementHandler (void* userData, const char* name)
{
	Parser* parser = (Parser*)userData;
	IHandler* handler = parser ? parser->getHandler () : 0;
	if (handler)
		handler->endXmlElement (parser, name);
}

//------------------------------------------------------------------------
static void XMLCALL gCharacterDataHandler (void* userData, const char* s, int len)
{
	Parser* parser = (Parser*)userData;
	IHandler* handler = parser ? parser->getHandler () : 0;
	if (handler)
		handler->xmlCharData (parser, s, len);
}

//------------------------------------------------------------------------
static void XMLCALL gCommentHandler (void* userData, const char* string)
{
	Parser* parser = (Parser*)userData;
	IHandler* handler = parser ? parser->getHandler () : 0;
	if (handler)
		handler->xmlComment (parser, string);
}

//-----------------------------------------------------------------------------
Parser::Parser ()
: parser (0)
, handler (0)
{
	parser = XML_ParserCreate ("UTF-8");
}

//-----------------------------------------------------------------------------
Parser::~Parser ()
{
	if (parser)
		XML_ParserFree (PARSER);
}

//-----------------------------------------------------------------------------
bool Parser::parse (IContentProvider* provider, IHandler* _handler)
{
	if (provider == 0 || _handler == 0)
		return false;

	handler = _handler;
	XML_SetUserData (PARSER, this);
	XML_SetStartElementHandler (PARSER, gStartElementHandler);
	XML_SetEndElementHandler (PARSER, gEndElementHandler);
	XML_SetCharacterDataHandler (PARSER, gCharacterDataHandler);
	XML_SetCommentHandler (PARSER, gCommentHandler);

	static const unsigned int kBufferSize = 0x8000;

	provider->rewind ();

	while (true) 
	{
		void* buffer = XML_GetBuffer (PARSER, kBufferSize);
		if (buffer == 0)
		{
			handler = 0;
			return false;
		}

		int bytesRead = provider->readRawXmlData ((char*)buffer, kBufferSize);

		XML_Status status = XML_ParseBuffer (PARSER, bytesRead, bytesRead == 0);
		switch (status) 
		{
			case XML_STATUS_ERROR:
			{
				XML_Error error = XML_GetErrorCode (PARSER);
				if (error == XML_ERROR_JUNK_AFTER_DOC_ELEMENT) // that's ok
				{
					handler = 0;
					return true;
				}
				handler = 0;
				return false;
			}
			case XML_STATUS_SUSPENDED:
			{
				handler = 0;
				return true;
			}
		}

		if (bytesRead == 0)
			break;
	}
	handler = 0;
	return true;
}

//-----------------------------------------------------------------------------
bool Parser::stop ()
{
	XML_StopParser (PARSER, false);
	return true;
}

}} // namespaces

extern "C" {
//------------------------------------------------------------------------
#include "./expat/xmltok.c"
#include "./expat/xmlrole.c"
#include "./expat/xmlparse.c"
}
