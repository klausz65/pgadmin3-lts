//////////////////////////////////////////////////////////////////////////
//
// pgAdmin III - PostgreSQL Tools
// Copyright (C) 2002 - 2003, The pgAdmin Development Team
// This software is released under the Artistic Licence
//
// ctlSQLResult.cpp - SQL Query result window
//
//////////////////////////////////////////////////////////////////////////

// wxWindows headers
#include <wx/wx.h>


#include "ctlSQLResult.h"


ctlSQLResult::ctlSQLResult(wxWindow *parent, pgConn *_conn, wxWindowID id, const wxPoint& pos, const wxSize& size)
: wxListView(parent, id, pos, size, wxLC_REPORT | wxSUNKEN_BORDER)
{
    conn=_conn;
    thread=0;
}



ctlSQLResult::~ctlSQLResult()
{
    Abort();
}



int ctlSQLResult::Execute(const wxString &query)
{
    Abort();
    ClearAll();
    rowsRetrieved=0;

    thread = new pgQueryThread(conn->connection(), query);

    if (thread->Create() != wxTHREAD_NO_ERROR)
    {
        Abort();
        return -1;
    }

    thread->Run();
    return RunStatus();
}


int ctlSQLResult::Abort()
{
    if (thread)
    {
        thread->Delete();
        delete thread;
    }
    thread=0;
    return 0;
}


int ctlSQLResult::RetrieveOne()
{
    if (!thread || !thread->DataValid())
        return -1;

    if (thread->ReturnCode() != PGRES_TUPLES_OK)
        return -1;

    if (!rowsRetrieved)
    {
        int w, h;
        GetSize(&w, &h);
        InsertColumn(0, thread->DataSet()->ColName(0), wxLIST_FORMAT_LEFT, w);

        while (!thread->DataSet()->Eof())
        {
            InsertItem(rowsRetrieved, thread->DataSet()->GetVal(0));
            rowsRetrieved++;
            thread->DataSet()->MoveNext();
        }
        return rowsRetrieved;
    }
    return 0;
}


int ctlSQLResult::Retrieve(long chunk)
{
    if (!thread || !thread->DataValid())
        return 0;

    if (chunk<0)
        chunk=thread->DataSet()->NumRows();
    wxLogInfo(wxT("retrieve %ld: did %ld of %ld"), chunk, rowsRetrieved, NumRows());

    long col, nCols=thread->DataSet()->NumCols();
    if (!rowsRetrieved)
    {
        wxString colName, colType;

        InsertColumn(0, wxT("Row"), wxLIST_FORMAT_RIGHT, 30);

        for (col=0 ; col < nCols ; col++)
        {
            colName = thread->DataSet()->ColName(col);
            colType = thread->DataSet()->ColType(col);
            InsertColumn(col+1, colName +wxT(" (")+ colType +wxT(")"), wxLIST_FORMAT_LEFT, -1);
        }
    }

    long count=0;
    long maxColSize=settings->GetMaxColSize();

    while (chunk-- && !thread->DataSet()->Eof())
    {
        InsertItem(rowsRetrieved, NumToStr(rowsRetrieved+1L));

        for (col=0 ; col < nCols ; col++)
        {
            wxString value = thread->DataSet()->GetVal(col);
            if (maxColSize > 0)
            {
                if ((int)value.Length() > maxColSize)
                    value = value.Left(maxColSize) + wxT(" (..)");
            }
            SetItem(rowsRetrieved, col+1, value);
        }
        
        thread->DataSet()->MoveNext();
        rowsRetrieved++;
        count++;
    }
    wxLogInfo(wxT("retrieve done %ld: did %ld of %ld"), count, rowsRetrieved, NumRows());

    return (count);
}


wxString ctlSQLResult::GetMessages()
{
    if (thread)
        return thread->GetMessages();
    return wxString();
}


wxString ctlSQLResult::GetErrorMessage()
{
    wxString errmsg=wxString::FromAscii(PQerrorMessage(conn->connection()));
    return errmsg;
}



long ctlSQLResult::NumRows()
{
    if (thread && thread->DataValid())
        return thread->DataSet()->NumRows();
    return 0;
}


int ctlSQLResult::RunStatus()
{
    if (!thread)
        return -1;
    
    if (thread->IsRunning())
        return CTLSQL_RUNNING;

    return thread->ReturnCode();
}


