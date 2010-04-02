/*
ELFI.h - ELF reader and producer.
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

#ifndef ELFI_H
#define ELFI_H

#include <string>
#include "ELFTypes.h"

// Forward declaration
class IELFI;
class IELFISection;
class IELFIStringReader;
class IELFISymbolTable;
class IELFIRelocationTable;
class IELFINoteReader;
class IELFIDynamicReader;
class IELFISegment;


// ELF file reader interface. This class gives an access to properties
// of a ELF file. Also you may get sections of the file or create
// section's readers.
class IELFI
{
  public:
    virtual ~IELFI(){}

    // Section reader's types
    enum ReaderType {
        ELFI_STRING,     // Strings reader
        ELFI_SYMBOL,     // Symbol table reader
        ELFI_RELOCATION, // Relocation table reader
        ELFI_NOTE,       // Notes reader
        ELFI_DYNAMIC,    // Dynamic section reader
        ELFI_HASH        // Hash
    };

    // Construct/destroy/initialize an object
    virtual ELFIO_Err Load( const std::string& sFileName )        = 0;
    virtual ELFIO_Err Load( std::istream* pStream, int startPos ) = 0;
    virtual bool      IsInitialized() const                       = 0;
    virtual int       AddRef()        const                       = 0;
    virtual int       Release()       const                       = 0;

    // ELF header functions
    virtual unsigned char GetClass()      const = 0;
    virtual unsigned char GetEncoding()   const = 0;
    virtual unsigned char GetELFVersion() const = 0;
    virtual Elf32_Half    GetType()       const = 0;
    virtual Elf32_Half    GetMachine()    const = 0;
    virtual Elf32_Word    GetVersion()    const = 0;
    virtual Elf32_Addr    GetEntry()      const = 0;
    virtual Elf32_Word    GetFlags()      const = 0;
    virtual Elf32_Half    GetSecStrNdx()  const = 0;

    // Section provider functions
    virtual       Elf32_Half    GetSectionsNum()                      const = 0;
    virtual const IELFISection* GetSection( Elf32_Half index )        const = 0;
    virtual const IELFISection* GetSection( const std::string& name ) const = 0;

    // Segment provider functions
    virtual       Elf32_Half    GetSegmentsNum()               const = 0;
    virtual const IELFISegment* GetSegment( Elf32_Half index ) const = 0;

    // Section readers' builder
    virtual ELFIO_Err CreateSectionReader( ReaderType type, const IELFISection* pSection,
                                           void** ppObj ) const = 0;
};


// The class represents an ELF file section
class IELFISection
{
  public:
    virtual ~IELFISection() = 0;

    virtual int AddRef()  const = 0;
    virtual int Release() const = 0;

    // Section info functions
    virtual Elf32_Half  GetIndex()     const = 0;
    virtual std::string GetName()      const = 0;
    virtual Elf32_Word  GetType()      const = 0;
    virtual Elf32_Word  GetFlags()     const = 0;
    virtual Elf32_Addr  GetAddress()   const = 0;
    virtual Elf32_Word  GetSize()      const = 0;
    virtual Elf32_Word  GetLink()      const = 0;
    virtual Elf32_Word  GetInfo()      const = 0;
    virtual Elf32_Word  GetAddrAlign() const = 0;
    virtual Elf32_Word  GetEntrySize() const = 0;
    virtual const char* GetData()      const = 0;
};


// Program segment
class IELFISegment
{
  public:
    virtual ~IELFISegment() = 0;

    virtual int AddRef()  const = 0;
    virtual int Release() const = 0;

    // Section info functions
    virtual Elf32_Word  GetType()            const = 0;
    virtual Elf32_Addr  GetVirtualAddress()  const = 0;
    virtual Elf32_Addr  GetPhysicalAddress() const = 0;
    virtual Elf32_Word  GetFileSize()        const = 0;
    virtual Elf32_Word  GetMemSize()         const = 0;
    virtual Elf32_Word  GetFlags()           const = 0;
    virtual Elf32_Word  GetAlign()           const = 0;
    virtual const char* GetData()            const = 0;
};


// String table reader
class IELFIStringReader : virtual public IELFISection
{
  public:
    virtual const char* GetString( Elf32_Word index ) const = 0;
};


// Symbol table reader
class IELFISymbolTable : virtual public IELFISection
{
  public:
    virtual Elf32_Half GetStringTableIndex() const = 0;
    virtual Elf32_Half GetHashTableIndex()   const = 0;

    virtual Elf32_Word GetSymbolNum() const = 0;
    virtual ELFIO_Err  GetSymbol( Elf32_Word index,
                                  std::string& name, Elf32_Addr& value,
                                  Elf32_Word& size,
                                  unsigned char& bind, unsigned char& type,
                                  Elf32_Half& section ) const = 0;
    virtual ELFIO_Err  GetSymbol( const std::string& name, Elf32_Addr& value,
                                  Elf32_Word& size,
                                  unsigned char& bind, unsigned char& type,
                                  Elf32_Half& section ) const = 0;
};


// Relocation table reader
class IELFIRelocationTable : virtual public IELFISection
{
  public:
    virtual Elf32_Half GetSymbolTableIndex()   const = 0;
    virtual Elf32_Half GetTargetSectionIndex() const = 0;

    virtual Elf32_Word GetEntriesNum()                      const = 0;
    virtual ELFIO_Err  GetEntry( Elf32_Word     index,
                                 Elf32_Addr&    offset,
                                 Elf32_Word&    symbol,
                                 unsigned char& type,
                                 Elf32_Sword&   addend )    const = 0;
    virtual ELFIO_Err  GetEntry( Elf32_Word     index,
                                 Elf32_Addr&    offset,
                                 Elf32_Addr&    symbolValue,
                                 std::string&   symbolName,
                                 unsigned char& type,
                                 Elf32_Sword&   addend,
                                 Elf32_Sword&   calcValue ) const = 0;
};


class IELFINoteReader : virtual public IELFISection
{
  public:
    // Notes reader functions
    virtual Elf32_Word GetNotesNum()                   const = 0;
    virtual ELFIO_Err  GetNote( Elf32_Word   index,
                                Elf32_Word&  type,
                                std::string& name,
                                void*& desc,
                                Elf32_Word& descSize ) const = 0;
};


class IELFIDynamicReader : virtual public IELFISection
{
  public:
    // Notes reader functions
    virtual Elf32_Word GetEntriesNum()                const = 0;
    virtual ELFIO_Err  GetEntry( Elf32_Word   index,
                                 Elf32_Sword& tag,
                                 Elf32_Word&  value ) const = 0;
};

#endif // ELFI_H
