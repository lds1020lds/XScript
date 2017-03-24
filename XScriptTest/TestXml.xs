
function TestReadXml()
{
	f = XmlDocument:OpenFile("test.xml", 1);
	dataXml = f:FirstChild("Data");
	serverXml = dataXml:FirstChild("Server");
	qzoneServerElem = serverXml:FirstChildElement("QQServer");
	serverInfo = qzoneServerElem:FirstChildElement("HostInfo");
	host = serverInfo:Attribute("Host");
	port = serverInfo:Attribute("Port");
	timeout = serverInfo:Attribute("Timeout");
	printf(host, string.utf8togbk(port), timeout);
}

function TestCreateXml()
{
	f = XmlDocument:CreateDocument();
	rootElement = XmlElement:CreateElement("Persons");
	f:LinkEndChild(rootElement);
	personElement = XmlElement:CreateElement("Person");
	rootElement:LinkEndChild(personElement);
	personElement:SetAttribute("ID", "1");
	nameElement = XmlElement:CreateElement("Name");
	ageElement = XmlElement:CreateElement("Age");
	personElement:LinkEndChild(nameElement);
	personElement:LinkEndChild(ageElement);
	
	nameText = XmlNode:CreateText("周星驰");
	ageText = XmlNode:CreateText("40");
	nameElement:LinkEndChild(nameText);
	ageElement:LinkEndChild(ageText);
	
	f:SaveFile("test2.xml");
}

function loadXmlFile()
{
	var xmlFile = XmlDocument:OpenFile("L_101.npCourse", 1);
	sceneLayerList = xmlFile:FirstChild("SceneLayerList");
	firstSceneLayer = sceneLayerList:FirstChildElement("SceneLayer");
	
	var mapX, mapY, mapH, mapW = 0,0,0,0;
	var layerCount = 0;
	var layer = nil;
	layer = firstSceneLayer;
	while (layer != nil)
	{
		layerCount++;
		x=string.atoi(layer:Attribute("X"))
		y=string.atoi(layer:Attribute("Y"))
		
		cellR= string.atoi(layer:Attribute("CellR"));
		cellC= string.atoi(layer:Attribute("CellC"));
		cellW= string.atoi(layer:Attribute("CellW"));
		cellH= string.atoi(layer:Attribute("CellH"));
		
		if (mapX > x)
			mapX = x;
		if (mapY > y)
			mapY = y;
		if(mapW < cellC*cellW)
			mapW = (cellC*cellW);
		if (mapH < cellR*cellH)
			mapH = (cellR*cellH);
			
		layer = layer:NextSiblingElement("SceneLayer");
	}
	
	var levelFile = File:open("level.bin", "w");
	levelFile:write(struct.pack("h4i", layerCount, mapX, mapY, mapW, mapH));
	
	layer = firstSceneLayer;
	while (layer != nil)
	{
		var childCount = layer:ChildElementCount("NP2DSFrameRef");
		layerName = layer:Attribute("Name")
		layerX = string.atoi(layer:Attribute("X"))
		layerY = string.atoi(layer:Attribute("Y"))
		printf(layerName)
		levelFile:write(struct.pack("i",string.len(layerName)))
		levelFile:write(struct.pack(toString(string.len(layerName))$"s",layerName))
		levelFile:write(struct.pack("h2i",childCount, layerX, layerY))

		firstRef = layer:FirstChildElement("NP2DSFrameRef");
		while (firstRef != nil)
		{
			ref = firstRef;
			firstRef = firstRef:NextSiblingElement("NP2DSFrameRef");

			var worldTrans = ref:FirstChildElement("WorldTrans")
			var worldScale = ref:FirstChildElement("WorldScale")
			
			groupID = 0
			resfile = string.atoi(ref:Attribute("Resfile"))
			frameID = string.atoi(ref:Attribute("Frameid"))
			
			transX = string.atoi(worldTrans:Attribute("x"))
			transY = string.atoi(worldTrans:Attribute("y"))
			scaleX = string.atof(worldScale:Attribute("x"))
			scaleY = string.atof(worldScale:Attribute("y"))
			levelFile:write(struct.pack("5i2f", groupID, resfile,frameID, transX, transY, scaleX, scaleY))
			

			var firstSceneParamList = ref:FirstChildElement("SceneParamList");
			if (firstSceneParamList != nil)
			{
				var sceneParamNum = firstSceneParamList:ChildElementCount("SceneParam");
				levelFile:write(struct.pack("h", sceneParamNum));
			
				var firstSceneParam = firstSceneParamList:FirstChildElement("SceneParam");
				while( firstSceneParam != nil )
				{
					var paramName = firstSceneParam:Attribute("Name");
					var paramText = firstSceneParam:Attribute("Text");
					
					levelFile:write(struct.pack("i",string.len(paramName)))
					levelFile:write(struct.pack(toString(string.len(paramName))$"s",paramName))
					
					levelFile:write(struct.pack("i",string.len(paramText)))
					levelFile:write(struct.pack(toString(string.len(paramText))$"s",paramText))
					
					firstSceneParam = firstSceneParam:NextSiblingElement("SceneParam");
				}
				
				
			}
			else
			{
				levelFile:write(struct.pack("h", 0));
			}
		}
		
		layer = layer:NextSiblingElement("SceneLayer");
	}
	
	levelFile:close();
}

loadXmlFile();

/*
TestReadXml();
TestCreateXml();
*/
system_pause();

