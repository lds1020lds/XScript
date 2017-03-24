#include "XsriptVM.h"
#include "xxmllib.h"
#include "tinyxml.h"

void init_xml_lib()
{
	std::vector<HostFunction> xmlNodeVec;
	xmlNodeVec.push_back(HostFunction("Clear", xmlnode_Clear));
	xmlNodeVec.push_back(HostFunction("Parent", xmlnode_Parent));
	xmlNodeVec.push_back(HostFunction("FirstChild", xmlnode_FirstChild));
	xmlNodeVec.push_back(HostFunction("LastChild", xmlnode_LastChild));
	xmlNodeVec.push_back(HostFunction("IterateChildren", xmlnode_IterateChildren));
	xmlNodeVec.push_back(HostFunction("InsertEndChild", xmlnode_InsertEndChild));
	xmlNodeVec.push_back(HostFunction("LinkEndChild", xmlnode_LinkEndChild));
	xmlNodeVec.push_back(HostFunction("InsertBeforeChild", xmlnode_InsertBeforeChild));
	xmlNodeVec.push_back(HostFunction("InsertAfterChild", xmlnode_InsertAfterChild));
	xmlNodeVec.push_back(HostFunction("ReplaceChild", xmlnode_ReplaceChild));
	xmlNodeVec.push_back(HostFunction("RemoveChild", xmlnode_RemoveChild));
	xmlNodeVec.push_back(HostFunction("PreviousSibling", xmlnode_PreviousSibling));
	xmlNodeVec.push_back(HostFunction("NextSiblingElement", xmlnode_NextSiblingElement));
	xmlNodeVec.push_back(HostFunction("NextSibling", xmlnode_NextSibling));

	xmlNodeVec.push_back(HostFunction("ChildNodeCount", xmlnode_ChildNodeCount));
	xmlNodeVec.push_back(HostFunction("ChildElementCount", xmlnode_ChildElementCount));

	xmlNodeVec.push_back(HostFunction("FirstChildElement", xmlnode_FirstChildElement));
	xmlNodeVec.push_back(HostFunction("GetDocument", xmlnode_GetDocument));
	xmlNodeVec.push_back(HostFunction("NoChildren", xmlnode_NoChildren));
	xmlNodeVec.push_back(HostFunction("CreateText", xmlnode_CreateText));
	gScriptVM.RegisterUserClass("XmlNode", NULL, xmlNodeVec);

	std::vector<HostFunction> xmlElementVec;
	xmlElementVec.push_back(HostFunction("Attribute", xmlelement_Attribute));
	xmlElementVec.push_back(HostFunction("SetAttribute", xmlelement_SetAttribute));
	xmlElementVec.push_back(HostFunction("RemoveAttribute", xmlelement_RemoveAttribute));
	xmlElementVec.push_back(HostFunction("GetText", xmlelement_GetText));
	xmlElementVec.push_back(HostFunction("CreateElement", xmlelement_CreateElement));
	gScriptVM.RegisterUserClass("XmlElement", "XmlNode", xmlElementVec);

	std::vector<HostFunction> xmlDocumentVec;
	xmlDocumentVec.push_back(HostFunction("OpenFile", xmldocument_OpenFile));
	xmlDocumentVec.push_back(HostFunction("SaveFile", xmldocument_SaveFile));
	xmlDocumentVec.push_back(HostFunction("RootElement", xmldocument_RootElement));
	xmlDocumentVec.push_back(HostFunction("CreateDocument", xmldocument_CreateDocument));
	gScriptVM.RegisterUserClass("XmlDocument", "XmlNode", xmlDocumentVec);
}

void xmlnode_Clear(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_Clear, 0, xmlNode, TiXmlNode, "XmlNode");
	xmlNode->Clear();
}

void xmlnode_Parent(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_Parent, 0, xmlNode, TiXmlNode, "XmlNode");
	vm->setReturnAsUserData("XmlNode", xmlNode->Parent());
}

