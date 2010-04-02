/*
ELFIOUtils.h - Utility functions.
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


#ifndef ELFIOUTILS_H
#define ELFIOUTILS_H

#include "ELFTypes.h"

Elf32_Addr  Convert32Addr2Host ( Elf32_Addr addr,  unsigned char encoding );
Elf32_Half  Convert32Half2Host ( Elf32_Half half,  unsigned char encoding );
Elf32_Off   Convert32Off2Host  ( Elf32_Off  off,   unsigned char encoding );
Elf32_Sword Convert32Sword2Host( Elf32_Sword word, unsigned char encoding );
Elf32_Word  Convert32Word2Host ( Elf32_Word word,  unsigned char encoding );

Elf32_Word ElfHashFunc( const unsigned char* name );

#endif // ELFIOUTILS_H
