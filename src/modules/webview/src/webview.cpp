/*
 | Webview for LuaRT - HTML/JS/CSS render Widget
 | Luart.org, Copyright (c) Tine Samir 2025
 | See Copyright Notice in LICENSE.TXT
 |-------------------------------------------------
 | webview.c | LuaRT binary module with Widget implementation
*/


#include <stddef.h>
#include <string.h>

#include <luart.h>
#include <Widget.h>
#include <File.h>
#include <Zip.h>	

#include <windows.h>
#include "handler.h"

#include <wrl.h>

using namespace Microsoft::WRL;


//--- Webview type
static luart_type TWebview;
HANDLE URIEvent;
LPWSTR URI;
UINT onReady, onMessage, onLoaded, onFullscreen;

//--- Webview procedure
LRESULT CALLBACK WebviewProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
	if (uMsg == WM_SIZE)
		(static_cast<WebviewHandler*>(((Widget *)GetWindowLongPtr(hwnd, GWLP_USERDATA))->user))->Resize();
	return lua_widgetproc(hwnd, uMsg, wParam, lParam, 0, 0);
}

//------------------------------------ Webview constructor
static const char *get_argstrfield(lua_State *L, const char *field, const char *defstr) {
	switch(lua_getfield(L, 3, field)) {
		case LUA_TSTRING: 	return lua_tostring(L, -1);
		case LUA_TNIL:		return defstr;
	}
	luaL_error(L, "bad '%s' field type (expecting string, found %s)", field, luaL_typename(L, -1));
	return NULL;
}

LUA_CONSTRUCTOR(Webview)
{   
    Widget *w, *wp;
	double dpi;
	BOOL isdark;
    WebviewHandler *wv;
	int i = lua_istable(L, 3) ? 4 : 3;
    HWND h, hParent = (HWND)lua_widgetinitialize(L, &wp, &dpi, &isdark);     
    h = CreateWindowExW(0, L"Window", NULL, WS_VISIBLE | WS_CHILD, (int)luaL_optinteger(L, i, 0)*dpi, (int)luaL_optinteger(L, i+1, 0)*dpi, (int)luaL_optinteger(L, i+2, 320)*dpi, (int)luaL_optinteger(L, i+3, 240)*dpi, hParent, 0, GetModuleHandle(NULL),  NULL);
	
	wv = i == 3 ? new WebviewHandler(h, "", NULL) : new WebviewHandler(h, get_argstrfield(L, "url", ""), get_argstrfield(L, "options", ""));
    w = lua_widgetconstructor(L, h, TWebview, wp, (SUBCLASSPROC)WebviewProc);
    w->user = wv;
	wv->archive = (zip_t *)luaL_embedopen(L);
    return 1;
}

wchar_t *ErrorFromHR(HRESULT errorCode) {
	wchar_t* errorMsg = nullptr;
	DWORD formatResult = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&errorMsg), 0, nullptr);
	errorMsg = (formatResult != 0 && errorMsg) ? errorMsg : nullptr;							
	return errorMsg;
}

static int EvalTaskContinue(lua_State* L, int status, lua_KContext ctx) {
    TaskCallback *cb = (TaskCallback *)ctx;
    
    if (cb->done) {
		lua_pushboolean(L, SUCCEEDED(cb->hr));
		if (!cb->result && FAILED(cb->hr)) {
			wchar_t *err = ErrorFromHR(cb->hr);
			lua_pushwstring(L, err ? err : L"Unknown error");
			if (err)
				LocalFree(err);
		} else if (cb->result)
			lua_pushwstring(L, cb->result);
		delete cb;
        return cb->result ? 2 : 1;
    }
    return lua_yieldk(L, 0, ctx, EvalTaskContinue);
}

LUA_METHOD(Webview, eval) {
   	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	lua_pushboolean(L, false);
  	if (wv->webview2) {
		wchar_t *js = lua_towstring(L, 2);
		TaskCallback *ExecCB = new TaskCallback();
		wv->webview2->ExecuteScript(js, ExecCB); 
		lua_pushtask(L, EvalTaskContinue, ExecCB, NULL);
		lua_pushvalue(L, -1);
		lua_call(L, 0, 0); 
		free(js);
	}
	return 1;
}

LUA_METHOD(Webview, reload) {
	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	if (wv->webview2)
		wv->webview2->Reload();
	return 0;
}

LUA_METHOD(Webview, goback) {
	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	if (wv->webview2)
		wv->webview2->GoBack();
	return 0;
}

LUA_METHOD(Webview, goforward) {
	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	if (wv->webview2)
		wv->webview2->GoForward();
	return 0;
}

