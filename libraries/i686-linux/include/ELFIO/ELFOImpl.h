/*
ELFOImpl.h - ELF reader and producer implementation classes.
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

#ifndef ELFOIMPL_H
#define ELFOIMPL_H

#include <vector>
#include <string>
#include "ELFIO.h"

class ELFOSection;
class ELFOSegment;

class ELFO : public IELFO
{
  public:
    // Construct/destroy/initialize an object
    ELFO();
    ~ELFO();
    virtual int AddRef();
    virtual int Release();

    virtual ELFIO_Err Save( const std::string& sFileName );

    // ELF header functions
    virtual ELFIO_Err SetAttr( unsigned char fileClass,
                               unsigned char encoding,
                               unsigned char ELFVersion,
                               Elf32_Half    type,
                               Elf32_Half    machine,
                               Elf32_Word    version,
                               Elf32_Word    flags );
    virtual Elf32_Addr GetEntry() const;
    virtual ELFIO_Err  SetEntry( Elf32_Addr entry );

    virtual unsigned char GetEncoding() const;

    // Section provider functions
    virtual Elf32_Half    GetSectionsNum()                            const;
    virtual IELFOSection* GetSection( Elf32_Half index )        const;
    virtual IELFOSection* GetSection( const std::string& name ) const;
    virtual IELFOSection* AddSection( const std::string& name,
                                      Elf32_Word type,
                                      Elf32_Word flags,
                                      Elf32_Word info,
                                      Elf32_Word addrAlign,
                                      Elf32_Word entrySize );
    virtual std::streampos GetSectionFileOffset( Elf32_Half index ) const;

    virtual Elf32_Half    GetSegmentNum() const;
    virtual IELFOSegment* AddSegment( Elf32_Word type,  Elf32_Addr vaddr,
                                      Elf32_Addr paddr, Elf32_Word flags, Elf32_Word align );
    virtual IELFOSegment* GetSegment( Elf32_Half index ) const;

    virtual ELFIO_Err CreateSectionWriter( WriterType type,
                                           IELFOSection* pSection,
                                           void** ppObj ) const;

  private:
    int                       m_nRefCnt;
    Elf32_Ehdr                m_header;
    std::vector<ELFOSection*> m_sections;
    std::vector<ELFOSegment*> m_segments;
};


class ELFOSection : public IELFOSection
{
  public:
    ELFOSection( Elf32_Half index,
                 IELFO*     pIELFO,
                 const std::string& name,
                 Elf32_Word type,
                 Elf32_Word flags,
                 Elf32_Word info,
                 Elf32_Word addrAlign,
                 Elf32_Word entrySize );
    ~ELFOSection();

    virtual int AddRef();
    virtual int Release();
    
    virtual Elf32_Half  GetIndex()     const;
    virtual std::string GetName()      const;
    virtual Elf32_Word  GetType()      const;
    virtual Elf32_Word  GetFlags()     const;
    virtual Elf32_Word  GetInfo()      const;
    virtual Elf32_Word  GetAddrAlign() const;
    virtual Elf32_Word  GetEntrySize() const;

    virtual Elf32_Word GetNameIndex()             const;
    virtual void       SetNameIndex( Elf32_Word index );

    virtual Elf32_Addr GetAddress() const;
    virtual void       SetAddress( Elf32_Addr addr );

    virtual Elf32_Word GetLink() const;
    virtual void       SetLink( Elf32_Word link );

    virtual char*      GetData() const;
    virtual Elf32_Word GetSize() const;
    virtual ELFIO_Err  SetData( const char* pData, Elf32_Word size );
    virtual ELFIO_Err  SetData( const std::string& data );
    virtual ELFIO_Err  AddData( const char* pData, Elf32_Word size );
    virtual ELFIO_Err  AddData( const std::string& data );

    virtual ELFIO_Err Save( std::ofstream& f, std::streampos posHeader,
                            std::streampos posData );

  private:
    Elf32_Half  m_index;
    IELFO*      m_pIELFO;
    Elf32_Shdr  m_sh;
    std::string m_name;
    char*       m_pData;
};


class ELFOSegment : public IELFOSegment
{
  public:
    ELFOSegment( IELFO* pIELFO, Elf32_Word type,  Elf32_Addr vaddr,
                 Elf32_Addr paddr, Elf32_Word flags, Elf32_Word align );

    virtual int AddRef();
    virtual int Release();
    
    virtual Elf32_Word  GetType()  const;
    virtual Elf32_Word  GetFlags() const;
    virtual Elf32_Word  GetAlign() const;

    virtual Elf32_Addr  GetVirtualAddress() const;
    virtual Elf32_Addr  GetPhysicalAddress() const;
    virtual void        SetAddresses( Elf32_Addr vaddr, Elf32_Addr paddr );

    virtual Elf32_Word  GetFileSize() const;
    virtual Elf32_Word  GetMemSize()  const;

    virtual Elf32_Half AddSection( IELFOSection* pSection );

    virtual ELFIO_Err Save( std::ofstream& f, std::streampos posHeader );
    
  private:
    IELFO*                     m_pIELFO;
    std::vector<IELFOSection*> m_sections;
    Elf32_Phdr                 m_ph;
};


// String table producer
class ELFOStringWriter : public IELFOStringWriter
{
  public:
    ELFOStringWriter( IELFO* pIELFI, IELFOSection* pSection );
    ~ELFOStringWriter();
 
    virtual int AddRef();
    virtual int Release();

    virtual const char* GetString( Elf32_Word index ) const;
    virtual Elf32_Word  AddString( const char* str );

  private:
    int           m_nRefCnt;
    IELFO*        m_pIELFO;
    IELFOSection* m_pSection;
    std::string   m_data;
};


// Symbol table producer
class ELFOSymbolTable : public IELFOSymbolTable
{
  public:
    ELFOSymbolTable( IELFO* pIELFI, IELFOSection* pSection );
    ~ELFOSymbolTable();

    virtual int AddRef();
    virtual int Release();

    virtual Elf32_Word AddEntry( Elf32_Word name, Elf32_Addr value, Elf32_Word size,
                                 unsigned char info, unsigned char other,
                                 Elf32_Half shndx );
    virtual Elf32_Word AddEntry( Elf32_Word name, Elf32_Addr value, Elf32_Word size,
                                 unsigned char bind, unsigned char type, unsigned char other,
                                 Elf32_Half shndx );
    virtual Elf32_Word AddEntry( IELFOStringWriter* pStrWriter, const char* str,
                                 Elf32_Addr value, Elf32_Word size,
                                 unsigned char info, unsigned char other,
                                 Elf32_Half shndx );
    virtual Elf32_Word AddEntry( IELFOStringWriter* pStrWriter, const char* str,
                                 Elf32_Addr value, Elf32_Word size,
                                 unsigned char bind, unsigned char type, unsigned char other,
                                 Elf32_Half shndx );

  private:
    int           m_nRefCnt;
    IELFO*        m_pIELFO;
    IELFOSection* m_pSection;
};


// Relocation table producer
class ELFORelocationTable : public IELFORelocationTable
{
  public:
    ELFORelocationTable( IELFO* pIELFI, IELFOSection* pSection );
    ~ELFORelocationTable();

    virtual int AddRef();
    virtual int Release();

    virtual ELFIO_Err AddEntry( Elf32_Addr offset, Elf32_Word info );
    virtual ELFIO_Err AddEntry( Elf32_Addr offset, Elf32_Word symbol,
                                unsigned char type );
    virtual ELFIO_Err AddEntry( Elf32_Addr offset, Elf32_Word info,
                                Elf32_Sword addend );
    virtual ELFIO_Err AddEntry( Elf32_Addr offset, Elf32_Word symbol,
                                unsigned char type, Elf32_Sword addend );
    virtual ELFIO_Err AddEntry( IELFOStringWriter* pStrWriter, const char* str,
                                IELFOSymbolTable* pSymWriter,
                                Elf32_Addr value, Elf32_Word size,
                                unsigned char symInfo, unsigned char other,
                                Elf32_Half shndx,
                                Elf32_Addr offset, unsigned char type );
    virtual ELFIO_Err AddEntry( IELFOStringWriter* pStrWriter, const char* str,
                                IELFOSymbolTable* pSymWriter,
                                Elf32_Addr value, Elf32_Word size,
                                unsigned char symInfo, unsigned char other,
                                Elf32_Half shndx,
                                Elf32_Addr offset, unsigned char type,
                                Elf32_Sword addend );

  private:
    int           m_nRefCnt;
    IELFO*        m_pIELFO;
    IELFOSection* m_pSection;
};


// Note section producer
class ELFONotesWriter : public IELFONotesWriter
{
  public:
    ELFONotesWriter( IELFO* pIELFI, IELFOSection* pSection );
    ~ELFONotesWriter();

    virtual int AddRef();
    virtual int Release();

    virtual ELFIO_Err AddNote( Elf32_Word type, const std::string& name,
                               const void* desc, Elf32_Word descSize );

  private:
    int           m_nRefCnt;
    IELFO*        m_pIELFO;
    IELFOSection* m_pSection;
};


// Dynamic section producer
class ELFODynamicWriter : public IELFODynamicWriter
{
  public:
    ELFODynamicWriter( IELFO* pIELFO, IELFOSection* pSection );
    ~ELFODynamicWriter();
  
    virtual int AddRef();
    virtual int Release();

    virtual ELFIO_Err AddEntry( Elf32_Sword tag, Elf32_Word value );

  private:
    int           m_nRefCnt;
    IELFO*        m_pIELFO;
    IELFOSection* m_pSection;
};

#endif // ELFOIMPL_H
