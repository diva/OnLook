/*
ELFIO.h - ELF reader and producer.
Copyright (C) 2001 Serge Lamikhov-Center <to_serge@users.sourceforge.net>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef ELFIO_H
#define ELFIO_H

#include <string>
#include <fstream>
#include "ELFTypes.h"

// ELFIO error codes
enum ELFIO_Err {
    ERR_ELFIO_NO_ERROR,         // No error
    ERR_ELFIO_INITIALIZED,      // The ELFIO object was initialized
    ERR_ELFIO_MEMORY,           // Out of memory
    ERR_ELFIO_CANT_OPEN,        // Can't open a specified file
    ERR_ELFIO_NOT_ELF,          // The file is not a valid ELF file
    ERR_NO_SUCH_READER,         // There is no such reader
    ERR_ELFIO_SYMBOL_ERROR,     // Symbol section reader error
    ERR_ELFIO_RELOCATION_ERROR, // Relocation section reader error
    ERR_ELFIO_INDEX_ERROR       // Index is out of range
};


#include "ELFI.h"
#include "ELFO.h"


// This class builds two main objects: ELF file reader (ELFI)
// and producer (ELFO)
class ELFIO
{
  public:
    virtual ~ELFIO() {}

    static const ELFIO* GetInstance();

    ELFIO_Err CreateELFI( IELFI** ppObj ) const;
    ELFIO_Err CreateELFO( IELFO** ppObj ) const;

    virtual std::string GetErrorText( ELFIO_Err err ) const;
    
  private:
    ELFIO();
    ELFIO( const ELFIO& );
};

#endif // ELFIO_H
