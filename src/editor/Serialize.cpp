/*
* ManifoldEditor
*
* Copyright (c) 2023 James Kinnaird
*/

#include "Common.hpp"
#include "Serialize.hpp"

#include <wx/log.h>
#include <wx/mstream.h>
#include <wx/xml/xml.h>

ISerializerFactory::serializers_t ISerializerFactory::ms_List;

void ISerializerFactory::AddSerializer(const wxString& extension,
	std::shared_ptr<ISerializerFactory> serializer)
{
	ms_List.emplace(extension, serializer);
}

std::shared_ptr<Serializer> ISerializerFactory::GetSave(const wxFileName& fileName)
{
	return std::shared_ptr<Serializer>(ms_List[fileName.GetExt()]->Save(fileName));
}

std::shared_ptr<Serializer> ISerializerFactory::GetLoad(const wxFileName& fileName)
{
	return std::shared_ptr<Serializer>(ms_List[fileName.GetExt()]->Load(fileName));
}

wxString ISerializerFactory::BuildFilter(void)
{
	wxString filter;

	for (serializers_t::iterator i = ms_List.begin();
		i != ms_List.end(); ++i)
	{
		if (!filter.empty())
			filter.append(wxT("|"));

		filter.append((*i).second->FilterString());
	}

	return filter;
}

Serializer::Serializer(const wxFileName& fileName)
	: m_FileName(fileName), m_VideoDriver(nullptr), m_FileSystem(nullptr)
{
}

Serializer::~Serializer(void)
{
}

void Serializer::SetVideoDriver(irr::video::IVideoDriver* videoDriver)
{
	m_VideoDriver = videoDriver;
}

void Serializer::SetFileSystem(irr::io::IFileSystem* fileSystem)
{
	m_FileSystem = fileSystem;
}

IrrSave::IrrSave(const wxFileName& fileName)
	: Serializer(fileName)
{
	m_OutXml = nullptr;
	m_Depth = 0;
}

IrrSave::~IrrSave(void)
{
}

bool IrrSave::Begin(wxInt32& nextId)
{
	// get things ready to save
	std::string path(m_FileName.GetFullPath().utf8_string());
	irr::io::path filePath(path.c_str());

	m_OutXml = m_FileSystem->createXMLWriter(filePath);
	if (m_OutXml == nullptr)
		return false;

	// write the base details
	m_OutXml->writeXMLHeader();
	m_OutXml->writeElement(L"irr_scene", false,
		L"editor", wxT(APP_NAME),
		L"version", wxT(APP_VERSION),
		L"nextId", wxString::Format(wxT("%d"), nextId).wc_str());
	m_OutXml->writeLineBreak();

	// @TODO: set these values, maybe
	m_OutXml->writeElement(L"attributes");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"string", true, L"name", L"name", L"value", L"");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"int", true, L"name", L"Id", L"value", L"-1");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"colorf", true, L"name", L"AmbientLight", L"value", L"0.000000, 0.000000, 0.000000, 0.000000");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"enum", true, L"name", L"FogType", L"value", L"FogLinear");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"colorf", true, L"name", L"FogColor", L"value", L"1.000000, 1.000000, 1.000000, 0.000000");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"float", true, L"name", L"FogStart", L"value", L"50.000000");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"float", true, L"name", L"FogEnd", L"value", L"100.000000");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"float", true, L"name", L"FogDensity", L"value", L"0.010000");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"bool", true, L"name", L"FogPixel", L"value", L"false");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"bool", true, L"name", L"FogRange", L"value", L"false");
	m_OutXml->writeLineBreak();
	m_OutXml->writeClosingTag(L"attributes");
	m_OutXml->writeLineBreak();
	m_OutXml->writeLineBreak();

	return true;
}