void xmlnode_FirstChild(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_FirstChild, 0, xmlNode, TiXmlNode, "XmlNode");
	if (vm->getNumParam() > 1)
	{
		CheckParam(xmlnode_FirstChild, 1 , value, OP_TYPE_STRING);
		vm->setReturnAsUserData("XmlNode", xmlNode->FirstChild(stringRawValue(&value)));
	}
	else
	{
		vm->setReturnAsUserData("XmlNode", xmlNode->FirstChild());
	}
}

void xmlnode_LastChild(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_LastChild, 0, xmlNode, TiXmlNode, "XmlNode");
	if (vm->getNumParam() > 1)
	{
		CheckParam(xmlnode_LastChild, 1, value, OP_TYPE_STRING);
		vm->setReturnAsUserData("XmlNode", xmlNode->LastChild(stringRawValue(&value)));
	}
	else
	{
		vm->setReturnAsUserData("XmlNode", xmlNode->LastChild());
	}
}

void xmlnode_IterateChildren(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_IterateChildren, 0, xmlNode, TiXmlNode, "XmlNode");
	if (vm->getNumParam() > 2)
	{
		CheckParam(xmlnode_IterateChildren, 1, value, OP_TYPE_STRING);
		CheckUserTypeParam(xmlnode_IterateChildren, 2, previousNode, TiXmlNode, "XmlNode");
		vm->setReturnAsUserData("XmlNode", xmlNode->IterateChildren(stringRawValue(&value), previousNode));
	}
	else
	{
		CheckUserTypeParam(xmlnode_IterateChildren, 1, previousNode, TiXmlNode, "XmlNode");
		vm->setReturnAsUserData("XmlNode", xmlNode->IterateChildren(previousNode));
	}
}

void xmlnode_InsertEndChild(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_InsertEndChild, 0, xmlNode, TiXmlNode, "XmlNode");
	CheckUserTypeParam(xmlnode_InsertEndChild, 1, addThis, TiXmlNode, "XmlNode");
	vm->setReturnAsUserData("XmlNode", xmlNode->InsertEndChild(*addThis));
}

void xmlnode_LinkEndChild(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_LinkEndChild, 0, xmlNode, TiXmlNode, "XmlNode");
	CheckUserTypeParam(xmlnode_LinkEndChild, 1, addThis, TiXmlNode, "XmlNode");
	vm->setReturnAsUserData("XmlNode", xmlNode->LinkEndChild(addThis));
}

void xmlnode_InsertBeforeChild(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_InsertBeforeChild, 0, xmlNode, TiXmlNode, "XmlNode");
	CheckUserTypeParam(xmlnode_InsertBeforeChild, 1, before, TiXmlNode, "XmlNode");
	CheckUserTypeParam(xmlnode_InsertBeforeChild, 2, addThis, TiXmlNode, "XmlNode");
	vm->setReturnAsUserData("XmlNode", xmlNode->InsertBeforeChild(before, *addThis));
}

void xmlnode_InsertAfterChild(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_InsertAfterChild, 0, xmlNode, TiXmlNode, "XmlNode");
	CheckUserTypeParam(xmlnode_InsertAfterChild, 1, afterThis, TiXmlNode, "XmlNode");
	CheckUserTypeParam(xmlnode_InsertAfterChild, 2, addThis, TiXmlNode, "XmlNode");
	vm->setReturnAsUserData("XmlNode", xmlNode->InsertAfterChild(afterThis, *addThis));
}

void xmlnode_ReplaceChild(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_ReplaceChild, 0, xmlNode, TiXmlNode, "XmlNode");
	CheckUserTypeParam(xmlnode_ReplaceChild, 1, replaceThis, TiXmlNode, "XmlNode");
	CheckUserTypeParam(xmlnode_ReplaceChild, 2, withThis, TiXmlNode, "XmlNode");
	vm->setReturnAsUserData("XmlNode", xmlNode->ReplaceChild(replaceThis, *withThis));
}

