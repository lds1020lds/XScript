// XScript.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "sourcefile.h"
#include "lexer.h"
#include "Parser.h"
#include "XSriptVM.h"
#include "windows.h"




class TestClass
{
public:
	int a;
	int b;
	int c;

	TestClass()
	{
		a = 10;
		b = 20;
		c = 30;
	}

	int	Test(int d, int e)
	{
		a += d;
		b += e;
		return a + b + c;
	}
};

class TestClass2: public TestClass
{
public:
	int a;
	int b;
	int c;

	TestClass2()
	{
		a = 10;
		b = 20;
		c = 30;
	}
};



TestClass2 gTest;

void  TestClass_Test(XScriptVM* vm)
{
	TestClass* obj = (TestClass*)vm->getParamAsObj(0, NULL);

	int d; 
	int e; 
	vm->getParamAsInt(1, d);
	vm->getParamAsInt(2, e);

	int e1 = obj->Test(d, e);

	TABLE table = vm->newTable();
	vm->setTableValue(table, "x", 123);
	vm->setTableValue(table, "y", 456);
	vm->setTableValue(table, 1, 1);
	vm->setTableValue(table, 0, "str");
	TABLE subTable = vm->newTable();
	vm->setTableValue(subTable, "x", 10);
	vm->setTableValue(subTable, "z", 10);
	vm->setTableValue(table, "sub", subTable);
	vm->setReturnAsTable(table);
}


void  TestClass_Out(XScriptVM* vm)
{
	TABLE table;
 	vm->getParamAsTable(0, table);
	

	int x;
	vm->getTableValue(table, "x", x);
 	vm->getTableValue(table, "y", x);

 	vm->getTableValue(table, 1, x);

	char* str = NULL;
 	vm->getTableValue(table, 0, str);


	TABLE subTable;
	vm->getTableValue(table, "sub", subTable);


 	vm->getTableValue(subTable, "x", x);
 	vm->getTableValue(subTable, "z", x);
}


void  TestClass2_GetInstance(XScriptVM* vm)
{
	vm->setReturnAsUserData("TestClass2", &gTest);
}


int _tmain(int argc, _TCHAR* argv[])
{
	__int64 startCounters, contersPersecond, currentCounter;
	QueryPerformanceCounter((LARGE_INTEGER *)&startCounters);
	QueryPerformanceFrequency((LARGE_INTEGER *)&contersPersecond);

	

	QueryPerformanceCounter((LARGE_INTEGER *)&currentCounter);
	float fTime = ((currentCounter - startCounters) * 1.0f) / contersPersecond;

	gScriptVM.init();

	gScriptVM.registerHostApi("TestOut", 1, (HOST_FUNC)TestClass_Out);


	gScriptVM.registerUserClass("TestClass", "");
	gScriptVM.beginRegisterUserClassFunc("TestClass");
	gScriptVM.registerUserClassFunc("Test", 2, TestClass_Test);
	gScriptVM.endRegisterUserClassFunc();

	gScriptVM.registerUserClass("TestClass2", "TestClass");

	gScriptVM.beginRegisterUserClassFunc("TestClass2");
	gScriptVM.registerUserClassFunc("GetInstance", 0, TestClass2_GetInstance);
	gScriptVM.endRegisterUserClassFunc();
	
	gScriptVM.doFile("testLib.xs");

	gScriptVM.doFile("test1.xs");
// 	paser.beginCall();
// 	paser.pushFloatParam(1.0f);
// 	paser.pushFloatParam(2.0f);
// 	paser.call("div");
// 	float f = paser.getReturnAsFloat();
// 	paser.endCall();

	return 0;
}

