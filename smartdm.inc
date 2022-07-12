stock int Downloader_GetMaterialsFromMDL(const char[] model, char[][] files, int maxsize, int maxlen)
{
	if(!FileExists(model))
	{
		return 0;
	}

	int iNum = 0;

	Handle hFile = OpenFile(model, "rb");

	FileSeek(hFile, 204, SEEK_SET);
	ReadFileCell(hFile, iNum, 4);

	FileSeek(hFile, 0, SEEK_END);

	int iChar;
	int iPos;
	do
	{
		FileSeek(hFile, -2, SEEK_CUR);
		ReadFileCell(hFile, iChar, 1);
	} while (iChar == 0);

	FileSeek(hFile, 1, SEEK_CUR);
	
	char szPath[PLATFORM_MAX_PATH];
	do
	{
		FileSeek(hFile, -2, SEEK_CUR);
		ReadFileCell(hFile, iChar, 1);
	} while(iChar != 0);

	iPos = FilePosition(hFile);
	ReadFileString(hFile, szPath, sizeof(szPath));
	FileSeek(hFile, iPos, SEEK_SET);

	int iRet;
	char szFile[PLATFORM_MAX_PATH];
	for(iRet = 0; iRet < iNum; ++iRet)
	{
		if(iNum == maxsize)
		{
			break;
		}

		FileSeek(hFile, -1, SEEK_CUR);

		do
		{
			FileSeek(hFile, -2, SEEK_CUR);
			ReadFileCell(hFile, iChar, 1);
		} while(iChar != 0);
		
		iPos = FilePosition(hFile);
		ReadFileString(hFile, szFile, sizeof(szFile));
		// LogError("MDL: %s - %s", szPath, szFile);
		Format(files[iRet], maxlen, "materials\\%s%s.vmt", szPath, szFile);
		FileSeek(hFile, iPos, SEEK_SET);
	}

	CloseHandle(hFile);

	return iRet;
}

static const char g_szModelExts[][16] = {".phy", ".sw.vtx", ".dx80.vtx", ".dx90.vtx", ".vtx", ".xbox.vtx", ".vvd", ".ani"};
stock int Downloader_GetModelFiles(char[] model, char[][] files, int maxsize, int maxlen)
{
	char szRawPath[PLATFORM_MAX_PATH];
	strcopy(szRawPath, sizeof(szRawPath), model);
	int iDot = FindCharInString(szRawPath, '.', true);
	if(iDot == -1)
	{
		return 0;
	}

	szRawPath[iDot] = 0;

	int iNum = 0;
	for(int i = 0; i < sizeof(g_szModelExts); ++i)
	{
		if(iNum == maxsize)
		{
			break;
		}

		Format(files[iNum], maxlen, "%s%s", szRawPath, g_szModelExts[i]);
		if(FileExists(files[iNum]))
		{
			++iNum;
		}
	}
	return iNum;
}

static const char g_szMaterialKeys[][64] = {"$baseTexture", "$bumpmap", "$lightwarptexture"};
stock int Downloader_GetMaterialsFromVMT(char[] vmt, char[][] materials, int maxsize, int maxlen)
{
	if(!FileExists(vmt))
	{
		return 0;
	}

	char szLine[512];

	Handle hFile = OpenFile(vmt, "r");
	
	bool bFound[sizeof(g_szMaterialKeys)];
	int iPos;
	int iLast;
	int iNum = 0;
	while(ReadFileLine(hFile, szLine, sizeof(szLine)) != false)
	{
		if(iNum == sizeof(g_szMaterialKeys) || maxsize == iNum)
		{
			break;
		}

		for(int i = 0; i < sizeof(g_szMaterialKeys); ++i)
		{
			if(bFound[i])
			{
				continue;
			}

			if((iPos = StrContains(szLine, g_szMaterialKeys[i], false)) > 0)
			{
				bFound[i]=true;

				while(szLine[iPos] != '"' && szLine[iPos] != ' ' && szLine[iPos] != '	')
				{
					++iPos;
				}

				while(szLine[iPos] == ' ' || szLine[iPos] == '	' || szLine[iPos] == '"')
				{
					++iPos;
				}

				iLast = iPos;
				while(szLine[iLast] != '"' && szLine[iLast] != '\r' && szLine[iLast] != '\n' && szLine[iLast] != ' ' && szLine[iLast] != '	' && szLine[iLast] != 0)
				{
					++iLast;
				}
					
				szLine[iLast] = 0;
				strcopy(materials[iNum], maxlen, szLine[iPos]);
				++iNum;
			}
		}
	}

	CloseHandle(hFile);

	return iNum;
}

