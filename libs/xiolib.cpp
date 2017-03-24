#include "XsriptVM.h"
#include "xiolib.h"

void init_io_lib()
{

	std::vector<HostFunction> iofuncVec;
	iofuncVec.push_back(HostFunction("input", host_io_input));
	iofuncVec.push_back(HostFunction("output",  host_io_output));
	gScriptVM.RegisterHostLib("io", iofuncVec);


	std::vector<HostFunction> filefuncVec;
	filefuncVec.push_back(HostFunction("open", file_open));
	filefuncVec.push_back(HostFunction("close", file_close));
	filefuncVec.push_back(HostFunction("seek", file_seek));
	filefuncVec.push_back(HostFunction("read", file_read));
	filefuncVec.push_back(HostFunction("write", file_write));
	filefuncVec.push_back(HostFunction("readline", file_readline));
	filefuncVec.push_back(HostFunction("writeline", file_writeline));
	filefuncVec.push_back(HostFunction("flush", file_flush));
	filefuncVec.push_back(HostFunction("eof", file_eof));
	filefuncVec.push_back(HostFunction("size", file_size));
	filefuncVec.push_back(HostFunction("tell", file_ftell));
	gScriptVM.RegisterUserClass("File", NULL, filefuncVec);

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

void file_open(XScriptVM* vm)
{
	CheckParam(file_open, 1, fileName, OP_TYPE_STRING);
	CheckParam(file_open, 2, mode, OP_TYPE_STRING);

	FILE* f = fopen(stringRawValue(&fileName), stringRawValue(&mode));
	if (f != NULL)
	{
		vm->setReturnAsUserData("File", f);
	}
	else
	{
		vm->setReturnAsNil(0);
	}
}

void file_close(XScriptVM* vm)
{
	CheckUserTypeParam(file_close, 0, file, FILE, "File");
	fclose(file);
}

void file_seek(XScriptVM* vm)
{
	CheckUserTypeParam(file_seek, 0, file , FILE, "File");
	CheckParam(file_seek, 1, offset, OP_TYPE_INT);
	CheckParam(file_seek, 2, origin, OP_TYPE_INT);
	fseek(file, (int)offset.iIntValue, (int)origin.iIntValue);
}

void file_size(XScriptVM* vm)
{
	CheckUserTypeParam(file.size, 0, file, FILE, "File");
	size_t curPos = ftell(file);
	fseek(file, 0, SEEK_END);
	int size = ftell(file);
	fseek(file, curPos, SEEK_SET);
	vm->setReturnAsInt(size);
}

void file_read(XScriptVM* vm)
{
	CheckUserTypeParam(file_read, 0, file, FILE, "File");
	CheckParam(file_read, 1, size, OP_TYPE_INT);
	char* buff = new char[(size_t)size.iIntValue];
	int realSize = fread(buff, 1, (size_t)size.iIntValue, file );

	XString* bufStr = vm->NewXString((const char*)buff, realSize);
	delete buff;
	vm->setReturnAsValue(vm->ConstructValue(bufStr));
}

void file_write(XScriptVM* vm)
{
	CheckUserTypeParam(file_write, 0, file, FILE, "File");
	CheckParam(file_write, 1, bufStr, OP_TYPE_STRING);
	int realSize = fwrite(stringRawValue(&bufStr), 1, stringRawLen(&bufStr), file);
	vm->setReturnAsInt(realSize);
}

void file_readline(XScriptVM* vm)
{
	CheckUserTypeParam(file_readline, 0, file, FILE, "File");
	CheckParam(file_readline, 1, maxSize, OP_TYPE_INT);
	char* buff = new char[(size_t)maxSize.iIntValue];
	
	if (fgets(buff, (int)maxSize.iIntValue, file))
	{
		XString* bufStr = vm->NewXString((const char*)buff);
		vm->setReturnAsValue(vm->ConstructValue(bufStr));
	}
	else
	{
		vm->setReturnAsNil(0);
	}
}

void file_writeline(XScriptVM* vm)
{
	CheckUserTypeParam(file_writeline, 0, file, FILE, "File");
	CheckParam(file_writeline, 1, bufStr, OP_TYPE_STRING);

	int realSize = fputs(stringRawValue(&bufStr), file);
	vm->setReturnAsInt(realSize);
}

void file_flush(XScriptVM* vm)
{
	CheckUserTypeParam(file_flush, 0, file, FILE, "File");
	fflush(file);
}

void file_eof(XScriptVM* vm)
{
	CheckUserTypeParam(file_flush, 0, file, FILE, "File");
	vm->setReturnAsInt(feof(file));
}

void file_ftell(XScriptVM* vm)
{
	CheckUserTypeParam(file_flush, 0, file, FILE, "File");
	vm->setReturnAsInt(ftell(file));
}
