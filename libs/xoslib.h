#pragma once
#ifndef xoslib_h__
#define xoslib_h__
class XScriptVM;

void init_os_lib();
void xos_listdir(XScriptVM* vm);
void xos_remove(XScriptVM* vm);
void xos_rmdir(XScriptVM* vm);
void xos_mkdir(XScriptVM* vm);
void xos_isfile(XScriptVM* vm);
void xos_isdir(XScriptVM* vm);
void xos_exists(XScriptVM* vm);
void xos_getsize(XScriptVM* vm);
void xos_getpwd(XScriptVM* vm);
void xos_setpwd(XScriptVM* vm);
void xos_system(XScriptVM* vm);
#endif // xoslib_h__