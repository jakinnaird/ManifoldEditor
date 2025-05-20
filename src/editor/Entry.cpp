/*
* ManifoldEditor
* 
* Copyright (c) 2023 James Kinnaird
*/

#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/cmdline.h>
#include <wx/dir.h>
#include <wx/fileconf.h>
#include <wx/filefn.h>
#include <wx/filesys.h>
#include <wx/fs_arc.h>
#include <wx/fs_filter.h>
#include <wx/mimetype.h>
#include <wx/stdpaths.h>
#include <wx/sysopt.h>

#include "Common.hpp"
#include "FSHandler.hpp"
#include "MainWindow.hpp"
#include "MpkFSHandler.hpp"
#include "PropertyPanel.hpp"
#include "Serialize.hpp"

// @TODO: enable GPU acceleration under MSW
//#if defined(__WXMSW__)
//extern "C"
//{
//	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
//	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
//}
//#endif

class ManifoldEditor : public wxApp
{
public:
	bool OnInit(void)
	{
		try
		{
			wxHandleFatalExceptions();
			wxInitAllImageHandlers();

			SetAppName(APP_NAME);

			wxStandardPaths::Get().UseAppInfo(wxStandardPaths::AppInfo_AppName);
			wxStandardPaths::Get().SetFileLayout(wxStandardPaths::FileLayout_XDG);

			// load the configuration files
			wxConfigBase::DontCreateOnDemand();

			wxFileName userConfigPath(wxStandardPaths::Get().GetDocumentsDir(), wxT(""));
			userConfigPath.AppendDir(wxT(APP_NAME));
			userConfigPath.SetFullName(wxStandardPaths::Get().MakeConfigFileName(wxT("user")));
			if (!userConfigPath.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL))
				userConfigPath.Clear();

			wxFileName systemConfigPath(wxStandardPaths::Get().GetDataDir(), wxT(""));
			systemConfigPath.SetFullName(wxStandardPaths::Get().MakeConfigFileName(wxT("editor")));

			wxFileConfig* config = new wxFileConfig(wxT(APP_NAME), wxEmptyString,
				userConfigPath.GetFullPath(), systemConfigPath.GetFullPath());
			if (config)
				delete wxConfigBase::Set(config);

#if defined(__WXMSW__)
			if (GetComCtl32Version() >= 600 && ::wxDisplayDepth() >= 32)
				wxSystemOptions::SetOption("msw.remap", 2);
			else
				wxSystemOptions::SetOption("msw.remap", 0);
#endif

#if defined(__WXOSX__)
			wxSystemOptions::SetOption(wxOSX_FILEDIALOG_ALWAYS_SHOW_TYPES, 1);
#endif

			wxFileSystem::AddHandler(new wxArchiveFSHandler);
			wxFileSystem::AddHandler(new wxFilterFSHandler);
			FolderFSHandler* folderHandler = new FolderFSHandler;
			wxFileSystem::AddHandler(folderHandler);
			MpkFSHandler* mpkHandler = new MpkFSHandler;
			wxFileSystem::AddHandler(mpkHandler);

			ISerializerFactory::AddSerializer(wxT("irr"),
				std::shared_ptr<ISerializerFactory>(new SerializerFactory<IrrSave, IrrLoad>(
					_("Irrlicht Scene (*.irr)|*.irr"))));
			ISerializerFactory::AddSerializer(wxT("mmp"),
				std::shared_ptr<ISerializerFactory>(new SerializerFactory<MmpSave, MmpLoad>(
					_("Manifold Editor Map (*.mmp)|*.mmp"))));

			// register all the engine MIME types
			static wxFileTypeInfo engineMimeTypes[] = 
			{
				// models
				{ wxT("model/3ds"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("3ds") },
				{ wxT("model/b3d"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("b3d") },
				{ wxT("model/md2"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("md2") },
				{ wxT("model/md3"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("md3") },
				{ wxT("model/mdl"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("mdl") },
				{ wxT("model/obj"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("obj") },
				{ wxT("model/X"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("x") },

				// sounds
				{ wxT("audio/mp3"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("mp3") },
				{ wxT("audio/ogg"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("ogg") },
				{ wxT("audio/wav"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("wav") },

				// maps
				{ wxT("map/irrlicht"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("irr") },
				{ wxT("map/manifold"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("mmp") },

				// packages
				{ wxT("package/manifold"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("mpk") },

				// scripts
				{ wxT("text/javascript"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("js") },

				// shaders - we only support 
				{ wxT("shader/vertex"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("vert"), wxT("vsh"), wxNullPtr },
				{ wxT("shader/pixel"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("frag"), wxT("psh"), wxNullPtr },
				{ wxT("shader/hlsl"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("hlsl") },

				// textures
				{ wxT("image/tga"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("tga") },

				// language translations
				{ wxT("lang/mo"), wxEmptyString, wxEmptyString, wxEmptyString, wxT("mo") },

				// end the list
				wxFileTypeInfo()
			};
			wxTheMimeTypesManager->AddFallbacks(engineMimeTypes);

			// preload all the packages
			wxString entry;
			long cookie;
			wxConfigPathChanger cpc(config, wxString::Format(wxT("/Paths/")));
			if (config->GetFirstEntry(entry, cookie))
			{
				do
				{
					wxString path = config->Read(entry);
					wxDir dir(path);
					if (dir.IsOpened())
					{
						// register the folder
						folderHandler->MountFolder(path);
						mpkHandler->AddSearchPath(path);

						wxString filename;
						bool cont = dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
						while (cont)
						{
							wxFileName fn(path, filename);
							if (fn.GetExt().CmpNoCase(wxT("mpk")) == 0 ||
								fn.GetExt().CmpNoCase(wxT("zip")) == 0)
								BrowserWindow::AddPackage(fn.GetFullPath());

							cont = dir.GetNext(&filename);
						}
					}
				} while (config->GetNextEntry(entry, cookie));
			}

			MainWindow* mainWindow = new MainWindow();
			mainWindow->Show(true);
			SetTopWindow(mainWindow);

			// default to a map editor
			wxString fileToLoad(wxT("*.mmp"));
			// default to a project editor
			// wxString fileToLoad(wxT("*.mep"));

			// see if we want to load a file
			wxCmdLineParser params(wxApp::argc, wxApp::argv);
			params.AddParam(_("File to open"), wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL);
			if (params.Parse() == 0)
			{
				if (params.GetParamCount() > 0)
				{
					fileToLoad = params.GetParam(0);
				}
			}

			mainWindow->LoadFile(fileToLoad);
			return true;
		}
		catch (std::exception& e)
		{
			wxLogFatalError(e.what());
		}

		return false;
	}

	int OnExit(void)
	{
		// clean up the config file
		delete wxConfigBase::Set(nullptr);

		return wxApp::OnExit();
	}

	void OnFatalException(void)
	{
		wxMessageBox("Unhandled fatal exception", APP_NAME);
	}
};

wxIMPLEMENT_APP(ManifoldEditor);
