#pragma once
#ifndef xstring_h__
#define xstring_h__
class XScriptVM;

void init_string_lib();
void host_str_len(XScriptVM* vm);
void host_str_find(XScriptVM* vm);
void host_str_sub(XScriptVM* vm);

#endif // xstring_h__