void xmlnode_RemoveChild(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_RemoveChild, 0, xmlNode, TiXmlNode, "XmlNode");
	CheckUserTypeParam(xmlnode_RemoveChild, 1, removeThis, TiXmlNode, "XmlNode");
	vm->setReturnAsInt(xmlNode->RemoveChild(removeThis));
}

void xmlnode_PreviousSibling(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_PreviousSibling, 0, xmlNode, TiXmlNode, "XmlNode");
	if (vm->getNumParam() > 1)
	{
		CheckParam(xmlnode_PreviousSibling, 1, prev, OP_TYPE_STRING);
		vm->setReturnAsUserData("XmlNode", xmlNode->PreviousSibling(stringRawValue(&prev)));

	}
	else
	{
		vm->setReturnAsUserData("XmlNode", xmlNode->PreviousSibling());
	}
}

void xmlnode_NextSiblingElement(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_NextSiblingElement, 0, xmlNode, TiXmlNode, "XmlNode");
	if (vm->getNumParam() > 1)
	{
		CheckParam(xmlnode_NextSiblingElement, 1, prev, OP_TYPE_STRING);
		vm->setReturnAsUserData("XmlElement", xmlNode->NextSiblingElement(stringRawValue(&prev)));

	}
	else
	{
		vm->setReturnAsUserData("XmlElement", xmlNode->NextSiblingElement());
	}
}

void xmlnode_ChildNodeCount(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_ChildNodeCount, 0, xmlNode, TiXmlNode, "XmlNode");

	int count = 0;

	const char* szChildName = NULL;
	if (vm->getNumParam() > 1)
	{
		CheckParam(xmlnode_NextSiblingElement, 1, name, OP_TYPE_STRING);
		szChildName = stringRawValue(&name);

		TiXmlNode* firstChild = xmlNode->FirstChild(szChildName);
		while (firstChild != NULL)
		{
			count++;
			firstChild = firstChild->NextSibling(szChildName);
		}
	}
	else
	{
		TiXmlNode* firstChild = xmlNode->FirstChild();
		while (firstChild != NULL)
		{
			count++;
			firstChild = firstChild->NextSibling();
		}
	}
	
	vm->setReturnAsInt(count);
}

void xmlnode_ChildElementCount(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_ChildElementCount, 0, xmlNode, TiXmlNode, "XmlNode");

	int count = 0;

	const char* szChildName = NULL;
	if (vm->getNumParam() > 1)
	{
		CheckParam(xmlnode_ChildElementCount, 1, name, OP_TYPE_STRING);
		szChildName = stringRawValue(&name);

		TiXmlNode* firstChild = xmlNode->FirstChildElement(szChildName);
		while (firstChild != NULL)
		{
			count++;
			firstChild = firstChild->NextSiblingElement(szChildName);
		}
	}
	else
	{
		TiXmlNode* firstChild = xmlNode->FirstChildElement();
		while (firstChild != NULL)
		{
			count++;
			firstChild = firstChild->NextSiblingElement();
		}
	}

	vm->setReturnAsInt(count);
}

void xmlnode_NextSibling(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_NextSibling, 0, xmlNode, TiXmlNode, "XmlNode");
	if (vm->getNumParam() > 1)
	{
		CheckParam(xmlnode_NextSibling, 1, prev, OP_TYPE_STRING);
		vm->setReturnAsUserData("XmlElement", xmlNode->NextSibling(stringRawValue(&prev)));

	}
	else
	{
		vm->setReturnAsUserData("XmlElement", xmlNode->NextSibling());
	}
}

void xmlnode_FirstChildElement(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_FirstChildElement, 0, xmlNode, TiXmlNode, "XmlNode");
	if (vm->getNumParam() > 1)
	{
		CheckParam(xmlnode_FirstChildElement, 1, prev, OP_TYPE_STRING);
		vm->setReturnAsUserData("XmlElement", xmlNode->FirstChildElement(stringRawValue(&prev)));

	}
	else
	{
		vm->setReturnAsUserData("XmlElement", xmlNode->FirstChildElement());
	}
}