LUA_METHOD(Webview, stop) {
   	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
  	if (wv->webview2)
    	wv->webview2->Stop();
  	return 0;
}

LUA_METHOD(Webview, opendevtools) {
   	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
  	if (wv->webview2)
    	wv->webview2->OpenDevToolsWindow();
  	return 0;
}

LUA_METHOD(Webview, addinitscript) {
   	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
  	if (wv->webview3) {
    	wchar_t *msg = lua_towstring(L, 2);
    	wv->webview3->AddScriptToExecuteOnDocumentCreated(msg, Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
						[](HRESULT error, PCWSTR i) -> HRESULT {
							return S_OK;
					}).Get());
    	free(msg);
  	}
  	return 0;
}

LUA_METHOD(Webview, postmessage) {
   	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	int result = FALSE;
  	if (wv->webview2) {
    	wchar_t *msg = lua_towstring(L, 2);
    	result = SUCCEEDED(wv->webview2->PostWebMessageAsJson(msg));    
    	free(msg);
  	}
  	lua_pushboolean(L, result);
  	return 1;
}

LUA_METHOD(Webview, loadstring) {
   	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	int result = FALSE;
  	if (wv->webview2) {
    	wchar_t *str = toUTF16(lua_tostring(L, 2));
    	result = SUCCEEDED(wv->webview2->NavigateToString( str));
    	GlobalFree(str);
  } 
  lua_pushboolean(L, result);
  return 1;
}

LUA_METHOD(call, cdp) {
   	WebviewHandler *wv;
	
	luaL_getmetafield(L, 1, "wv");
	wv = static_cast<WebviewHandler*>(lua_self(L, -1, Widget)->user);

  	if (wv->webview2) {
		luaL_getmetafield(L, 1, "path");
    	wchar_t *method = toUTF16(lua_tostring(L, -1));
    	wchar_t *JSON = toUTF16(luaL_checkstring(L, 2));
		TaskCallback *ExecCB = new TaskCallback();

		lua_pushboolean(L, false);
		if (SUCCEEDED(wv->webview2->CallDevToolsProtocolMethod(method, JSON, static_cast<ICoreWebView2CallDevToolsProtocolMethodCompletedHandler *>(ExecCB)))) {
			lua_pushtask(L, EvalTaskContinue, ExecCB, NULL);
			lua_pushvalue(L, -1);
			lua_call(L, 0, 0); 
		} else
			delete ExecCB;
		GlobalFree(method);
		GlobalFree(JSON);
  } 
  return 1;
}

// LUA_METHOD(index, cdp) {
// 	luaL_checkstring(L, 2);
// 	lua_getmetatable(L, 1);
// 	if (lua_getfield(L, -1, "__cdp")) {
// 		lua_pushstring(L, ".");
// 		lua_pushvalue(L, 2);
// 		lua_concat(L, 3);
// 	} else {
// 		lua_createtable(L, 0, 2);

// 	}

// 	// lua_createtable(L, 0, 0);
// 	// lua_createtable(L, 0, 1);
// 	// lua_pushvalue(L, lua_upvalueindex(1));
// 	// lua_pushvalue(L, 2);
// 	// lua_pushcclosure(L, call_cdp, 2);
// 	return 1;
// }

static int path_index(lua_State* L) {
    const char* key = luaL_checkstring(L, 2);
    int type = luaL_getmetafield(L, 1, "path");
    const char* current = type == LUA_TNIL ? "" : lua_tostring(L, -1);
    lua_pop(L, 1);
    
    size_t current_len = strlen(current);
    size_t key_len = strlen(key);
    size_t new_len = current_len + key_len + (current_len > 0 ? 1 : 0);
    char* new_path = (char*)malloc(new_len + 1);   

    if (current_len > 0)
        sprintf(new_path, "%s.%s", current, key);
    else
        strcpy(new_path, key);
    
	lua_getmetatable(L, 1);
    lua_pushstring(L, new_path);
    lua_setfield(L, -2, "path");
	lua_pop(L, 1);
    free(new_path);

    lua_pushvalue(L, 1);
    return 1;
}

LUA_PROPERTY_GET(Webview, cdp) {
	lua_createtable(L, 0, 2);
    if (luaL_newmetatable(L, "path_mt")) {
        lua_pushcfunction(L, path_index);
        lua_setfield(L, -2, "__index");
        lua_pushcfunction(L, call_cdp);
        lua_setfield(L, -2, "__call");
		lua_pushvalue(L, 1);
		lua_setfield(L, -2, "wv");
    }
    lua_setmetatable(L, -2);
	return 1;
}

