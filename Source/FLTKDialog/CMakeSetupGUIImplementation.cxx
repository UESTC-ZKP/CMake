
#include "CMakeSetupGUIImplementation.h"
#include "Fl/fl_file_chooser.H"
#include "Fl/filename.H"
#include "Fl/fl_ask.H"
#include "cstring"
#include "../cmCacheManager.h"
#include "../cmMakefile.h"
#include <iostream>
#include "FLTKPropertyList.h"
#include "Fl/fl_draw.H"
#include "../cmake.h"



/**
 * Constructor
 */
CMakeSetupGUIImplementation
::CMakeSetupGUIImplementation()
{
  m_BuildPathChanged = false;
  char fname[1024];
  //::GetModuleFileName(NULL,fname,1023);
  m_PathToExecutable = cmSystemTools::GetProgramPath(fname).c_str();
  m_PathToExecutable += "/cmake.exe";
  std::cout << "Path to executable = " << m_PathToExecutable << std::endl;
}



/**
 * Destructor
 */
CMakeSetupGUIImplementation
::~CMakeSetupGUIImplementation()
{
}




/**
 * Show the graphic interface
 */
void
CMakeSetupGUIImplementation
::Show( void )
{
  dialogWindow->show();
}





/**
 * Hide the graphic interface
 */
void
CMakeSetupGUIImplementation
::Close( void )
{
  dialogWindow->hide();
}





/**
 * Browse for the path to the sources
 */
void
CMakeSetupGUIImplementation
::BrowseForSourcePath( void )
{
  const char * path = 
                  fl_file_chooser(
                    "Path to Sources",
                    "",
                    sourcePathTextInput->value() );
                    
  if( !path )
  {
    return;
  }
  
  SetSourcePath( path );

}




/**
 * Browse for the path to the binaries
 */
void
CMakeSetupGUIImplementation
::BrowseForBinaryPath( void )
{
  const char * path = 
                  fl_file_chooser(
                    "Path to Binaries",
                    "",
                    binaryPathTextInput->value() );
                    
  if( !path )
  {
    return;
  }

  SetBinaryPath( path );

}





/**
 * Set the source path
 */
void
CMakeSetupGUIImplementation
::SetSourcePath( const char * path )
{
  if( VerifySourcePath( path ) )
  {
    m_WhereSource = path;
    sourcePathTextInput->value( path );
  }

}




/**
 * Set the binary path
 */
void
CMakeSetupGUIImplementation
::SetBinaryPath( const char * path )
{

  if( VerifyBinaryPath( path ) )
  {
    if( m_WhereBuild != path )
    {
      m_BuildPathChanged = true;
      m_WhereBuild = path;
    }
    binaryPathTextInput->value( path );
  }

  LoadCacheFromDiskToGUI();

}



/**
 * Verify the path to binaries
 */
bool
CMakeSetupGUIImplementation
::VerifyBinaryPath( const char * path )
{

  if( !path || strlen(path)==0 )
  {
    fl_alert("Please select the path to the binaries");
    return false; 
  }


  if( !filename_isdir( path ) )
  {
    fl_alert("%s \n Doesn't exist or is not a directory",path);
    return false; 
  }

  return true;
}



/**
 * Verify the path to sources
 */
bool
CMakeSetupGUIImplementation
::VerifySourcePath( const char * path )
{

  if( !path || strlen(path)==0 )
  {
    fl_alert("Please select the path to the sources");
    return false; 
  }


  if( !filename_isdir( path ) )
  {
    fl_alert("%s \n Doesn't exist or is not a directory",path);
    return false; 
  }

  return true;
}




/**
 * Build the project files
 */
void
CMakeSetupGUIImplementation
::BuildProjectFiles( void )
{

  // Verify that source path is a valid directory
  if( !VerifySourcePath( sourcePathTextInput->value() ) )
  { 
    return;
  }

  // Verify that binary path is a valid directory
  if( !VerifyBinaryPath( binaryPathTextInput->value() ) )
  { 
    return;
  }

  SaveCacheFromGUI();
  
  // set the wait cursor
  fl_cursor(FL_CURSOR_WAIT,FL_BLACK,FL_WHITE);
  // get all the info from the dialog
  // this->UpdateData();
  if(!m_BuildPathChanged)
    {
    // if the build path has not changed save the 
    // current GUI values to the cache
    this->SaveCacheFromGUI();
    }
  // Make sure we are working from the cache on disk
  this->LoadCacheFromDiskToGUI();
  // create a cmake object
  cmake make;
  // create the arguments for the cmake object
  std::vector<std::string> args;
  args.push_back( m_PathToExecutable.c_str() );
  std::string arg;
  arg = "-H";
  arg += m_WhereSource;
  args.push_back(arg);
  arg = "-B";
  arg += m_WhereBuild;
  args.push_back(arg);
  // run the generate process
  if(make.Generate(args) != 0)
    {
    cmSystemTools::Error(
      "Error in generation process, project files may be invalid");
    }
  // update the GUI with any new values in the caused by the
  // generation process
  this->LoadCacheFromDiskToGUI();
  // path is up-to-date now
  m_BuildPathChanged = false;
  // put the cursor back
  fl_cursor(FL_CURSOR_DEFAULT,FL_BLACK,FL_WHITE);
  fl_message("Done !");

}




