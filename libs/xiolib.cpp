#include "XsriptVM.h"
#include "xiolib.h"

void init_io_lib()
{

	std::vector<HostFunction> iofuncVec;
	iofuncVec.push_back(HostFunction("input", 0, host_io_input));
	iofuncVec.push_back(HostFunction("output", 1, host_io_output));
	gScriptVM.RegisterHostLib("io", iofuncVec);
}


void host_io_input(XScriptVM* vm)
{
	char buffer[250] = { 0 };
	if (fgets(buffer, sizeof(buffer), stdin) != 0)
	{
		int len = strlen(buffer);
		if (len > 0 && buffer[len - 1] == '\n')
			buffer[len - 1] = 0;
		vm->setReturnAsStr(buffer, 0);
	}
	else
	{
		vm->setReturnAsStr("", 0);
	}
}

void host_io_output(XScriptVM* vm)
{
	char* varName = 0;
	if (vm->getParamAsString(0, varName))
	{
		fputs(varName, stderr);
	}

}