void xmlnode_GetDocument(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_GetDocument, 0, xmlNode, TiXmlNode, "XmlNode");
	vm->setReturnAsUserData("XmlDocument", xmlNode->GetDocument());
}

void xmlnode_NoChildren(XScriptVM* vm)
{
	CheckUserTypeParam(xmlnode_NoChildren, 0, xmlNode, TiXmlNode, "XmlNode");
	vm->setReturnAsInt(xmlNode->NoChildren());
}

void xmlnode_CreateText(XScriptVM* vm)
{
	CheckParam(xmlnode_CreateText, 1, name, OP_TYPE_STRING);
	TiXmlText* text = new TiXmlText(stringRawValue(&name));
	vm->setReturnAsUserData("XmlNode", text);
}


void xmlelement_Attribute(XScriptVM* vm)
{
	CheckUserTypeParam(xmlelement_Attribute, 0, xmlElement, TiXmlElement, "XmlElement");
	CheckParam(xmlelement_Attribute, 1, name, OP_TYPE_STRING);
	vm->setReturnAsStr(xmlElement->Attribute(stringRawValue(&name)));
}


void xmlelement_RemoveAttribute(XScriptVM* vm)
{
	CheckUserTypeParam(xmlelement_RemoveAttribute, 0, xmlElement, TiXmlElement, "XmlElement");
	CheckParam(xmlelement_RemoveAttribute, 1, name, OP_TYPE_STRING);
	xmlElement->RemoveAttribute(stringRawValue(&name));
}

void xmlelement_SetAttribute(XScriptVM* vm)
{
	CheckUserTypeParam(xmlelement_SetAttribute, 0, xmlElement, TiXmlElement, "XmlElement");
	CheckParam(xmlelement_SetAttribute, 1, name, OP_TYPE_STRING);
	CheckParam(xmlelement_SetAttribute, 2, value, OP_TYPE_STRING);
	xmlElement->SetAttribute(stringRawValue(&name), stringRawValue(&value));

}

void xmlelement_GetText(XScriptVM* vm)
{
	CheckUserTypeParam(xmlelement_SetAttribute, 0, xmlElement, TiXmlElement, "XmlElement");
	vm->setReturnAsStr(xmlElement->GetText());
}

void xmldocument_OpenFile(XScriptVM* vm)
{
	TiXmlDocument* doc = new TiXmlDocument();
	CheckParam(xmldocument_OpenFile, 1, name, OP_TYPE_STRING);
	CheckParam(xmldocument_OpenFile, 2, encode, OP_TYPE_INT);
	if (doc->LoadFile(stringRawValue(&name), (TiXmlEncoding)encode.iIntValue))
	{
		vm->setReturnAsUserData("XmlDocument", doc);
	}
	else
	{
		vm->setReturnAsNil(0);
		vm->setReturnAsStr(doc->ErrorDesc(), 1);
		delete doc;
	}
}

void xmlelement_CreateElement(XScriptVM* vm)
{
	CheckParam(xmlelement_CreateElement, 1, name, OP_TYPE_STRING);
	TiXmlElement* element = new TiXmlElement(stringRawValue(&name));
	vm->setReturnAsUserData("XmlElement", element);
}

void xmldocument_CreateDocument(XScriptVM* vm)
{
	TiXmlDocument* doc = new TiXmlDocument();
	vm->setReturnAsUserData("XmlDocument", doc);
}

void xmldocument_SaveFile(XScriptVM* vm)
{
	CheckUserTypeParam(xmldocument_SaveFile, 0, xmlDoc, TiXmlDocument, "XmlDocument");
	CheckParam(xmldocument_SaveFile, 1, name, OP_TYPE_STRING);
	vm->setReturnAsInt(xmlDoc->SaveFile(stringRawValue(&name)));
}


void xmldocument_RootElement(XScriptVM* vm)
{
	CheckUserTypeParam(xmldocument_RootElement, 0, xmlDoc, TiXmlDocument, "XmlDocument");
	vm->setReturnAsUserData( "XmlElement", xmlDoc->RootElement());
}