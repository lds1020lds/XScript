#pragma once
#ifndef xio_h__
#define xio_h__

class XScriptVM;

void init_io_lib();

void host_io_input(XScriptVM* vm);
void host_io_output(XScriptVM* vm);

#endif // xio_h__