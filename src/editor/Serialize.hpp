/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#pragma once

#include "wx/filename.h"
#include "wx/mstream.h"
#include "wx/string.h"
#include "wx/wfstream.h"
#include "wx/zipstrm.h"

#include "irrlicht.h"

#include <map>
#include <memory>

class Map;
class Serializer;

class ISerializerFactory
{
protected:
	typedef std::map<wxString, std::shared_ptr<ISerializerFactory>> serializers_t;
	static serializers_t ms_List;

public:
	static void AddSerializer(const wxString& extension, std::shared_ptr<ISerializerFactory> serializer);
	static std::shared_ptr<Serializer> GetSave(const wxFileName& fileName);
	static std::shared_ptr<Serializer> GetLoad(const wxFileName& fileName);
	static wxString BuildFilter(void);

public:
	virtual std::shared_ptr<Serializer> Save(const wxFileName& fileName) = 0;
	virtual std::shared_ptr<Serializer> Load(const wxFileName& fileName) = 0;
	virtual const wxString& FilterString(void) = 0;
};

template <class _save, class _load> class SerializerFactory : public ISerializerFactory
{
private:
	wxString m_FilterString;

public:
	SerializerFactory(const wxString& filterString)
		: m_FilterString(filterString) {}

	std::shared_ptr<Serializer> Save(const wxFileName& fileName)
	{ 
		return std::shared_ptr<Serializer>(new _save(fileName));
	}

	std::shared_ptr<Serializer> Load(const wxFileName& fileName)
	{
		return std::shared_ptr<Serializer>(new _load(fileName));
	}

	const wxString& FilterString(void) { return m_FilterString; }
};

class Serializer
{
public:
	enum CONTENT_TYPE
	{
		CONTENT_UNKNOWN,
		CONTENT_MAP,
		CONTENT_PACKAGE
	};

protected:
	wxFileName m_FileName;
	irr::video::IVideoDriver* m_VideoDriver;
	irr::io::IFileSystem* m_FileSystem;

public:
	Serializer(const wxFileName& fileName);
	virtual ~Serializer(void);

	void SetVideoDriver(irr::video::IVideoDriver* videoDriver);
	void SetFileSystem(irr::io::IFileSystem* fileSystem);

	virtual CONTENT_TYPE Verify(void) { return CONTENT_UNKNOWN; }

	virtual bool Begin(wxInt32& nextId) = 0;
	virtual bool Next(irr::core::stringc& type, irr::io::IAttributes* attributes,
		irr::core::array<irr::io::IAttributes*>& materials, irr::core::array<irr::io::IAttributes*>& animators, 
		irr::io::IAttributes* userData, bool& child) = 0;
	virtual void Finalize(void) = 0;
};

// process .irr XML files
class IrrSave : public Serializer
{
protected:
	irr::io::IXMLWriter* m_OutXml;
	irr::u32 m_Depth;

public:
	IrrSave(const wxFileName& fileName);
	virtual ~IrrSave(void);

	virtual bool Begin(wxInt32& nextId);
	virtual bool Next(irr::core::stringc& type, irr::io::IAttributes* attributes,
		irr::core::array<irr::io::IAttributes*>& materials, irr::core::array<irr::io::IAttributes*>& animators, 
		irr::io::IAttributes* userData, bool& child);
	virtual void Finalize(void);
};

class IrrLoad : public Serializer
{
protected:
	irr::io::IXMLReader* m_InXml;

public:
	IrrLoad(const wxFileName& fileName);
	virtual ~IrrLoad(void);

	virtual CONTENT_TYPE Verify(void);

	virtual bool Begin(wxInt32& nextId);
	virtual bool Next(irr::core::stringc& type, irr::io::IAttributes* attributes,
		irr::core::array<irr::io::IAttributes*>& materials, irr::core::array<irr::io::IAttributes*>& animators,
		irr::io::IAttributes* userData, bool& child);
	virtual void Finalize(void);
};

// process .mmp archive files
// it uses an .irr file under the hood
class MmpSave : public IrrSave
{
protected:
	irr::io::IWriteFile* m_WriteFile;

	wxTempFileOutputStream m_OutFile;
	wxZipOutputStream m_OutStream;

public:
	MmpSave(const wxFileName& fileName);
	virtual ~MmpSave(void);

	virtual bool Begin(wxInt32& nextId);
	virtual bool Next(irr::core::stringc& type, irr::io::IAttributes* attributes,
		irr::core::array<irr::io::IAttributes*>& materials, irr::core::array<irr::io::IAttributes*>& animators,
		irr::io::IAttributes* userData, bool& child);
	virtual void Finalize(void);

protected:
	bool AddFile(const wxFileName& source, const wxString& dest);
};

class MmpLoad : public IrrLoad
{
protected:
	irr::io::IReadFile* m_ReadFile;

	wxFileInputStream m_InFile;
	wxZipInputStream m_InStream;

public:
	MmpLoad(const wxFileName& fileName);
	virtual ~MmpLoad(void);

	virtual CONTENT_TYPE Verify(void);

	virtual bool Begin(wxInt32& nextId);
	virtual bool Next(irr::core::stringc& type, irr::io::IAttributes* attributes,
		irr::core::array<irr::io::IAttributes*>& materials, irr::core::array<irr::io::IAttributes*>& animators,
		irr::io::IAttributes* userData, bool& child);
	virtual void Finalize(void);
};