/**
 * Load Cache from disk to GUI
 */
void
CMakeSetupGUIImplementation
::LoadCacheFromDiskToGUI( void )
{
  
    
  if( m_WhereBuild != "" )
    {
    cmCacheManager::GetInstance()->LoadCache( m_WhereBuild.c_str() );
    this->FillCacheGUIFromCacheManager();
	}
}
   

/**
 * Save Cache from disk to GUI
 */
void
CMakeSetupGUIImplementation
::SaveCacheFromGUI( void )
{
  this->FillCacheManagerFromCacheGUI();
  if( m_WhereBuild != "" )
  {
    cmCacheManager::GetInstance()->SaveCache( 
                                  m_WhereBuild.c_str() );
  }
}


/**
 * Fill Cache GUI from cache manager
 */
void
CMakeSetupGUIImplementation
::FillCacheGUIFromCacheManager( void )
{

  // Prepare to add rows to the scroll
  propertyListPack->begin();

  const cmCacheManager::CacheEntryMap &cache =
    cmCacheManager::GetInstance()->GetCacheMap();
  for(cmCacheManager::CacheEntryMap::const_iterator i = cache.begin();
      i != cache.end(); ++i)
    {
    const char* key = i->first.c_str();
    const cmCacheManager::CacheEntry& value = i->second;
    

    switch(value.m_Type )
      {
      case cmCacheManager::BOOL:
        if(cmCacheManager::GetInstance()->IsOn(key))
          {
          m_CacheEntriesList.AddProperty(key,
                                         "ON",
                                         value.m_HelpString.c_str(),
                                         fltk::PropertyList::CHECKBOX,"");
          }
        else
          {
          m_CacheEntriesList.AddProperty(key,
                                         "OFF",
                                         value.m_HelpString.c_str(),
                                         fltk::PropertyList::CHECKBOX,"");
          }
        break;
      case cmCacheManager::PATH:
        m_CacheEntriesList.AddProperty(key, 
                                       value.m_Value.c_str(),
                                       value.m_HelpString.c_str(),
                                       fltk::PropertyList::PATH,"");
        break;
      case cmCacheManager::FILEPATH:
        m_CacheEntriesList.AddProperty(key, 
                                       value.m_Value.c_str(),
                                       value.m_HelpString.c_str(),
                                       fltk::PropertyList::FILE,"");
        break;
      case cmCacheManager::STRING:
        m_CacheEntriesList.AddProperty(key,
                                       value.m_Value.c_str(),
                                       value.m_HelpString.c_str(),
                                       fltk::PropertyList::EDIT,"");
        break;
      case cmCacheManager::INTERNAL:
        break;
      }

    }

  propertyListPack->end();
  propertyListPack->init_sizes();
  cacheValuesScroll->position( 0, 0 );

  this->UpdateData(false);

}


/**
 * UpdateData
 */
void
CMakeSetupGUIImplementation
::UpdateData( bool option )
{
  dialogWindow->redraw();
  Fl::check();
}



/**
 * Fill cache manager from Cache GUI 
 */
void
CMakeSetupGUIImplementation
::FillCacheManagerFromCacheGUI( void )
{
  cmCacheManager::GetInstance()->GetCacheMap();
  std::set<fltk::PropertyItem*> items = m_CacheEntriesList.GetItems();
  for(std::set<fltk::PropertyItem*>::iterator i = items.begin();
      i != items.end(); ++i)
    {
      fltk::PropertyItem* item = *i; 
      cmCacheManager::CacheEntry *entry = 
        cmCacheManager::GetInstance()->GetCacheEntry(
          (const char*)item->m_propName.c_str() );
      if (entry)
        {
        entry->m_Value = item->m_curValue;
        }
    }

}