LUA_PROPERTY_SET(Webview, url) {
   	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	int result = FALSE;
  	if (wv->webview2) {
    	wchar_t *uri = toUTF16(lua_tostring(L, 2));
    	result = SUCCEEDED(wv->webview2->Navigate( uri));
    	GlobalFree(uri);
  } 
  lua_pushboolean(L, result);
  return 1;
}

LUA_METHOD(Webview, hostfromfolder) {
   	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	int result = FALSE;
	if (wv->webview3) {
		wchar_t *host = lua_towstring(L, 2);
		wchar_t *path = lua_towstring(L, 3);
		result = SUCCEEDED(wv->webview3->SetVirtualHostNameToFolderMapping(host, path, COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW));
		free(path);
		free(host);
	}
	lua_pushboolean(L, result);
	return 1;
}

LUA_METHOD(Webview, restorehost) {
   	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	int result = FALSE;
	if (wv->webview3) {
		wchar_t *host = lua_towstring(L, 2);
		result = SUCCEEDED(wv->webview3->ClearVirtualHostNameToFolderMapping(host));
		free(host);
	}
	lua_pushboolean(L, result);
	return 1;
}

static void printsettingsFromTable(lua_State *L, ICoreWebView2PrintSettings *printSettings) {
	ComPtr<ICoreWebView2PrintSettings2> printSettings2 = nullptr;

	luaL_checktype(L, 2, LUA_TTABLE);
	if (lua_getfield(L, 2, "orientation")) 
		printSettings->put_Orientation(strcmp(lua_tostring(L, -1), "landscape") == 0 ? COREWEBVIEW2_PRINT_ORIENTATION_LANDSCAPE : COREWEBVIEW2_PRINT_ORIENTATION_PORTRAIT);
	if (lua_getfield(L, 2, "pageHeight"))
		printSettings->put_PageHeight(lua_tonumber(L, -1));
	if (lua_getfield(L, 2, "pageWidth"))
		printSettings->put_PageWidth(lua_tonumber(L, -1));
	if (lua_getfield(L, 2, "header"))
		printSettings->put_ShouldPrintHeaderAndFooter(lua_toboolean(L, -1));
	if (lua_getfield(L, 2, "backgrounds"))
		printSettings->put_ShouldPrintBackgrounds(lua_toboolean(L, -1));
	if (SUCCEEDED(printSettings->QueryInterface(IID_PPV_ARGS(&printSettings2)))) {
		if (lua_getfield(L, 2, "colorMode"))
			printSettings2->put_ColorMode(strcmp(lua_tostring(L, -1), "grayscale") == 0 ? COREWEBVIEW2_PRINT_COLOR_MODE_GRAYSCALE : COREWEBVIEW2_PRINT_COLOR_MODE_COLOR);
		if (lua_getfield(L, 2, "copies")) {
			lua_Integer n = lua_tointeger(L, -1);
			printSettings2->put_Copies(n == 0 ? 1 : n);
		}
		if (lua_getfield(L, 2, "scaleFactor"))
			printSettings2->put_ScaleFactor(lua_tonumber(L, -1));
	}
}

LUA_METHOD(Webview, print) {
  	ICoreWebView2_16 *webView2_16;
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);

	if (lua_gettop(L) > 1) {
		ComPtr<ICoreWebView2PrintSettings> printSettings = nullptr;
		ComPtr<ICoreWebView2Environment6> env6 = nullptr;
		ComPtr<ICoreWebView2Environment> env = nullptr;
		HWND h = wv->hwnd;
		
		luaL_checktype(L, 2, LUA_TTABLE);
		if (SUCCEEDED(wv->webview3->get_Environment(&env))) {
			if (SUCCEEDED(env->QueryInterface(IID_PPV_ARGS(&env6)))) {
				if (SUCCEEDED(env6->CreatePrintSettings(&printSettings))) {
					printsettingsFromTable(L, printSettings.Get());
					lua_pushboolean(L, false);
					TaskCallback *ExecCB = new TaskCallback();
					lua_pushboolean(L, false);
					if (SUCCEEDED(wv->webview3->Print(printSettings.Get(), static_cast<ICoreWebView2PrintCompletedHandler *>(ExecCB)))) {
						lua_pushtask(L, EvalTaskContinue, ExecCB, NULL);
						lua_pushvalue(L, -1);
						lua_call(L, 0, 0); 
					} else delete ExecCB;
				}
			}
		}
	} else lua_pushboolean(L, SUCCEEDED(wv->webview3->ShowPrintUI(COREWEBVIEW2_PRINT_DIALOG_KIND_BROWSER)));
	return 1;
}

