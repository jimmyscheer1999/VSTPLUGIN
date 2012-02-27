//-----------------------------------------------------------------------------
// VST Plug-Ins SDK
// VSTGUI: Graphical User Interface Framework for VST plugins : 
//
// Version 4.0
//
// CDataBrowser written 2006 by Arne Scheffler
//
//-----------------------------------------------------------------------------
// VSTGUI LICENSE
// (c) 2011, Steinberg Media Technologies, All Rights Reserved
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

#ifndef __cdatabrowser__
#define __cdatabrowser__

#include "cscrollview.h"
#include "cfont.h"
#include "cdrawcontext.h"
#include <vector>
#include <string>

namespace VSTGUI {

class CTextEdit;
class CDataBrowser;
class CDataBrowserView;
class CDataBrowserHeader;
class GenericStringListDataBrowserSource;

//-----------------------------------------------------------------------------
// IDataBrowser Declaration
//! @brief DataBrowser Interface
//-----------------------------------------------------------------------------------------------
class IDataBrowser
{
public:
	/** @name Setup */
	///	@{
	virtual int32_t dbGetNumRows (CDataBrowser* browser) = 0;	///< return number of rows for CDataBrowser browser
	virtual int32_t dbGetNumColumns (CDataBrowser* browser) = 0;	///< return number of columns for CDataBrowser browser
	virtual bool dbGetColumnDescription (int32_t index, CCoord& minWidth, CCoord& maxWidth, CDataBrowser* browser) { return false; }
	virtual CCoord dbGetCurrentColumnWidth (int32_t index, CDataBrowser* browser) = 0;	///< return current width of index column
	virtual void dbSetCurrentColumnWidth (int32_t index, const CCoord& width, CDataBrowser* browser) {}	///< the width of a column has changed
	virtual CCoord dbGetRowHeight (CDataBrowser* browser) = 0;	///< return height of one row
	virtual bool dbGetLineWidthAndColor (CCoord& width, CColor& color, CDataBrowser* browser) { return false; } ///< return the line width and color
	virtual void dbAttached (CDataBrowser* browser) {}	///< databrowser view was attached to a parent
	virtual void dbRemoved (CDataBrowser* browser) {}		///< databrowser view will be removed from its parent
	///	@}

	/** @name Drawing */
	///	@{
	virtual void dbDrawHeader (CDrawContext* context, const CRect& size, int32_t column, int32_t flags, CDataBrowser* browser) = 0;	///< draw the db header
	virtual void dbDrawCell (CDrawContext* context, const CRect& size, int32_t row, int32_t column, int32_t flags, CDataBrowser* browser) = 0;	///< draw a db cell
	///	@}

	/** @name Mouse Handling */
	///	@{
	virtual CMouseEventResult dbOnMouseDown (const CPoint& where, const CButtonState& buttons, int32_t row, int32_t column, CDataBrowser* browser) { return kMouseDownEventHandledButDontNeedMovedOrUpEvents; } ///< mouse button was pressed on a cell
	virtual CMouseEventResult dbOnMouseMoved (const CPoint& where, const CButtonState& buttons, int32_t row, int32_t column, CDataBrowser* browser) { return kMouseEventNotHandled; } ///< mouse was moved over a cell
	virtual CMouseEventResult dbOnMouseUp (const CPoint& where, const CButtonState& buttons, int32_t row, int32_t column, CDataBrowser* browser) { return kMouseEventNotHandled; } ///< mouse button was released on a cell
	///	@}

	/** @name Drag'n Drop Handling */
	///	@{
	virtual void dbOnDragEnterBrowser (CDragContainer* drag, CDataBrowser* browser) {}
	virtual void dbOnDragExitBrowser (CDragContainer* drag, CDataBrowser* browser) {}
	virtual void dbOnDragEnterCell (int32_t row, int32_t column, CDragContainer* drag, CDataBrowser* browser) {}
	virtual void dbOnDragExitCell (int32_t row, int32_t column, CDragContainer* drag, CDataBrowser* browser) {}
	virtual bool dbOnDropInCell (int32_t row, int32_t column, CDragContainer* drag, CDataBrowser* browser) { return false; }
	///	@}

	/** @name Selection  */
	///	@{
	virtual void dbSelectionChanged (CDataBrowser* browser) {}	///< the selection of the db changed
	///	@}

	/** @name Cell Text Editing */
	///	@{
	virtual void dbCellTextChanged (int32_t row, int32_t column, UTF8StringPtr newText, CDataBrowser* browser) {} ///< the text of the cell changed beginTextEdit was called for
	virtual void dbCellSetupTextEdit (int32_t row, int32_t column, CTextEdit* textEditControl, CDataBrowser* browser) {} ///< beginTextEdit calls this, so you can setup the textedit control
	//@}

	/** @name Keyboard Handling */
	///	@{
	virtual int32_t dbOnKeyDown (const VstKeyCode& key, CDataBrowser* browser) { return -1; }
	///	@}

	enum {
		kRowSelected = 1 << 1
	};
};

//-----------------------------------------------------------------------------
// CDataBrowser Declaration
//! @brief DataBrowser view
/// @ingroup controls
//-----------------------------------------------------------------------------------------------
class CDataBrowser : public CScrollView
{
protected:
	enum
	{
		kDrawRowLinesFlag = kLastScrollViewStyleFlag,
		kDrawColumnLinesFlag,
		kDrawHeaderFlag
	};
	
public:
	CDataBrowser (const CRect& size, CFrame* pParent, IDataBrowser* db, int32_t style = 0, CCoord scrollbarWidth = 16, CBitmap* pBackground = 0);

