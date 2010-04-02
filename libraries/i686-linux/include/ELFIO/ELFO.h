/*
ELFO.h - ELF reader and producer.
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

#ifndef ELFO_H
#define ELFO_H

#include <string>
#include <fstream>
#include "ELFTypes.h"

// Forward declaration
class IELFO;
class IELFOSection;
class IELFOSegment;

class IELFO
{
  public:
    virtual ~IELFO() {}

    // Section reader's types
    enum WriterType {
        ELFO_STRING,     // Strings writer
        ELFO_SYMBOL,     // Symbol table writer
        ELFO_RELOCATION, // Relocation table writer
        ELFO_NOTE,       // Notes writer
        ELFO_DYNAMIC,    // Dynamic section writer
        ELFO_HASH        // Hash 
    };

    // Construct/destroy/initialize an object
    virtual int AddRef()  = 0;
    virtual int Release() = 0;

    virtual ELFIO_Err Save( const std::string& sFileName )     = 0;

    // ELF header functions
    virtual ELFIO_Err SetAttr( unsigned char fileClass,
                               unsigned char encoding,
                               unsigned char ELFVersion,
                               Elf32_Half    type,
                               Elf32_Half    machine,
                               Elf32_Word    version,
                               Elf32_Word    flags )   = 0;
    virtual Elf32_Addr GetEntry()                const = 0;
    virtual ELFIO_Err SetEntry( Elf32_Addr entry )     = 0;
    virtual unsigned char GetEncoding() const = 0;

    // Section provider functions
    virtual Elf32_Half    GetSectionsNum()                      const = 0;
    virtual IELFOSection* GetSection( Elf32_Half index )        const = 0;
    virtual IELFOSection* GetSection( const std::string& name ) const = 0;
    virtual IELFOSection* AddSection( const std::string& name,
                                      Elf32_Word type,
                                      Elf32_Word flags,
                                      Elf32_Word info,
                                      Elf32_Word addrAlign,
                                      Elf32_Word entrySize ) = 0;
    virtual std::streampos GetSectionFileOffset( Elf32_Half index ) const = 0;

    // Segment provider functions
    virtual Elf32_Half    GetSegmentNum()                            const = 0;
    virtual IELFOSegment* AddSegment( Elf32_Word type,  Elf32_Addr vaddr,
                                      Elf32_Addr paddr, Elf32_Word flags, Elf32_Word align ) = 0;
    virtual IELFOSegment* GetSegment( Elf32_Half index )             const = 0;

    virtual ELFIO_Err CreateSectionWriter( WriterType type,
                                           IELFOSection* pSection,
                                           void** ppObj ) const = 0;
};


class IELFOSection
{
  public:
    virtual ~IELFOSection() {}

    virtual int AddRef()  = 0;
    virtual int Release() = 0;

    virtual Elf32_Half  GetIndex()     const = 0;
    virtual std::string GetName()      const = 0;
    virtual Elf32_Word  GetType()      const = 0;
    virtual Elf32_Word  GetFlags()     const = 0;
    virtual Elf32_Word  GetInfo()      const = 0;
    virtual Elf32_Word  GetAddrAlign() const = 0;
    virtual Elf32_Word  GetEntrySize() const = 0;

    virtual Elf32_Word GetNameIndex()             const = 0;
    virtual void       SetNameIndex( Elf32_Word index ) = 0;

    virtual Elf32_Addr GetAddress()            const = 0;
    virtual void       SetAddress( Elf32_Addr addr ) = 0;

    virtual Elf32_Word GetLink()            const = 0;
    virtual void       SetLink( Elf32_Word link ) = 0;

    virtual char*      GetData()                               const = 0;
    virtual Elf32_Word GetSize()                               const = 0;
    virtual ELFIO_Err  SetData( const char* pData, Elf32_Word size ) = 0;
    virtual ELFIO_Err  SetData( const std::string& data )            = 0;
    virtual ELFIO_Err  AddData( const char* pData, Elf32_Word size ) = 0;
    virtual ELFIO_Err  AddData( const std::string& data )            = 0;
    
    virtual ELFIO_Err Save( std::ofstream& f, std::streampos posHeader,
                            std::streampos posData ) = 0;
};


class IELFOSegment
{
  public:
    virtual ~IELFOSegment() {}

    virtual int AddRef()  = 0;
    virtual int Release() = 0;
    
    virtual Elf32_Word  GetType()  const = 0;
    virtual Elf32_Word  GetFlags() const = 0;
    virtual Elf32_Word  GetAlign() const = 0;

    virtual Elf32_Addr  GetVirtualAddress()  const = 0;
    virtual Elf32_Addr  GetPhysicalAddress() const = 0;
    virtual void        SetAddresses( Elf32_Addr vaddr, Elf32_Addr paddr ) = 0;

    virtual Elf32_Word  GetFileSize()        const = 0;
    virtual Elf32_Word  GetMemSize()         const = 0;

    virtual Elf32_Half AddSection( IELFOSection* pSection ) = 0;

    virtual ELFIO_Err Save( std::ofstream& f, std::streampos posHeader ) = 0;
};


// String table producer
class IELFOStringWriter
{
  public:
    virtual ~IELFOStringWriter() {}

    virtual int AddRef()  = 0;
    virtual int Release() = 0;

    virtual const char* GetString( Elf32_Word index ) const = 0;
    virtual Elf32_Word  AddString( const char* str )        = 0;
};


// Symbol table producer
class IELFOSymbolTable
{
  public:
    virtual ~IELFOSymbolTable() {}

    virtual int AddRef()  = 0;
    virtual int Release() = 0;

    virtual Elf32_Word AddEntry( Elf32_Word name, Elf32_Addr value, Elf32_Word size,
                                 unsigned char info, unsigned char other,
                                 Elf32_Half shndx ) = 0;
    virtual Elf32_Word AddEntry( Elf32_Word name, Elf32_Addr value, Elf32_Word size,
                                 unsigned char bind, unsigned char type, unsigned char other,
                                 Elf32_Half shndx ) = 0;
    virtual Elf32_Word AddEntry( IELFOStringWriter* pStrWriter, const char* str,
                                 Elf32_Addr value, Elf32_Word size,
                                 unsigned char info, unsigned char other,
                                 Elf32_Half shndx ) = 0;
    virtual Elf32_Word AddEntry( IELFOStringWriter* pStrWriter, const char* str,
                                 Elf32_Addr value, Elf32_Word size,
                                 unsigned char bind, unsigned char type, unsigned char other,
                                 Elf32_Half shndx ) = 0;
};


// Relocation table producer
class IELFORelocationTable
{
  public:
    virtual ~IELFORelocationTable() {}

    virtual int AddRef()  = 0;
    virtual int Release() = 0;

    virtual ELFIO_Err AddEntry( Elf32_Addr offset, Elf32_Word info )     = 0;
    virtual ELFIO_Err AddEntry( Elf32_Addr offset, Elf32_Word symbol,
                                unsigned char type )                     = 0;
    virtual ELFIO_Err AddEntry( Elf32_Addr offset, Elf32_Word info,
                                Elf32_Sword addend )                     = 0;
    virtual ELFIO_Err AddEntry( Elf32_Addr offset, Elf32_Word symbol,
                                unsigned char type, Elf32_Sword addend ) = 0;
    virtual ELFIO_Err AddEntry( IELFOStringWriter* pStrWriter, const char* str,
                                IELFOSymbolTable* pSymWriter,
                                Elf32_Addr value, Elf32_Word size,
                                unsigned char symInfo, unsigned char other,
                                Elf32_Half shndx,
                                Elf32_Addr offset, unsigned char type )     = 0;
    virtual ELFIO_Err AddEntry( IELFOStringWriter* pStrWriter, const char* str,
                                IELFOSymbolTable* pSymWriter,
                                Elf32_Addr value, Elf32_Word size,
                                unsigned char symInfo, unsigned char other,
                                Elf32_Half shndx,
                                Elf32_Addr offset, unsigned char type,
                                Elf32_Sword addend )                     = 0;
};


// Note section producer
class IELFONotesWriter
{
  public:
    virtual ~IELFONotesWriter() {}

    virtual int AddRef()  = 0;
    virtual int Release() = 0;

    virtual ELFIO_Err AddNote( Elf32_Word type, const std::string& name,
                               const void* desc, Elf32_Word descSize ) = 0;
};


// Dynamic section producer
class IELFODynamicWriter
{
  public:
    virtual ~IELFODynamicWriter() {}

    virtual int AddRef()  = 0;
    virtual int Release() = 0;

    virtual ELFIO_Err AddEntry( Elf32_Sword tag, Elf32_Word value ) = 0;
};

#endif // ELFO_H