LUA_METHOD(Webview, printPDF) {
	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	ComPtr<ICoreWebView2PrintSettings> printSettings = nullptr;
	ComPtr<ICoreWebView2Environment6> env6 = nullptr;
	ComPtr<ICoreWebView2Environment> env = nullptr;
	std::wstring fname;
	HWND h = wv->hwnd;

	luaL_checktype(L, 2, LUA_TTABLE);
	if (SUCCEEDED(wv->webview3->get_Environment(&env))) {
		if (SUCCEEDED(env->QueryInterface(IID_PPV_ARGS(&env6)))) {
			if (SUCCEEDED(env6->CreatePrintSettings(&printSettings))) {
				printsettingsFromTable(L, printSettings.Get());
				if (!lua_getfield(L, 2, "file"))
					luaL_error(L, "'file' key not found in the provided table");
				fname = luaL_checkFilename(L, -1);
				size_t pos = fname.find_last_of(L"\\");
				if (!(pos != std::wstring::npos && (pos > 0))) {
					DWORD bufferSize = GetCurrentDirectoryW(0, nullptr);
					std::wstring currentDirectory(bufferSize, L'\0');
					DWORD result = GetCurrentDirectoryW(bufferSize, &currentDirectory[0]);
					currentDirectory.resize(result);
					fname = currentDirectory + L"\\" + fname;
				}
				lua_pushboolean(L, false);
				TaskCallback *ExecCB = new TaskCallback();
				lua_pushboolean(L, false);
				if (SUCCEEDED(wv->webview3->PrintToPdf(fname.c_str(), printSettings.Get(), static_cast<ICoreWebView2PrintToPdfCompletedHandler *>(ExecCB)))) {
					lua_pushtask(L, EvalTaskContinue, ExecCB, NULL);
					lua_pushvalue(L, -1);
					lua_call(L, 0, 0); 
				} else delete ExecCB;
			}
		}
	}
	return 1;
}

LUA_PROPERTY_GET(Webview, zoom) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	double zoom = 1;
  	if (wv->controller)
    	wv->controller->get_ZoomFactor(&zoom);
  	lua_pushnumber(L, zoom);
  	return 1;
}

LUA_PROPERTY_SET(Webview, zoom) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	double zoom = 1;
  	if (wv->controller)
    	wv->controller->put_ZoomFactor((double)luaL_checknumber(L, 2));
  	return 0;
}

LUA_PROPERTY_GET(Webview, cangoback) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	int result = false;
  	if (wv->webview2)
		wv->webview2->get_CanGoBack(&result);
	lua_pushboolean(L, result);
	return 1;
}

LUA_PROPERTY_GET(Webview, cangoforward) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	int result = false;
  	if (wv->webview2)
		wv->webview2->get_CanGoForward(&result);
	lua_pushboolean(L, result);
	return 1;
}

LUA_PROPERTY_GET(Webview, url) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
  	if (wv->webview2) {
		wchar_t *result = NULL;
		wv->webview2->get_Source(&result);
		lua_pushwstring(L, result);
		CoTaskMemFree(result);
	} else lua_pushnil(L);
	return 1;
}

LUA_PROPERTY_GET(Webview, title) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
  	if (wv->webview2) {
		wchar_t *result;
		wv->webview2->get_DocumentTitle(&result);
		lua_pushwstring(L, result);
		CoTaskMemFree(result);
	} else lua_pushnil(L);
	return 1;
}

LUA_PROPERTY_GET(Webview, devtools) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	int result = FALSE;
  	if (wv->settings)
   		wv->settings->get_AreDevToolsEnabled(&result);
	lua_pushboolean(L, result);
	return 1;
}

LUA_PROPERTY_SET(Webview, devtools) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
  	if (wv->settings)
   		wv->settings->put_AreDevToolsEnabled(lua_toboolean(L, 2));
	return 0;
}

LUA_PROPERTY_GET(Webview, acceleratorkeys) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	int result = FALSE;
  	if (wv->settings)
   		wv->settings->get_AreBrowserAcceleratorKeysEnabled(&result);
	lua_pushboolean(L, result);
	return 1;
}

LUA_PROPERTY_SET(Webview, acceleratorkeys) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
  	if (wv->settings)
   		wv->settings->put_AreBrowserAcceleratorKeysEnabled(lua_toboolean(L, 2));
	return 0;
}