stock int Downloader_AddFileToDownloadsTable(ArrayList hCachedFiles, StringMap hCachedNums, char[] filename)
{
	if(!FileExists(filename))
	{
		return 0;
	}

	if (!hCachedFiles)
	{
		// hCachedFiles = CreateArray(PLATFORM_MAX_PATH);
		LogError("Downloader_AddFileToDownloadsTable: Invalid ArrayList handle!");
		return 0;
	}

	if(!hCachedNums)
	{
		// hCachedNums = CreateTrie();
		LogError("Downloader_AddFileToDownloadsTable: Invalid StringMap handle!");
		return 0;
	}

	AddFileToDownloadsTable(filename);

	int iValue;
	if(GetTrieValue(hCachedNums, filename, iValue))
	{
		int iStart = FindStringInArray(hCachedFiles, filename) + 1;
		char szFile[PLATFORM_MAX_PATH];
		for(int i = (iStart - iValue -1); i < iStart - 1; ++i)
		{
			GetArrayString(hCachedFiles, i, szFile, sizeof(szFile));

			AddFileToDownloadsTable(szFile);
		}
		return 0;
	}

	char szExt[16];
	int iDot = FindCharInString(filename, '.', true);
	if(iDot == -1)
	{
		return 0;
	}

	int iNumFiles = 0;

	strcopy(szExt, sizeof(szExt), filename[iDot]);
	char szMaterials[32][PLATFORM_MAX_PATH];
	int iNum;
	if(strcmp(szExt, ".mdl") == 0)
	{
		// LogError("Downloader_AddFileToDownloadsTable: Is mdl: %s!", filename);
		
		char szFiles[sizeof(g_szModelExts)][PLATFORM_MAX_PATH];
		iNum = Downloader_GetModelFiles(filename, szFiles, sizeof(szFiles), sizeof(szFiles[]));
		for(int i = 0; i < iNum; ++i)
		{
			// LogError("Downloader_GetModelFiles: %s!", szFiles[i]);
			iNumFiles += Downloader_AddFileToDownloadsTable(hCachedFiles, hCachedNums, szFiles[i]) + 1;
		}
		
		iNum = Downloader_GetMaterialsFromMDL(filename, szMaterials, sizeof(szMaterials), sizeof(szMaterials[]));
		for(int i = 0; i < iNum; ++i)
		{
			// LogError("Downloader_GetModelFiles: %s!", szMaterials[i]);
			if(FileExists(szMaterials[i]))
			{
				// LogError("Downloader_GetModelFiles (Exist): %s!", szMaterials[i]);
				iNumFiles += Downloader_AddFileToDownloadsTable(hCachedFiles, hCachedNums, szMaterials[i]) + 1;
			}
		}
	}
	else if(strcmp(szExt, ".vmt") == 0)
	{
		iNum = Downloader_GetMaterialsFromVMT(filename, szMaterials, sizeof(szMaterials), sizeof(szMaterials[]));
		char szMaterial[PLATFORM_MAX_PATH];
		for(int i = 0; i < iNum; ++i)
		{
			Format(szMaterial, sizeof(szMaterial), "materials\\%s.vtf", szMaterials[i]);
			if(FileExists(szMaterial))
			{
				iNumFiles += Downloader_AddFileToDownloadsTable(hCachedFiles, hCachedNums, szMaterial)+1;
			}
		}
	}

	PushArrayString(hCachedFiles, filename);
	SetTrieValue(hCachedNums, filename, iNumFiles);

	return iNumFiles;
}
