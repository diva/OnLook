/*
ELFIImpl.h - ELF reader and producer implementation classes.
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

#ifndef ELFIIMPL_H
#define ELFIIMPL_H

#include <iostream>
#include <vector>
#include "ELFIO.h"

class ELFI : public IELFI
{
  public:
    ELFI();
    virtual ~ELFI();

    // Creation / state / destroy functions
    virtual ELFIO_Err Load( const std::string& sFileName );
    virtual ELFIO_Err Load( std::istream* pStream, int startPos );
    virtual bool      IsInitialized() const;
    virtual int       AddRef()        const;
    virtual int       Release()       const;

    // ELF header functions
    virtual unsigned char GetClass()      const;
    virtual unsigned char GetEncoding()   const;
    virtual unsigned char GetELFVersion() const;
    virtual Elf32_Half    GetType()       const;
    virtual Elf32_Half    GetMachine()    const;
    virtual Elf32_Word    GetVersion()    const;
    virtual Elf32_Addr    GetEntry()      const;
    virtual Elf32_Word    GetFlags()      const;
    virtual Elf32_Half    GetSecStrNdx()  const;

    // Section provider functions
    virtual       Elf32_Half    GetSectionsNum()                      const;
    virtual const IELFISection* GetSection( Elf32_Half index )        const;
    virtual const IELFISection* GetSection( const std::string& name ) const;

    // Segment provider functions
    virtual       Elf32_Half    GetSegmentsNum()               const;
    virtual const IELFISegment* GetSegment( Elf32_Half index ) const;

    // Section's readers
    virtual ELFIO_Err CreateSectionReader( ReaderType type, const IELFISection* pSection,
                                           void** ppObj ) const;

  private:
    ELFIO_Err LoadSections();
    ELFIO_Err LoadSegments();

  private:
    mutable int                      m_nRefCnt;
    std::istream*                    m_pStream;
    int                              m_nFileOffset;
    bool                             m_bOwnStream;
    bool                             m_bInitialized;
    Elf32_Ehdr                       m_header;
    std::vector<const IELFISection*> m_sections;
    std::vector<const IELFISegment*> m_segments;
};


class ELFISection : public IELFISection
{
  public:
    ELFISection( IELFI* pIELFI, std::istream* pStream, int nFileOffset,
                 Elf32_Shdr* pHeader, Elf32_Half index );
    virtual ~ELFISection();

    virtual int AddRef()  const;
    virtual int Release() const;

    // Section info functions
    virtual Elf32_Half  GetIndex()     const;
    virtual std::string GetName()      const;
    virtual Elf32_Word  GetType()      const;
    virtual Elf32_Word  GetFlags()     const;
    virtual Elf32_Addr  GetAddress()   const;
    virtual Elf32_Word  GetSize()      const;
    virtual Elf32_Word  GetLink()      const;
    virtual Elf32_Word  GetInfo()      const;
    virtual Elf32_Word  GetAddrAlign() const;
    virtual Elf32_Word  GetEntrySize() const;
    virtual const char* GetData()      const;

  private:
    Elf32_Half     m_index;
    mutable int    m_nRefCnt;
    IELFI*         m_pIELFI;
    std::istream*  m_pStream;
    int            m_nFileOffset;
    Elf32_Shdr     m_sh;
    mutable char*  m_data;
};


class ELFISegment : public IELFISegment
{
  public:
    ELFISegment( IELFI* pIELFI, std::istream* pStream, int nFileOffset,
                 Elf32_Phdr* pHeader, Elf32_Half index );
    virtual ~ELFISegment();

    virtual int AddRef()  const;
    virtual int Release() const;

    // Section info functions
    virtual Elf32_Word  GetType()            const;
    virtual Elf32_Addr  GetVirtualAddress()  const;
    virtual Elf32_Addr  GetPhysicalAddress() const;
    virtual Elf32_Word  GetFileSize()        const;
    virtual Elf32_Word  GetMemSize()         const;
    virtual Elf32_Word  GetFlags()           const;
    virtual Elf32_Word  GetAlign()           const;
    virtual const char* GetData()            const;

  private:
    Elf32_Half     m_index;
    mutable int    m_nRefCnt;
    IELFI*         m_pIELFI;
    std::istream*  m_pStream;
    int            m_nFileOffset;
    Elf32_Phdr     m_sh;
    mutable char*  m_data;
};


class ELFIReaderImpl : virtual public IELFISection
{
  public:
    ELFIReaderImpl( const IELFI* pIELFI, const IELFISection* pSection );
    virtual ~ELFIReaderImpl();

    virtual int AddRef()  const;
    virtual int Release() const;

    // Section info functions
    virtual Elf32_Half  GetIndex()     const;
    virtual std::string GetName()      const;
    virtual Elf32_Word  GetType()      const;
    virtual Elf32_Word  GetFlags()     const;
    virtual Elf32_Addr  GetAddress()   const;
    virtual Elf32_Word  GetSize()      const;
    virtual Elf32_Word  GetLink()      const;
    virtual Elf32_Word  GetInfo()      const;
    virtual Elf32_Word  GetAddrAlign() const;
    virtual Elf32_Word  GetEntrySize() const;
    virtual const char* GetData()      const;

  protected:
    mutable int         m_nRefCnt;
    const IELFI*        m_pIELFI;
    const IELFISection* m_pSection;
};


class ELFIStringReader : public ELFIReaderImpl, public IELFIStringReader
{
  public:
    ELFIStringReader( const IELFI* pIELFI, const IELFISection* pSection );
    virtual ~ELFIStringReader();

    // IELFIStringReader implementation
    virtual const char* GetString( Elf32_Word index ) const;
};


class ELFISymbolTable : public ELFIReaderImpl, public IELFISymbolTable
{
  public:
    ELFISymbolTable( const IELFI* pIELFI, const IELFISection* pSection );
    virtual ~ELFISymbolTable();

    virtual int AddRef()  const;
    virtual int Release() const;

    virtual Elf32_Half GetStringTableIndex() const;
    virtual Elf32_Half GetHashTableIndex()   const;

    virtual Elf32_Word GetSymbolNum() const;
    virtual ELFIO_Err  GetSymbol( Elf32_Word index,
                                  std::string& name, Elf32_Addr& value,
                                  Elf32_Word& size,
                                  unsigned char& bind, unsigned char& type,
                                  Elf32_Half& section ) const;
    virtual ELFIO_Err  GetSymbol( const std::string& name, Elf32_Addr& value,
                                  Elf32_Word& size,
                                  unsigned char& bind, unsigned char& type,
                                  Elf32_Half& section ) const;

  private:
    const IELFIStringReader* m_pStrReader;
    Elf32_Half               m_nHashSection;
    const IELFISection*      m_pHashSection;
};


class ELFIRelocationTable : public ELFIReaderImpl, public IELFIRelocationTable
{
  public:
    ELFIRelocationTable( const IELFI* pIELFI, const IELFISection* pSection );
    virtual ~ELFIRelocationTable();

    virtual int AddRef()  const;
    virtual int Release() const;

    virtual Elf32_Half GetSymbolTableIndex()   const;
    virtual Elf32_Half GetTargetSectionIndex() const;

    virtual Elf32_Word GetEntriesNum() const;
    virtual ELFIO_Err  GetEntry( Elf32_Word     index,
                                 Elf32_Addr&    offset,
                                 Elf32_Word&    symbol,
                                 unsigned char& type,
                                 Elf32_Sword&   addend ) const;
    virtual ELFIO_Err  GetEntry( Elf32_Word     index,
                                 Elf32_Addr&    offset,
                                 Elf32_Addr&    symbolValue,
                                 std::string&   symbolName,
                                 unsigned char& type,
                                 Elf32_Sword&   addend,
                                 Elf32_Sword&   calcValue ) const;

  private:
    const IELFISymbolTable* m_pSymTbl;
};


class ELFINoteReader : public ELFIReaderImpl, public IELFINoteReader
{
  public:
    ELFINoteReader( const IELFI* pIELFI, const IELFISection* pSection );
    virtual ~ELFINoteReader();

    // Notes reader functions
    virtual Elf32_Word GetNotesNum() const;
    virtual ELFIO_Err  GetNote( Elf32_Word   index,
                                Elf32_Word&  type,
                                std::string& name,
                                void*& desc,
                                Elf32_Word& descSize ) const;

  private:
    void ProcessSection();

  private:
    std::vector<Elf32_Word> m_beginPtrs;
};


class ELFIDynamicReader : public ELFIReaderImpl, public IELFIDynamicReader
{
  public:
    ELFIDynamicReader( const IELFI* pIELFI, const IELFISection* pSection );
    virtual ~ELFIDynamicReader();

    // Dynamic reader functions
    virtual Elf32_Word GetEntriesNum() const;
    virtual ELFIO_Err  GetEntry( Elf32_Word   index,
                                 Elf32_Sword& tag,
                                 Elf32_Word&  value ) const;
};

#endif // ELFIIMPL_H