LUA_PROPERTY_GET(Webview, contextmenu) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	int result = FALSE;
  	if (wv->settings)
   		wv->settings->get_AreDefaultContextMenusEnabled(&result);
	lua_pushboolean(L, result);
	return 1;
}

LUA_PROPERTY_SET(Webview, contextmenu) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
  	if (wv->settings)
   		wv->settings->put_AreDefaultContextMenusEnabled(lua_toboolean(L, 2));
	return 0;
}

LUA_PROPERTY_GET(Webview, statusbar) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	int result = FALSE;
  	if (wv->settings)
   		wv->settings->get_IsStatusBarEnabled(&result);
	lua_pushboolean(L, result);
	return 1;
}

LUA_PROPERTY_SET(Webview, statusbar) {
  	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
  	if (wv->settings)
   		wv->settings->put_IsStatusBarEnabled(lua_toboolean(L, 2));
	return 0;
}

LUA_PROPERTY_GET(Webview, useragent) {
	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	if (wv->settings) {
	wchar_t *result = NULL;
	wv->settings->get_UserAgent(&result);
	lua_pushwstring(L, result);
	CoTaskMemFree(result);
	} else lua_pushnil(L);
	return 1;
}

LUA_PROPERTY_SET(Webview, useragent) {
	WebviewHandler *wv = static_cast<WebviewHandler*>(lua_self(L, 1, Widget)->user);
	int result = FALSE;
	if (wv->settings) {
		wchar_t *ua = toUTF16(lua_tostring(L, 2));
		result = SUCCEEDED(wv->settings->put_UserAgent(ua));
		GlobalFree(ua);
	}
	lua_pushboolean(L, result);
	return 1;
}

LUA_METHOD(Webview, __gc) {
  Widget *w = lua_widgetdestructor(L);
  delete (WebviewHandler *)w->user;
  free(w);
  return 0;
}

OBJECT_MEMBERS(Webview)
  METHOD(Webview, eval)
  METHOD(Webview, addinitscript)
  METHOD(Webview, reload)
  METHOD(Webview, goback)
  METHOD(Webview, goforward)
  METHOD(Webview, stop)
  METHOD(Webview, opendevtools)
  METHOD(Webview, loadstring)
  METHOD(Webview, hostfromfolder)
  METHOD(Webview, restorehost)
  METHOD(Webview, postmessage)
  METHOD(Webview, print)
  METHOD(Webview, printPDF)
  READONLY_PROPERTY(Webview, cdp)
  READWRITE_PROPERTY(Webview, zoom)
  READWRITE_PROPERTY(Webview, devtools)
  READWRITE_PROPERTY(Webview, acceleratorkeys)
  READWRITE_PROPERTY(Webview, contextmenu)
  READWRITE_PROPERTY(Webview, statusbar)
  READWRITE_PROPERTY(Webview, url)
  READONLY_PROPERTY(Webview, title)
  READONLY_PROPERTY(Webview, cangoback)
  READONLY_PROPERTY(Webview, cangoforward)
  READWRITE_PROPERTY(Webview, useragent)
END

OBJECT_METAFIELDS(Webview)
  METHOD(Webview, __gc)
END

int event_onReady(lua_State *L, Widget *w, MSG *msg) {
  lua_throwevent(L, "onReady", 1);
  return 0;
}

int event_onFullscreen(lua_State *L, Widget *w, MSG *msg) {
	lua_pushboolean(L, msg->wParam);
	lua_throwevent(L, "onFullScreenChange", 2);
	return 0;
}

int event_onLoaded(lua_State *L, Widget *w, MSG *msg) {
	lua_pushboolean(L, msg->wParam);
	lua_pushinteger(L, msg->lParam);
	lua_throwevent(L, "onLoaded", 3);
	return 0;
}

int event_onMessage(lua_State *L, Widget *w, MSG *msg) {
    lua_pushwstring(L, (wchar_t*)msg->wParam);
    free((wchar_t*)msg->wParam);
	lua_throwevent(L, "onMessage", 2);
	return 0;
}

extern "C" {
	int __declspec(dllexport) luaopen_webview(lua_State *L) {
		onReady = lua_registerevent(L, NULL, event_onReady);
		onMessage = lua_registerevent(L, NULL, event_onMessage);
		onLoaded = lua_registerevent(L, NULL, event_onLoaded);
		onFullscreen = lua_registerevent(L, NULL, event_onFullscreen);
		luaL_require(L, "ui");
		lua_regwidgetmt(L, Webview, WIDGET_METHODS, FALSE, FALSE, FALSE, FALSE, FALSE);
		luaL_setrawfuncs(L, Webview_methods);
		return 0;
	}
}