bool IrrSave::Next(irr::core::stringc& type, irr::io::IAttributes* attributes,
	irr::core::array<irr::io::IAttributes*>& materials, irr::core::array<irr::io::IAttributes*>& animators,
	irr::io::IAttributes* userData, bool& child)
{
	m_OutXml->writeElement(L"node", false, L"type", irr::core::stringw(type).c_str());
	m_OutXml->writeLineBreak();

	if (attributes)
	{
		attributes->write(m_OutXml, false, L"attributes");
		attributes->drop();
	}

	m_OutXml->writeElement(L"materials", false);
	m_OutXml->writeLineBreak();

	for (irr::u32 i = 0; i < materials.size(); ++i)
	{
		materials[i]->write(m_OutXml, false, L"attributes");
		materials[i]->drop();
	}
	materials.clear();

	m_OutXml->writeClosingTag(L"materials");
	m_OutXml->writeLineBreak();

	m_OutXml->writeElement(L"animators", false);
	m_OutXml->writeLineBreak();

	for (irr::u32 i = 0; i < animators.size(); ++i)
	{
		animators[i]->write(m_OutXml, false, L"attributes");
		animators[i]->drop();
	}
	animators.clear();

	m_OutXml->writeClosingTag(L"animators");
	m_OutXml->writeLineBreak();

	m_OutXml->writeElement(L"userData", false);
	m_OutXml->writeLineBreak();
	if (userData)
	{
		userData->write(m_OutXml, false, L"attributes");
		userData->drop();
	}
	m_OutXml->writeClosingTag(L"userData");
	m_OutXml->writeLineBreak();

	if (child)
		++m_Depth;
	else
	{
		m_OutXml->writeClosingTag(L"node");
		if (m_Depth > 0)
			--m_Depth;
	}

	m_OutXml->writeLineBreak();
	return true;
}

void IrrSave::Finalize(void)
{
	if (m_OutXml)
	{
		while (m_Depth > 0)
		{
			m_OutXml->writeClosingTag(L"node");
			m_OutXml->writeLineBreak();
			--m_Depth;
		}

		// close the document
		m_OutXml->writeClosingTag(L"irr_scene");
		m_OutXml->writeLineBreak();
		m_OutXml->drop();
		m_OutXml = nullptr;
	}
}

IrrLoad::IrrLoad(const wxFileName& fileName)
	: Serializer(fileName)
{
	m_InXml = nullptr;
}

IrrLoad::~IrrLoad(void)
{
}

Serializer::CONTENT_TYPE IrrLoad::Verify(void)
{
	// load the file as XML and make sure the root element is 'irr_scene'
	wxXmlDocument doc;
	if (!doc.Load(m_FileName.GetFullPath()))
		return CONTENT_UNKNOWN;

	if (doc.GetRoot()->GetName().compare("irr_scene") != 0)
		return CONTENT_UNKNOWN;

	return CONTENT_MAP;
}

bool IrrLoad::Begin(wxInt32& nextId)
{
	std::string path(m_FileName.GetFullPath().utf8_string());
	irr::io::path filePath(path.c_str());

	m_InXml = m_FileSystem->createXMLReader(filePath);
	if (m_InXml == nullptr)
		return false;

	// process the document for base properties
	while (m_InXml->read())
	{
		irr::core::stringw name(m_InXml->getNodeName());
		if (m_InXml->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if (name == L"irr_scene")
			{
				nextId = m_InXml->getAttributeValueAsInt(L"nextId");

				// read the scene attributes
				if (m_InXml->read() && m_InXml->getNodeType() == irr::io::EXN_ELEMENT)
				{
					name = m_InXml->getNodeName();
					if (name == L"attributes")
					{
						irr::io::IAttributes* attribs = m_FileSystem->createEmptyAttributes(
							m_VideoDriver);
						attribs->read(m_InXml, true);

						// @TODO: process the scene attributes

						attribs->drop();
					}
				}

				break;
			}
		}
	}

	return true;
}

