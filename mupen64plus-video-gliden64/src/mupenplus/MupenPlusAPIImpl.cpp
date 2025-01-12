#include "GLideN64_mupenplus.h"
#include "../PluginAPI.h"
#include "../GLideN64.h"
#include "../OpenGL.h"

ptr_ConfigGetSharedDataFilepath ConfigGetSharedDataFilepath = NULL;
ptr_ConfigGetUserConfigPath ConfigGetUserConfigPath = NULL;
ptr_ConfigGetUserDataPath ConfigGetUserDataPath = NULL;
ptr_ConfigGetUserCachePath ConfigGetUserCachePath = NULL;
ptr_ConfigOpenSection      ConfigOpenSection = NULL;
ptr_ConfigDeleteSection ConfigDeleteSection = NULL;
ptr_ConfigSaveSection ConfigSaveSection = NULL;
ptr_ConfigSaveFile ConfigSaveFile = NULL;
ptr_ConfigSetDefaultInt    ConfigSetDefaultInt = NULL;
ptr_ConfigSetDefaultFloat  ConfigSetDefaultFloat = NULL;
ptr_ConfigSetDefaultBool   ConfigSetDefaultBool = NULL;
ptr_ConfigSetDefaultString ConfigSetDefaultString = NULL;
ptr_ConfigGetParamInt      ConfigGetParamInt = NULL;
ptr_ConfigGetParamFloat    ConfigGetParamFloat = NULL;
ptr_ConfigGetParamBool     ConfigGetParamBool = NULL;
ptr_ConfigGetParamString   ConfigGetParamString = NULL;

/* definitions of pointers to Core video extension functions */
ptr_VidExt_Init                  CoreVideo_Init = NULL;
ptr_VidExt_Quit                  CoreVideo_Quit = NULL;
ptr_VidExt_ListFullscreenModes   CoreVideo_ListFullscreenModes = NULL;
ptr_VidExt_SetVideoMode          CoreVideo_SetVideoMode = NULL;
ptr_VidExt_SetCaption            CoreVideo_SetCaption = NULL;
ptr_VidExt_ToggleFullScreen      CoreVideo_ToggleFullScreen = NULL;
ptr_VidExt_ResizeWindow          CoreVideo_ResizeWindow = NULL;
ptr_VidExt_GL_GetProcAddress     CoreVideo_GL_GetProcAddress = NULL;
ptr_VidExt_GL_SetAttribute       CoreVideo_GL_SetAttribute = NULL;
ptr_VidExt_GL_GetAttribute       CoreVideo_GL_GetAttribute = NULL;
ptr_VidExt_GL_SwapBuffers        CoreVideo_GL_SwapBuffers = NULL;

void(*renderCallback)(int) = NULL;

m64p_error PluginAPI::PluginStartup(m64p_dynlib_handle _CoreLibHandle)
{
	return M64ERR_SUCCESS;
}

m64p_error PluginAPI::PluginShutdown()
{
#ifdef RSPTHREAD
	_callAPICommand(acRomClosed);
	delete m_pRspThread;
	m_pRspThread = NULL;
#else
	video().stop();
#endif
	return M64ERR_SUCCESS;
}

m64p_error PluginAPI::PluginGetVersion(
	m64p_plugin_type * _PluginType,
	int * _PluginVersion,
	int * _APIVersion,
	const char ** _PluginNamePtr,
	int * _Capabilities
)
{
	/* set version info */
	if (_PluginType != NULL)
		*_PluginType = M64PLUGIN_GFX;

	if (_PluginVersion != NULL)
		*_PluginVersion = PLUGIN_VERSION;

	if (_APIVersion != NULL)
		*_APIVersion = VIDEO_PLUGIN_API_VERSION;

	if (_PluginNamePtr != NULL)
		*_PluginNamePtr = pluginName;

	if (_Capabilities != NULL)
	{
		*_Capabilities = 0;
	}

	return M64ERR_SUCCESS;
}

void PluginAPI::SetRenderingCallback(void (*callback)(int))
{
	renderCallback = callback;
}

void PluginAPI::ResizeVideoOutput(int _Width, int _Height)
{
	video().setWindowSize(_Width, _Height);
}

void PluginAPI::ReadScreen2(void * _dest, int * _width, int * _height, int _front)
{
	video().readScreen2(_dest, _width, _height, _front);
}