	enum CDataBrowserStyle 
	{
		// see CScrollView for more styles
		kDrawRowLines			= 1 << kDrawRowLinesFlag,
		kDrawColumnLines		= 1 << kDrawColumnLinesFlag,
		kDrawHeader				= 1 << kDrawHeaderFlag
	};
	
	enum 
	{
		kNoSelection	= -1
	};

	//-----------------------------------------------------------------------------
	/// @name CDataBrowser Methods
	//-----------------------------------------------------------------------------
	//@{
	virtual void recalculateLayout (bool rememberSelection = false);						///< trigger recalculation, call if numRows or numColumns changed
	virtual void invalidate (int32_t row, int32_t column);									///< invalidates an individual cell
	virtual void invalidateRow (int32_t row);												///< invalidates a complete row
	virtual void makeRowVisible (int32_t row);												///< scrolls the scrollview so that row is visible

	virtual CRect getCellBounds (int32_t row, int32_t column);								///< get bounds of a cell

	virtual int32_t getSelectedRow () const { return selectedRow; }							///< get selected row
	virtual void setSelectedRow (int32_t row, bool makeVisible = false);					///< set the exclusive selected row

	virtual void beginTextEdit (int32_t row, int32_t column, UTF8StringPtr initialText);	///< starts a text edit for a cell

	IDataBrowser* getDataSource () const { return db; }										///< get data source object
	//@}

	void setAutosizeFlags (int32_t flags);
	void setViewSize (const CRect& size, bool invalid);

	int32_t onKeyDown (VstKeyCode& keyCode);
	CMouseEventResult onMouseDown (CPoint& where, const CButtonState& buttons);
protected:
	~CDataBrowser ();
	void valueChanged (CControl *pControl);
	CMessageResult notify (CBaseObject* sender, IdStringPtr message);
	bool attached (CView *parent);
	bool removed (CView* parent);

	IDataBrowser* db;
	CDataBrowserView* dbView;
	CDataBrowserHeader* dbHeader;
	CViewContainer* dbHeaderContainer;
	int32_t selectedRow;
};

//-----------------------------------------------------------------------------
class IGenericStringListDataBrowserSourceSelectionChanged
{
public:
	virtual void dbSelectionChanged (int32_t selectedRow, GenericStringListDataBrowserSource* source) = 0;
};

//-----------------------------------------------------------------------------
// GenericStringListDataBrowserSource Declaration
//! @brief Generic string list data browser source
//-----------------------------------------------------------------------------------------------
class GenericStringListDataBrowserSource : public IDataBrowser, public CBaseObject
{
public:
	GenericStringListDataBrowserSource (const std::vector<std::string>* stringList, IGenericStringListDataBrowserSourceSelectionChanged* delegate = 0);
	~GenericStringListDataBrowserSource ();

	void setStringList (const std::vector<std::string>* stringList);
	const std::vector<std::string>* getStringList () const { return stringList; }

	void setupUI (const CColor& selectionColor, const CColor& fontColor, const CColor& rowlineColor, const CColor& rowBackColor, const CColor& rowAlteranteBackColor, CFontRef font = 0, int32_t rowHeight = -1);

protected:
	int32_t dbGetNumRows (CDataBrowser* browser);
	int32_t dbGetNumColumns (CDataBrowser* browser) { return 1; }
	bool dbGetColumnDescription (int32_t index, CCoord& minWidth, CCoord& maxWidth, CDataBrowser* browser) { return false; }
	CCoord dbGetCurrentColumnWidth (int32_t index, CDataBrowser* browser);
	void dbSetCurrentColumnWidth (int32_t index, const CCoord& width, CDataBrowser* browser) {}
	CCoord dbGetRowHeight (CDataBrowser* browser);
	bool dbGetLineWidthAndColor (CCoord& width, CColor& color, CDataBrowser* browser);

	void dbDrawHeader (CDrawContext* context, const CRect& size, int32_t column, int32_t flags, CDataBrowser* browser);
	void dbDrawCell (CDrawContext* context, const CRect& size, int32_t row, int32_t column, int32_t flags, CDataBrowser* browser);

	CMouseEventResult dbOnMouseDown (const CPoint& where, const CButtonState& buttons, int32_t row, int32_t column, CDataBrowser* browser) { return kMouseDownEventHandledButDontNeedMovedOrUpEvents; }
	CMouseEventResult dbOnMouseMoved (const CPoint& where, const CButtonState& buttons, int32_t row, int32_t column, CDataBrowser* browser) { return kMouseEventNotHandled; }
	CMouseEventResult dbOnMouseUp (const CPoint& where, const CButtonState& buttons, int32_t row, int32_t column, CDataBrowser* browser) { return kMouseEventNotHandled; }

	void dbSelectionChanged (CDataBrowser* browser);

	void dbCellTextChanged (int32_t row, int32_t column, UTF8StringPtr newText, CDataBrowser* browser) {}
	void dbCellSetupTextEdit (int32_t row, int32_t column, CTextEdit* textEditControl, CDataBrowser* browser) {}

	int32_t dbOnKeyDown (const VstKeyCode& key, CDataBrowser* browser);

	void dbAttached (CDataBrowser* browser);
	void dbRemoved (CDataBrowser* browser);

	CMessageResult notify (CBaseObject* sender, IdStringPtr message);

	const std::vector<std::string>* stringList;
	int32_t rowHeight;
	CColor fontColor;
	CColor selectionColor;
	CColor rowlineColor;
	CColor rowBackColor;
	CColor rowAlternateBackColor;
	CPoint textInset;
	CHoriTxtAlign textAlignment;
	CFontRef drawFont;
	CDataBrowser* dataBrowser;
	IGenericStringListDataBrowserSourceSelectionChanged* delegate;

	CVSTGUITimer* timer;
	std::string keyDownFindString;
};

} // namespace

#endif