bool IrrLoad::Next(irr::core::stringc& type, irr::io::IAttributes* attributes,
	irr::core::array<irr::io::IAttributes*>& materials, irr::core::array<irr::io::IAttributes*>& animators,
	irr::io::IAttributes* userData, bool& child)
{
	bool started = false;
	while (m_InXml->read())
	{
		irr::core::stringw name(m_InXml->getNodeName());
		switch (m_InXml->getNodeType())
		{
		case irr::io::EXN_ELEMENT:
		{
			if (name == L"node")
			{
				started = true;
				type = m_InXml->getAttributeValue(L"type");
			}
			else if (name == L"attributes")
			{
				if (started)
					attributes->read(m_InXml, true);
			}
			else if (name == L"materials")
			{
				// read any materials
				while (m_InXml->read())
				{
					name = m_InXml->getNodeName();
					if (m_InXml->getNodeType() == irr::io::EXN_ELEMENT &&
						name == L"attributes")
					{
						irr::io::IAttributes* material = m_FileSystem->createEmptyAttributes(m_VideoDriver);
						material->read(m_InXml, true);
						materials.push_back(material);
					}
					else if (m_InXml->getNodeType() == irr::io::EXN_ELEMENT_END &&
						name == L"materials")
					{
						break;
					}
				}
			}
			else if (name == L"animators")
			{
				// read any animators
				while (m_InXml->read())
				{
					name = m_InXml->getNodeName();
					if (m_InXml->getNodeType() == irr::io::EXN_ELEMENT &&
						name == L"attributes")
					{
						irr::io::IAttributes* animator = m_FileSystem->createEmptyAttributes(m_VideoDriver);
						animator->read(m_InXml, true);
						animators.push_back(animator);
					}
					else if (m_InXml->getNodeType() == irr::io::EXN_ELEMENT_END &&
						name == L"animators")
					{
						break;
					}
				}
			}
			else if (name == L"userData")
			{
				if (started)
					userData->read(m_InXml);
			}
		} break;
		case irr::io::EXN_ELEMENT_END:
		{
			if (name == L"node")
			{
				started = false;
				return true; // done reading this node
			}
		} break;
		}
	}

	return false;
}

void IrrLoad::Finalize(void)
{
	if (m_InXml)
	{
		m_InXml->drop();
		m_InXml = nullptr;
	}
}

class StreamWriteFile : public irr::io::IWriteFile
{
private:
	irr::io::path m_FileName;
	wxMemoryOutputStream m_Stream;

public:
	StreamWriteFile(const wxString& fileName)
		: m_FileName(fileName.ToStdString().c_str()) {}
	~StreamWriteFile(void) {}

	wxMemoryOutputStream& GetStream(void) { return m_Stream; }

	const irr::io::path& getFileName(void) const
	{
		return m_FileName;
	}

	long getPos(void) const { return m_Stream.TellO(); }
	bool seek(long finalPos, bool relativeMovement = false)
	{
		return m_Stream.SeekO(finalPos,
			relativeMovement ? wxFromCurrent : wxFromStart) != wxInvalidOffset;
	}

	irr::s32 write(const void* buffer, irr::u32 sizeToWrite)
	{
		m_Stream.Write(buffer, sizeToWrite);
		return m_Stream.LastWrite();
	}
};

class StreamReadFile : public irr::io::IReadFile
{
private:
	irr::io::path m_FileName;
	wxMemoryInputStream m_Stream;

public:
	StreamReadFile(const wxString& fileName,
		wxInputStream& inputStream)
		: m_FileName(fileName.ToStdString().c_str()),
		  m_Stream(inputStream) {}
	~StreamReadFile(void) {}

	const irr::io::path& getFileName(void) const
	{
		return m_FileName;
	}

	long getPos(void) const { return m_Stream.TellI(); }
	long getSize(void) const { return m_Stream.GetSize(); }

	bool seek(long finalPos, bool relativeMovement = false)
	{
		return m_Stream.SeekI(finalPos,
			relativeMovement ? wxFromCurrent : wxFromStart) != wxInvalidOffset;
	}

	irr::s32 read(void* buffer, irr::u32 sizeToWrite)
	{
		m_Stream.Read(buffer, sizeToWrite);
		return m_Stream.LastRead();
	}
};

MmpSave::MmpSave(const wxFileName& fileName)
	: IrrSave(fileName), m_OutFile(fileName.GetFullPath()),
	  m_OutStream(m_OutFile)
{
	m_WriteFile = nullptr;
}

MmpSave::~MmpSave(void)
{
	m_OutStream.Close();
}

bool MmpSave::Begin(wxInt32& nextId)
{
	if (!m_OutStream.IsOk())
		return false;

	wxFileName mapName(m_FileName);
	mapName.SetExt(wxT("irr"));

	m_WriteFile = new StreamWriteFile(mapName.GetFullName());
	m_OutXml = m_FileSystem->createXMLWriter(m_WriteFile);
	if (m_OutXml == nullptr)
	{
		m_WriteFile->drop();
		return false;
	}

	// write the base details
	m_OutXml->writeXMLHeader();
	m_OutXml->writeElement(L"irr_scene", false,
		L"editor", wxT(APP_NAME),
		L"version", wxT(APP_VERSION),
		L"nextId", wxString::Format(wxT("%d"), nextId).wc_str());
	m_OutXml->writeLineBreak();

	// @TODO: set these values, maybe
	m_OutXml->writeElement(L"attributes");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"string", true, L"name", L"name", L"value", L"");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"int", true, L"name", L"Id", L"value", L"-1");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"colorf", true, L"name", L"AmbientLight", L"value", L"0.000000, 0.000000, 0.000000, 0.000000");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"enum", true, L"name", L"FogType", L"value", L"FogLinear");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"colorf", true, L"name", L"FogColor", L"value", L"1.000000, 1.000000, 1.000000, 0.000000");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"float", true, L"name", L"FogStart", L"value", L"50.000000");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"float", true, L"name", L"FogEnd", L"value", L"100.000000");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"float", true, L"name", L"FogDensity", L"value", L"0.010000");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"bool", true, L"name", L"FogPixel", L"value", L"false");
	m_OutXml->writeLineBreak();
	m_OutXml->writeElement(L"bool", true, L"name", L"FogRange", L"value", L"false");
	m_OutXml->writeLineBreak();
	m_OutXml->writeClosingTag(L"attributes");
	m_OutXml->writeLineBreak();
	m_OutXml->writeLineBreak();

	return true;
}

bool MmpSave::Next(irr::core::stringc& type, irr::io::IAttributes* attributes,
	irr::core::array<irr::io::IAttributes*>& materials, irr::core::array<irr::io::IAttributes*>& animators,
	irr::io::IAttributes* userData, bool& child)
{
	// when saving, process the file names to align them to this package
	// materials
	for (irr::u32 i = 0; i < materials.size(); ++i)
	{
		for (irr::u32 t = 0; t < irr::video::MATERIAL_MAX_TEXTURES; ++t)
		{
			wxString texId = wxString::Format("Texture%d", t + 1);
			irr::video::ITexture* texture = materials[i]->getAttributeAsTexture(texId.ToStdString().c_str());
			if (texture)
			{
				wxString location(texture->getName().getPath().c_str());

				// first try to load the texture file directly from disk
				wxFileName fileName(location);
				if (fileName.IsOk() && fileName.IsFileReadable())
				{
					wxString destPath = wxString::Format(wxT("textures/%s"), fileName.GetFullName());
					if (AddFile(fileName, destPath))
					{
						// update the texture path to this package
						location = wxString::Format(wxT("%s:%s"),
								m_FileName.GetFullName(), destPath);
					}
					else
						wxLogWarning(_("Failed to add file '%s'"), fileName.GetFullPath());
				}
				else // it's likely in another package
				{
					wxString archivePath(location.substr(0,
						location.rfind(wxT(':'))));
					wxFileName filePath(location.substr(location.rfind(wxT(':')) + 1));

					wxFileName fn(archivePath);
					if (fn.GetFullName() == m_FileName.GetFullName() ||	// saving
						fn.GetExt() == wxT("mmp")) // save as
					{
						// copy the the texture over
						wxFileInputStream source(fn.GetFullPath());
						if (source.IsOk())
						{
							wxZipInputStream inputStream(source);
							wxZipEntry* entry = inputStream.GetNextEntry();
							while (entry)
							{
								// find the entry to copy
								wxFileName entryPath(entry->GetName()); 
								if (entryPath == filePath)
								{
									if (m_OutStream.CopyEntry(entry, inputStream))
									{
										// update the texture path to this package
										location = wxString::Format(wxT("%s:%s"),
											m_FileName.GetFullName(), entryPath.GetFullPath());
									}
									else
										wxLogWarning(_("Failed to copy file '%s'"), filePath.GetFullPath());

									entry->UnRef();
									break;
								}

								entry->UnRef();
								entry = inputStream.GetNextEntry();
							}
						}
					}

					// sanitize the location path
					wxFileName outPackageName(location.substr(0,
						location.rfind(wxT(':'))));
					wxString outFileName(location.substr(location.rfind(wxT(':')) + 1));
					location = wxString::Format(wxT("%s:%s"),
						outPackageName.GetFullName(), outFileName);

					// update the texture attributes
					materials[i]->setAttribute(texId.ToStdString().c_str(), texture,
						location.ToStdString().c_str());
				}
			}
		}
	}

	return IrrSave::Next(type, attributes, materials, animators, userData, child);
}

void MmpSave::Finalize(void)
{
	IrrSave::Finalize();

	// saving
	StreamWriteFile* swf = dynamic_cast<StreamWriteFile*>(m_WriteFile);
	wxMemoryInputStream irrStream(swf->GetStream());

	// write the .irr XML file to the package
	if (m_OutStream.PutNextEntry(swf->getFileName().c_str()))
	{
		m_OutStream.Write(irrStream);
		m_OutStream.Close();
	}

	m_OutFile.Commit();

	m_WriteFile->drop();
	m_WriteFile = nullptr;
}

bool MmpSave::AddFile(const wxFileName& source, const wxString& dest)
{
	wxLogNull ln; // suppress error messages

	wxFileInputStream srcFile(source.GetFullPath());
	if (!srcFile.IsOk())
		return false;

	if (!m_OutStream.PutNextEntry(dest))
		return false;

	size_t size = srcFile.GetSize();
	m_OutStream.Write(srcFile);

	return true;
}

MmpLoad::MmpLoad(const wxFileName& fileName)
	: IrrLoad(fileName), m_InFile(fileName.GetFullPath()),
	m_InStream(m_InFile)
{
	m_ReadFile = nullptr;
}

MmpLoad::~MmpLoad(void)
{
}

Serializer::CONTENT_TYPE MmpLoad::Verify(void)
{	
	wxZipEntry* entry = m_InStream.GetNextEntry();
	while (entry)
	{
		wxFileName fn(entry->GetName());
		if (fn.GetPath().empty() &&	// the .irr file must be at the root
			fn.GetName().compare(m_FileName.GetName()) == 0 && // the .irr file name must match the package name
			fn.GetExt().compare(wxT("irr")) == 0) // and it's actually an .irr file
		{
			// confirm the contents
			wxXmlDocument doc(m_InStream);
			if (!doc.IsOk())
				return CONTENT_UNKNOWN;

			if (doc.GetRoot()->GetName().compare("irr_scene") != 0)
				return CONTENT_UNKNOWN;

			entry->UnRef();
			return CONTENT_MAP;
		}
	
		entry->UnRef();
		entry = m_InStream.GetNextEntry();
	}
	
	return CONTENT_UNKNOWN;
}

bool MmpLoad::Begin(wxInt32& nextId)
{
	wxFileName mapName(m_FileName);
	mapName.SetExt(wxT("irr"));

	wxZipEntry* entry = m_InStream.GetNextEntry();
	while (entry)
	{
		if (entry->GetName() == mapName.GetFullName())
		{
			m_ReadFile = new StreamReadFile(mapName.GetFullName(), m_InStream);
			entry->UnRef();
			break;
		}

		entry->UnRef();
		entry = m_InStream.GetNextEntry();
	}

	if (m_ReadFile == nullptr)
		return false;

	m_InXml = m_FileSystem->createXMLReader(m_ReadFile);
	if (m_InXml == nullptr)
	{
		m_ReadFile->drop();
		m_ReadFile = nullptr;
		return false;
	}

	// process the document for base properties
	while (m_InXml->read())
	{
		irr::core::stringw name(m_InXml->getNodeName());
		if (m_InXml->getNodeType() == irr::io::EXN_ELEMENT)
		{
			if (name == L"irr_scene")
			{
				nextId = m_InXml->getAttributeValueAsInt(L"nextId");

				// read the scene attributes
				if (m_InXml->read() && m_InXml->getNodeType() == irr::io::EXN_ELEMENT)
				{
					name = m_InXml->getNodeName();
					if (name == L"attributes")
					{
						irr::io::IAttributes* attribs = m_FileSystem->createEmptyAttributes(
							m_VideoDriver);
						attribs->read(m_InXml, true);

						// @TODO: process the scene attributes

						attribs->drop();
					}
				}

				break;
			}
		}
	}

	return true;
}

bool MmpLoad::Next(irr::core::stringc& type, irr::io::IAttributes* attributes,
	irr::core::array<irr::io::IAttributes*>& materials, irr::core::array<irr::io::IAttributes*>& animators,
	irr::io::IAttributes* userData, bool& child)
{
	// pass through
	return IrrLoad::Next(type, attributes, materials, animators, userData, child);
}

void MmpLoad::Finalize(void)
{
	IrrLoad::Finalize();

	if (m_ReadFile)
	{
		m_ReadFile->drop();
		m_ReadFile = nullptr;
	}
}
