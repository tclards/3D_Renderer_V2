#include <d3dcompiler.h>	// required for compiling shaders on the fly, consider pre-compiling instead
#pragma comment(lib, "d3dcompiler.lib") 
#include "renderer.h"

void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}

// Globals for Renderer - input bools and 2D render pass/HUD & UI data
#pragma region Globals
// Bools for input - splitscreen, level loading, and wireframe mode
bool splitScreen = false;
bool startCounter = false;
int splitCounter = 30;
bool wireFrameMode = false;
bool startWireframeCounter = false;
int wireframeInputCounter = 30;
int levelIndex = 0;
bool firstLoadingPass = true;
bool orthoMode = false;
bool startOrthoCounter = false;
int orthoCounter = 30;
bool debugCamera = false;
int debugFreeCamInputCounter = 30;
bool startDebugFreeCamCounter = false;
float debugFreeCamMoveSpeedModifier = 0.000055f;

// array of floats representing the information for the quad
// first two values are the position [x,y]
// next two values are the uvs [u,v]
float verts[] =
{
	-1.0f,  1.0f, 0.0f, 0.0f,		//[x,y,u,v]
	 1.0f,  1.0f, 1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f, 1.0f,
	 1.0f, -1.0f, 1.0f, 1.0f
};

// array of unsigned ints representing the information for ordering our vertices
// into the shape of a quad (square)
unsigned int indices[] =
{
	0, 1, 2,
	1, 3, 2
};

// an few arrays to store all of the texture names for various UI/HUD elements
// this makes looping over and creating shader resource views easier
std::wstring texture_names[] =		// example HUD from original implementation
{
	L"HUD_Sharp_backplate.dds",
	L"Health_left.dds",
	L"Health_right.dds",
	L"Mana_left.dds",
	L"Mana_right.dds",
	L"Center_top.dds",
	L"font_consolas_32.dds"
};
#pragma endregion

// Constructor
DirectXRendererLogic::DirectXRendererLogic(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX11Surface _d3d)
{
	window = _win;
	d3dSurface = _d3d;

	InitializeSound();
	loadLevel();
	InitializeMatricesAndVariables();
	InitializeConstantBufferData();
	IntializeGraphics();

	inputProxy.Create(window);
	controllerInputProxy.Create();
}

// Shutdown Function to release resources - TODO
bool DirectXRendererLogic::Shutdown()
{
	return true; // !
}

// Level Loading
void DirectXRendererLogic::loadLevel()
{
	const char* levelToLoad = levels[levelIndex];

	// begin loading level
	log.Create("../LevelLoaderLog.txt");
	log.EnableConsoleLogging(true); // mirror output to the console
	log.Log("Start Program.");

	loadedLevel.UnloadLevel();
	loadedLevel.LoadLevel(levelToLoad, "../../Assets/Level_Assets", log.Relinquish());

	ID3D11Device* creator;
	d3dSurface.GetDevice((void**)&creator);
	ReInitializeBuffers(creator);
	InitializeConstantBufferData();
	cbuffMesh.Reset();
	cbuffScene.Reset();
	InitializeConstantBuffers(creator);

	bool isPlayingFX;
	loadingFX.isPlaying(isPlayingFX);
	if (isPlayingFX == false)
	{
		loadingFX.Play();
	}
	else if (isPlayingFX == true)
	{
		loadingFX.Stop();
		loadingFX.Play();
	}
}

// Helper Functions for intializing renderer and creating required components
#pragma region Helpers
// 2D Helpers
std::vector<Sprite> DirectXRendererLogic::LoadHudFromXML(std::string filepath)
{
	std::vector<Sprite> result;

	tinyxml2::XMLDocument document;
	tinyxml2::XMLError error_message = document.LoadFile(filepath.c_str());
	if (error_message != tinyxml2::XML_SUCCESS)
	{
		std::cout << "XML file [" + filepath + "] did not load properly." << std::endl;
		return std::vector<Sprite>();
	}

	std::string name = document.FirstChildElement("hud")->FindAttribute("name")->Value();
	GW::MATH2D::GVECTOR2F screen_size;
	screen_size.x = atof(document.FirstChildElement("hud")->FindAttribute("width")->Value());
	screen_size.y = atof(document.FirstChildElement("hud")->FindAttribute("height")->Value());

	tinyxml2::XMLElement* current = document.FirstChildElement("hud")->FirstChildElement("element");
	while (current)
	{
		Sprite s = Sprite();
		name = current->FindAttribute("name")->Value();
		float x = atof(current->FindAttribute("pos_x")->Value());
		float y = atof(current->FindAttribute("pos_y")->Value());
		float sx = atof(current->FindAttribute("scale_x")->Value());
		float sy = atof(current->FindAttribute("scale_y")->Value());
		float r = atof(current->FindAttribute("rotation")->Value());
		float d = atof(current->FindAttribute("depth")->Value());
		GW::MATH2D::GVECTOR2F s_min, s_max;
		s_min.x = atof(current->FindAttribute("sr_x")->Value());
		s_min.y = atof(current->FindAttribute("sr_y")->Value());
		s_max.x = atof(current->FindAttribute("sr_w")->Value());
		s_max.y = atof(current->FindAttribute("sr_h")->Value());
		unsigned int tid = atoi(current->FindAttribute("textureID")->Value());

		s.SetName(name);
		s.SetScale(sx, sy);
		s.SetPosition(x, y);
		s.SetRotation(r);
		s.SetDepth(d);
		s.SetScissorRect({ s_min, s_max });
		s.SetTextureIndex(tid);

		result.push_back(s);

		current = current->NextSiblingElement();
	}
	return result;
}
void DirectXRendererLogic::loadHUD()
{
	// load a hud.xml file that contains all of the hud information
	// [sprite data]
	std::string filepath = XML_PATH;
	filepath += "hud.xml";
	HUD xml_items = LoadHudFromXML(filepath);
	// insert the xml items into the hud vector
	hud.insert(hud.end(), xml_items.begin(), xml_items.end());
	// sorting lambda based on depth of sprites
	auto sortfunc = [=](const Sprite& a, const Sprite& b)
		{
			return a.GetDepth() > b.GetDepth();
		};
	// sort the hud from furthest to closest
	std::sort(hud.begin(), hud.end(), sortfunc);
}
DirectXRendererLogic::SpriteData DirectXRendererLogic::UpdateSpriteConstantBufferData(const Sprite& s)
{
	SpriteData temp = { 0 };
	temp.pos_scale.x = s.GetPosition().x;
	temp.pos_scale.y = s.GetPosition().y;
	temp.pos_scale.z = s.GetScale().x;
	temp.pos_scale.w = s.GetScale().y;
	temp.rotation_depth.x = s.GetRotation();
	temp.rotation_depth.y = s.GetDepth();
	return temp;
}
DirectXRendererLogic::SpriteData DirectXRendererLogic::UpdateTextConstantBufferData(const Text& s)
{
	SpriteData temp = { 0 };
	temp.pos_scale.x = s.GetPosition().x;
	temp.pos_scale.y = s.GetPosition().y;
	temp.pos_scale.z = s.GetScale().x;
	temp.pos_scale.w = s.GetScale().y;
	temp.rotation_depth.x = s.GetRotation();
	temp.rotation_depth.y = s.GetDepth();
	return temp;
}
// Graphics and Sound
void DirectXRendererLogic::IntializeGraphics()
{
	ID3D11Device* creator;
	d3dSurface.GetDevice((void**)&creator);

	InitializeVertexBuffer(creator);
	InitializeIndexBuffer(creator);
	InitializeConstantBuffers(creator);
	InitializePipeline(creator);
	InitializeWireframeMode(creator);
	CreateBlendState(creator);
	CreateDepthStencilDesc(creator);
	CreateRasterState_2D(creator);

	// free temporary handle
	creator->Release();
}
void DirectXRendererLogic::InitializeSound()
{
	audioPlayer.Create();

	loadingFX.Create(loadingSound, audioPlayer, 0.1f);
	music.Create(backgroundMusic, audioPlayer, 0.2f);
}
// WireFrame Rendering Mode Raster state
void DirectXRendererLogic::InitializeWireframeMode(ID3D11Device* creator)
{
	ZeroMemory(&wfdesc, sizeof(D3D11_RASTERIZER_DESC));
	wfdesc.FillMode = D3D11_FILL_WIREFRAME;
	wfdesc.CullMode = D3D11_CULL_NONE;
	hr_wireframe = creator->CreateRasterizerState(&wfdesc, &WireFrame);
}
// Scene and Mesh Constant Buffers
void DirectXRendererLogic::InitializeConstantBufferData()
{
	cbuffSceneData.projMat = projectionMat;
	cbuffSceneData.viewMat = viewMat;
	cbuffSceneData.lightDir = lightDir;
	cbuffSceneData.lightColor = lightColor;
	cbuffSceneData.sunAmbient = sunAmbient;
}
void DirectXRendererLogic::InitializeConstantBuffers(ID3D11Device* creator)
{
	CreateConstantBufferScene(creator, &cbuffSceneData, sizeof(cbuffSceneData));
	CreateConstantBufferMesh(creator, &cbuffMeshData, sizeof(cbuffMeshData));
	createConstantBufferSprites(creator);
}
void DirectXRendererLogic::createConstantBufferSprites(ID3D11Device* creator)
{
	D3D11_SUBRESOURCE_DATA cbData = { &constantBufferSpriteData, 0, 0 };

	CD3D11_BUFFER_DESC cbDesc(sizeof(constantBufferSpriteData), D3D11_BIND_CONSTANT_BUFFER);
	creator->CreateBuffer(&cbDesc, &cbData, cbuffSprite.GetAddressOf());
}
void DirectXRendererLogic::CreateConstantBufferScene(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
{
	D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
	CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	creator->CreateBuffer(&bDesc, &bData, cbuffScene.GetAddressOf());
}
void DirectXRendererLogic::CreateConstantBufferMesh(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
{
	D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
	CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);

	creator->CreateBuffer(&bDesc, &bData, cbuffMesh.GetAddressOf());
}
// Index Buffer
void DirectXRendererLogic::InitializeIndexBuffer(ID3D11Device* creator)
{
	CreateIndexBuffer(creator, loadedLevel.levelIndices.data(), sizeof(unsigned int) * loadedLevel.levelIndices.size());
	CreateIndexBuffer2D(creator);
}
void DirectXRendererLogic::CreateIndexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
{
	D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
	CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_INDEX_BUFFER);

	creator->CreateBuffer(&bDesc, &bData, &indexBuffer);
}
void DirectXRendererLogic::CreateIndexBuffer2D(ID3D11Device* creator)
{
	D3D11_SUBRESOURCE_DATA ibData = { indices, 0, 0 };
	CD3D11_BUFFER_DESC ibDesc(sizeof(indices), D3D11_BIND_INDEX_BUFFER);
	creator->CreateBuffer(&ibDesc, &ibData, indexBuffer_2D.GetAddressOf());
}
// Vertex Buffer
void DirectXRendererLogic::InitializeVertexBuffer(ID3D11Device* creator)
{
	CreateVertexBuffer(creator, loadedLevel.levelVertices.data(), sizeof(H2B::VERTEX) * loadedLevel.levelVertices.size());
	CreateVertexBuffer2D(creator);
}
void DirectXRendererLogic::CreateVertexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
{
	D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
	CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_VERTEX_BUFFER);
	creator->CreateBuffer(&bDesc, &bData, vertexBuffer.GetAddressOf());
}
void DirectXRendererLogic::CreateVertexBuffer2D(ID3D11Device* creator)
{
	D3D11_SUBRESOURCE_DATA vbData = { verts, 0, 0 };
	CD3D11_BUFFER_DESC vbDesc(sizeof(verts), D3D11_BIND_VERTEX_BUFFER);
	creator->CreateBuffer(&vbDesc, &vbData, vertexBuffer_2D.GetAddressOf());
}
// Text and Font Helpers
void DirectXRendererLogic::InitializeTextandFont()
{
	ID3D11Device* creator;
	d3dSurface.GetDevice((void**)&creator);

	unsigned int screenHeight = 0;
	window.GetClientHeight(screenHeight);
	unsigned int screenWidth = 0;
	window.GetClientWidth(screenWidth);

	// font loading
	// credit for generating font texture
	// https://evanw.github.io/font-texture-generator/
	std::string filepath = XML_PATH;
	filepath += "font_consolas_32.xml";
	bool success = consolas32.LoadFromXML(filepath);

	// setting up the static text object with information
	// keep in mind the position will always be the center of the text
	staticText = Text();
	staticText.SetText("Score");
	staticText.SetFont(&consolas32);
	staticText.SetPosition(0.0f, -0.82f);
	staticText.SetScale(0.5f, 0.5f);
	staticText.SetRotation(0.0f);
	staticText.SetDepth(0.01f);

	// update will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	staticText.Update(screenWidth, screenHeight);

	// vertex buffer creation for the staticText
	const auto& staticVerts = staticText.GetVertices();
	D3D11_SUBRESOURCE_DATA svbData = { staticVerts.data(), 0, 0 };
	CD3D11_BUFFER_DESC svbDesc(sizeof(TextVertex) * staticVerts.size(), D3D11_BIND_VERTEX_BUFFER);
	creator->CreateBuffer(&svbDesc, &svbData, vertexBufferStaticText.GetAddressOf());
	//
	// setting up the dynamic text object with information
	// keep in mind the position will always be the center of the text
	dynamicText = Text();
	dynamicText.SetFont(&consolas32);
	dynamicText.SetPosition(0.0f, -0.88f);
	dynamicText.SetScale(0.5f, 0.5f);
	dynamicText.SetRotation(0.0f);
	dynamicText.SetDepth(0.01f);

	CD3D11_BUFFER_DESC dvbDesc(sizeof(TextVertex) * 6 * 5000, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
	creator->CreateBuffer(&dvbDesc, nullptr, vertexBufferDynamicText.GetAddressOf());
	creator->Release();
}
// Reinitialize Buffers - helper function for use in level loading
void DirectXRendererLogic::ReInitializeBuffers(ID3D11Device* creator)
{
	indexBuffer.Reset();
	vertexBuffer.Reset();
	CreateIndexBuffer(creator, loadedLevel.levelIndices.data(), sizeof(unsigned int) * loadedLevel.levelIndices.size());
	CreateVertexBuffer(creator, loadedLevel.levelVertices.data(), sizeof(H2B::VERTEX) * loadedLevel.levelVertices.size());
}
// Initialize Matrices
void DirectXRendererLogic::InitializeWorldMatrix()
{
	GW::MATH::GMATRIXF tempMat = GW::MATH::GIdentityMatrixF;

	worldMat = tempMat;
}
void DirectXRendererLogic::InitializePerspectiveMatrix()
{
	GW::MATH::GMATRIXF tempMat = GW::MATH::GIdentityMatrixF;

	float aspect = 0.0f;
	d3dSurface.GetAspectRatio(aspect);

	float fov = G_DEGREE_TO_RADIAN_F(65.0f);

	GW::MATH::GMatrix::ProjectionDirectXLHF(fov, aspect, 0.1f, 100.0f, tempMat);

	perspectiveMat = tempMat;
}
void DirectXRendererLogic::InitializeOrthographicMatrix()
{
	GW::MATH::GMATRIXF tempMat = GW::MATH::GIdentityMatrixF;

	float fLeft = 5.0f;
	float fRight = -5.0f;
	float fBottom = -5.0f;
	float fTop = 5.0f;
	float fNear = 5.0f;
	float fFar = -5.0f;

	tempMat.row1 = { 2 / (fRight - fLeft), 0, 0, -(fRight + fLeft) / (fRight - fLeft) };
	tempMat.row2 = { 0, 2 / (fTop - fBottom), 0, -(fTop + fBottom) / (fTop - fBottom) };
	tempMat.row3 = { 0, 0, -2 / (fFar - fNear) , -(fFar + fNear) / (fFar - fNear) };
	tempMat.row4 = { 0, 0, 0, 1 };

	orthographicMat = tempMat;
}
void DirectXRendererLogic::InitializeCameraMatrix()
{
	GW::MATH::GMATRIXF tempMat = GW::MATH::GIdentityMatrixF;

	// inverse view into cam
	GW::MATH::GMatrix::InverseF(viewMat, tempMat);

	camMatrix = tempMat;
}
void DirectXRendererLogic::InitializeViewMatrix()
{
	// place viewMat at player position
	GW::MATH::GMATRIXF tempMat = GW::MATH::GIdentityMatrixF;

	viewMat = tempMat;
}
// General Catch all helper that calls many of the above functions
void DirectXRendererLogic::InitializeMatricesAndVariables()
{
	// initialize matrices
	InitializeWorldMatrix();
	InitializeViewMatrix();
	InitializePerspectiveMatrix();
	InitializeOrthographicMatrix();
	InitializeCameraMatrix();

	projectionMat = GW::MATH::GIdentityMatrixF;

	// initialize variables
	lightDir =
	{
		-1.0f, -1.0f, 2.0f, 1
	};
	GW::MATH::GVector::NormalizeF(lightDir, lightDir);
	lightColor =
	{
		229.0f / 255.0f,
		229.0f / 255.0f,
		255.0f / 255.0f,
		255.0f / 255.0f
	};
	sunAmbient =
	{
		63.75f / 255.0f,
		63.75f / 255.0f,
		89.25f / 255.0f,
		0
	};
}
// Pipeline Initialization
void DirectXRendererLogic::InitializePipeline(ID3D11Device* creator)
{
	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
	compilerFlags |= D3DCOMPILE_DEBUG;
#endif
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = CompileVertexShader(creator, compilerFlags);
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob = CompilePixelShader(creator, compilerFlags);


	CreateVertexInputLayout(creator, vsBlob);

	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob_2D = CompileVertexShader_2D(creator, compilerFlags);
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob_2D = CompilePixelShader_2D(creator, compilerFlags);

	CreateVertexInputLayout_2D(creator, vsBlob_2D);
}
void DirectXRendererLogic::InitializePipeline_2D(ID3D11Device* creator)
{
	UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
	compilerFlags |= D3DCOMPILE_DEBUG;
#endif
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob_2D = CompileVertexShader_2D(creator, compilerFlags);
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob_2D = CompilePixelShader_2D(creator, compilerFlags);

	CreateVertexInputLayout_2D(creator, vsBlob_2D);
}
// Shaders
Microsoft::WRL::ComPtr<ID3DBlob> DirectXRendererLogic::CompileVertexShader(ID3D11Device* creator, UINT compilerFlags)
{
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

	HRESULT compilationResult =
		D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.length(),
			nullptr, nullptr, nullptr, "main", "vs_4_0", compilerFlags, 0,
			vsBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(compilationResult))
	{
		creator->CreateVertexShader(vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(), nullptr, vertexShader.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return vsBlob;
}
Microsoft::WRL::ComPtr<ID3DBlob> DirectXRendererLogic::CompilePixelShader(ID3D11Device* creator, UINT compilerFlags)
{
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

	HRESULT compilationResult =
		D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.length(),
			nullptr, nullptr, nullptr, "main", "ps_4_0", compilerFlags, 0,
			psBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(compilationResult))
	{
		creator->CreatePixelShader(psBlob->GetBufferPointer(),
			psBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return psBlob;
}
Microsoft::WRL::ComPtr<ID3DBlob> DirectXRendererLogic::CompileVertexShader_2D(ID3D11Device* creator, UINT compilerFlags)
{
	Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

	HRESULT compilationResult =
		D3DCompile(vertexShaderSource_2D.c_str(), vertexShaderSource_2D.length(),
			nullptr, nullptr, nullptr, "main", "vs_4_0", compilerFlags, 0,
			vsBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(compilationResult))
	{
		creator->CreateVertexShader(vsBlob->GetBufferPointer(),
			vsBlob->GetBufferSize(), nullptr, vertexShader_2D.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return vsBlob;
}
Microsoft::WRL::ComPtr<ID3DBlob> DirectXRendererLogic::CompilePixelShader_2D(ID3D11Device* creator, UINT compilerFlags)
{
	Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

	HRESULT compilationResult =
		D3DCompile(pixelShaderSource_2D.c_str(), pixelShaderSource_2D.length(),
			nullptr, nullptr, nullptr, "main", "ps_4_0", compilerFlags, 0,
			psBlob.GetAddressOf(), errors.GetAddressOf());

	if (SUCCEEDED(compilationResult))
	{
		creator->CreatePixelShader(psBlob->GetBufferPointer(),
			psBlob->GetBufferSize(), nullptr, pixelShader_2D.GetAddressOf());
	}
	else
	{
		PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
		abort();
		return nullptr;
	}

	return psBlob;
}
// Vertex Layout
void DirectXRendererLogic::CreateVertexInputLayout(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob)
{
	D3D11_INPUT_ELEMENT_DESC attributes[3];

	attributes[0].SemanticName = "POS";
	attributes[0].SemanticIndex = 0;
	attributes[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	attributes[0].InputSlot = 0;
	attributes[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	attributes[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	attributes[0].InstanceDataStepRate = 0;

	attributes[1].SemanticName = "UVM";
	attributes[1].SemanticIndex = 0;
	attributes[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	attributes[1].InputSlot = 0;
	attributes[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	attributes[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	attributes[1].InstanceDataStepRate = 0;

	attributes[2].SemanticName = "NRM";
	attributes[2].SemanticIndex = 0;
	attributes[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	attributes[2].InputSlot = 0;
	attributes[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	attributes[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	attributes[2].InstanceDataStepRate = 0;

	creator->CreateInputLayout(attributes, ARRAYSIZE(attributes),
		vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
		vertexFormat.GetAddressOf());
}
void DirectXRendererLogic::CreateVertexInputLayout_2D(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob)
{
	D3D11_INPUT_ELEMENT_DESC format[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// create the directx 11 interface object for the input layout
	creator->CreateInputLayout(format, ARRAYSIZE(format),
		vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
		vertexFormat_2D.GetAddressOf());
}
void DirectXRendererLogic::CreateBlendState(ID3D11Device* creator)
{
	// this is used to alpha blend objects with transparency
	CD3D11_BLEND_DESC blendDesc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	creator->CreateBlendState(&blendDesc, blendState_2D.GetAddressOf());
}
void DirectXRendererLogic::CreateDepthStencilDesc(ID3D11Device* creator)
{
	// the depth function needs to be set to less_equal instead of less
	CD3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	creator->CreateDepthStencilState(&depthStencilDesc, depthStencilState_2D.GetAddressOf());
}
void DirectXRendererLogic::CreateRasterState_2D(ID3D11Device* creator)
{
	CD3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
	rasterizerDesc.ScissorEnable = true;
	creator->CreateRasterizerState(&rasterizerDesc, rasterizerState_2D.GetAddressOf());
}
#pragma endregion

// UI loading function for sprites
void DirectXRendererLogic::loadSprites(ID3D11Device* creator)
{
	for (size_t i = 0; i < ARRAYSIZE(texture_names); i++)
	{
		// create a wide string to store the file path and file name
		std::wstring texturePath = LTEXTURES_PATH;
		texturePath += texture_names[i];
		// load texture from disk 
		DirectX::CreateDDSTextureFromFile(creator, texturePath.c_str(), nullptr, shaderResourceView[i].GetAddressOf());
	}

	CD3D11_SAMPLER_DESC samp_desc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
	creator->CreateSamplerState(&samp_desc, samplerState.GetAddressOf());
}

// Initializes renderer - 
// loads level to render, loads sprite data, font data, and hud data, then initializes the matrices and required variables, sets up the cbuffers, and initializes the graphics systems
void DirectXRendererLogic::InitializeAll()
{
	ID3D11Device* creator;
	d3dSurface.GetDevice((void**)&creator);

	// Initialize
	loadSprites(creator);
	InitializeMatricesAndVariables();
	InitializeConstantBufferData();
	IntializeGraphics();
	InitializeTextandFont();
	loadHUD();
	creator->Release();
}

// Rendering/Drawing Function - called each frame
void DirectXRendererLogic::Render()
{
	PipelineHandles curHandles = GetCurrentPipelineHandles();
	SetUpPipeline(curHandles);
	ID3D11Device* creator;
	d3dSurface.GetDevice((void**)&creator);

	unsigned int screenHeight = 0;
	window.GetClientHeight(screenHeight);
	unsigned int screenWidth = 0;
	window.GetClientWidth(screenWidth);

	D3D11_MAPPED_SUBRESOURCE sub1 = { nullptr };
	D3D11_MAPPED_SUBRESOURCE sub2 = { nullptr };

	// Wireframe Debug Mode
	if (wireFrameMode == true)
	{
		curHandles.context->RSSetState(WireFrame);
	}
	else if (wireFrameMode == false)
	{
		curHandles.context->RSSetState(nullptr);
	}

	// Orthographic mode
	if (orthoMode == true)
	{
		projectionMat = orthographicMat;
	}
	else if (orthoMode == false)
	{
		projectionMat = perspectiveMat;
	}
	cbuffSceneData.projMat = projectionMat;

	// bind scene cbuffer data
	curHandles.context->Map(cbuffScene.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &sub1);
	memcpy(sub1.pData, &cbuffSceneData, sizeof(cbuffSceneData));
	curHandles.context->Unmap(cbuffScene.Get(), 0);

	if (splitScreen == false)
	{
		// default viewport
		D3D11_VIEWPORT vp{};
		vp.Width = screenWidth;
		vp.Height = screenHeight;
		vp.MinDepth = 0;
		vp.MaxDepth = 1;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;

		curHandles.context->RSSetViewports(1u, &vp);

		// loop through all objects in current loaded level and extract needed data
		for (const auto& b : loadedLevel.blenderObjects)
		{
			const int& modelIndex = b.modelIndex;
			const int& transformIndex = b.transformIndex;

			for (unsigned int j = 0; j < loadedLevel.levelModels[modelIndex].meshCount; j++)
			{
				const unsigned int& meshIndex = j + loadedLevel.levelModels[modelIndex].meshStart;
				const H2B::MESH* mesh = &loadedLevel.levelMeshes[meshIndex];
				const unsigned int& matIndex = j + loadedLevel.levelModels[modelIndex].materialStart;
				cbuffMeshData.material = loadedLevel.levelMaterials[matIndex].attrib;
				cbuffMeshData.worldMat = loadedLevel.levelTransforms[transformIndex];

				curHandles.context->Map(cbuffMesh.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &sub2);
				memcpy(sub2.pData, &cbuffMeshData, sizeof(cbuffMeshData));
				curHandles.context->Unmap(cbuffMesh.Get(), 0);

				curHandles.context->DrawIndexed(mesh->drawInfo.indexCount,
					mesh->drawInfo.indexOffset + loadedLevel.levelModels[modelIndex].indexStart, loadedLevel.levelModels[modelIndex].vertexStart);

				mesh = nullptr;
			}
		}
	}
	else if (splitScreen == true)
	{
		// left viewport
		D3D11_VIEWPORT left_vp{};
		left_vp.Width = screenWidth / 2.0f;
		left_vp.Height = screenHeight;
		left_vp.MinDepth = 0;
		left_vp.MaxDepth = 1;
		left_vp.TopLeftX = 0;
		left_vp.TopLeftY = 0;

		curHandles.context->RSSetViewports(1u, &left_vp);

		// loop through all objects in current loaded level and extract needed data
		for (const auto& b : loadedLevel.blenderObjects)
		{
			int modelIndex = b.modelIndex;
			int transformIndex = b.transformIndex;

			for (int j = 0; j < loadedLevel.levelModels[modelIndex].meshCount; j++)
			{
				unsigned int meshIndex = j + loadedLevel.levelModels[modelIndex].meshStart;
				const H2B::MESH* mesh = &loadedLevel.levelMeshes[meshIndex];
				unsigned int matIndex = j + loadedLevel.levelModels[modelIndex].materialStart;
				cbuffMeshData.material = loadedLevel.levelMaterials[matIndex].attrib;
				cbuffMeshData.worldMat = loadedLevel.levelTransforms[transformIndex];

				curHandles.context->Map(cbuffMesh.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &sub2);
				memcpy(sub2.pData, &cbuffMeshData, sizeof(cbuffMeshData));
				curHandles.context->Unmap(cbuffMesh.Get(), 0);

				curHandles.context->DrawIndexed(mesh->drawInfo.indexCount,
					mesh->drawInfo.indexOffset + loadedLevel.levelModels[modelIndex].indexStart, loadedLevel.levelModels[modelIndex].vertexStart);
			}
		}

		// right viewport
		D3D11_VIEWPORT right_vp{};
		right_vp.Width = screenWidth / 2.0f;
		right_vp.Height = screenHeight;
		right_vp.MinDepth = 0;
		right_vp.MaxDepth = 1;
		right_vp.TopLeftX = screenWidth / 2.0f;
		right_vp.TopLeftY = 0;

		curHandles.context->RSSetViewports(1u, &right_vp);

		// loop through all objects in current loaded level and extract needed data
		for (const auto& b : loadedLevel.blenderObjects)
		{
			const int modelIndex = b.modelIndex;
			const int transformIndex = b.transformIndex;

			for (unsigned int j = 0; j < loadedLevel.levelModels[modelIndex].meshCount; j++)
			{
				const unsigned int meshIndex = j + loadedLevel.levelModels[modelIndex].meshStart;
				const H2B::MESH* mesh = &loadedLevel.levelMeshes[meshIndex];
				const unsigned int matIndex = j + loadedLevel.levelModels[modelIndex].materialStart;
				cbuffMeshData.material = loadedLevel.levelMaterials[matIndex].attrib;
				cbuffMeshData.worldMat = loadedLevel.levelTransforms[transformIndex];

				curHandles.context->Map(cbuffMesh.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &sub2);
				memcpy(sub2.pData, &cbuffMeshData, sizeof(cbuffMeshData));
				curHandles.context->Unmap(cbuffMesh.Get(), 0);

				curHandles.context->DrawIndexed(mesh->drawInfo.indexCount,
					mesh->drawInfo.indexOffset + loadedLevel.levelModels[modelIndex].indexStart, loadedLevel.levelModels[modelIndex].vertexStart);
			}
		}
	}


	ReleasePipelineHandles(curHandles);
	creator->Release();
	//RenderFlecsObjects();
}

// 2D Rendering/Drawing Function - called each frame
void DirectXRendererLogic::Render2D()
{
	ID3D11Device* creator;
	d3dSurface.GetDevice((void**)&creator);

	unsigned int screenHeight = 0;
	window.GetClientHeight(screenHeight);
	unsigned int screenWidth = 0;
	window.GetClientWidth(screenWidth);

	// grab the context & render target
	ID3D11DeviceContext* con;
	ID3D11RenderTargetView* view;
	ID3D11DepthStencilView* depth;
	d3dSurface.GetImmediateContext((void**)&con);
	d3dSurface.GetRenderTargetView((void**)&view);
	d3dSurface.GetDepthStencilView((void**)&depth);

	// setting up the static text object with information
	// keep in mind the position will always be the center of the text
	// only update things that need to be changed between levels
	staticText.SetText("Score");
	staticText.SetPosition(0.0f, -0.82f);
	staticText.SetScale(0.5f, 0.5f);

	// update will create the vertices so they will be ready to use
	// for static text this only needs to be done one time
	staticText.Update(screenWidth, screenHeight);

	// vertex buffer creation for the staticText
	const auto& staticVerts = staticText.GetVertices();
	D3D11_SUBRESOURCE_DATA svbData = { staticVerts.data(), 0, 0 };
	CD3D11_BUFFER_DESC svbDesc(sizeof(TextVertex) * staticVerts.size(), D3D11_BIND_VERTEX_BUFFER);
	vertexBufferStaticText->Release(); // need to release before making new buffer
	creator->CreateBuffer(&svbDesc, &svbData, vertexBufferStaticText.GetAddressOf());

	// update the dynamic text so we create the vertices
	dynamicText.SetText(std::to_string(0));
	dynamicText.Update(screenWidth, screenHeight);

	// upload the new information to the vertex buffer using map / unmap
	const auto& verts = dynamicText.GetVertices();
	D3D11_MAPPED_SUBRESOURCE msr = { nullptr };
	con->Map(vertexBufferDynamicText.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	memcpy(msr.pData, verts.data(), sizeof(TextVertex) * verts.size());
	con->Unmap(vertexBufferDynamicText.Get(), 0);

	// setup the pipeline
	ID3D11RenderTargetView* const views[] = { view };
	con->OMSetRenderTargets(ARRAYSIZE(views), views, depth);
	// set the blend state in order to use transparency
	con->OMSetBlendState(blendState_2D.Get(), nullptr, 0xFFFFFFFF);
	// set the depth stencil state for depth comparison [useful for transparency with the hud objects]
	con->OMSetDepthStencilState(depthStencilState_2D.Get(), 0xFFFFFFFF);
	// set the rasterization state for use with the scissor rectangle
	con->RSSetState(rasterizerState_2D.Get());

	const UINT strides[] = { sizeof(float) * 4 };
	const UINT offsets[] = { 0 };
	ID3D11Buffer* const buffs[] = { vertexBuffer_2D.Get() };
	// set the vertex buffer to the pipeline
	con->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
	con->IASetIndexBuffer(indexBuffer_2D.Get(), DXGI_FORMAT_R32_UINT, 0);
	con->VSSetShader(vertexShader_2D.Get(), nullptr, 0);
	con->PSSetShader(pixelShader_2D.Get(), nullptr, 0);
	con->IASetInputLayout(vertexFormat_2D.Get());
	// set the topology
	con->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	// set and update the constant buffer (cb)
	con->VSSetConstantBuffers(0, 1, cbuffSprite.GetAddressOf());

	// loop through all of the hud items and draw each one
	for (size_t i = 0; i < hud.size(); i++)
	{
		// store a constant reference to the current hud item
		const Sprite& current = hud[i];
		// update the constant buffer data with the sprite's information
		constantBufferSpriteData = UpdateSpriteConstantBufferData(current);
		// set the sprite's scissor rect
		const auto& scissor = current.GetScissorRect();
		D3D11_RECT rect = { static_cast<LONG>(scissor.min.x), static_cast<LONG>(scissor.min.y), static_cast<LONG>(scissor.max.x), static_cast<LONG>(scissor.max.y) };
		con->RSSetScissorRects(1, &rect);
		// update the constant buffer with the current sprite's data
		con->UpdateSubresource(cbuffSprite.Get(), 0, nullptr, &constantBufferSpriteData, 0, 0);
		// set a texture (srv) and sampler to the pixel shader
		con->PSSetShaderResources(0, 1, shaderResourceView[i].GetAddressOf());
		con->PSSetSamplers(0, 1, samplerState.GetAddressOf());
		// now we can draw
		con->DrawIndexed(6, 0, 0);
	}

	// set the vertex buffer for the static text
	con->IASetVertexBuffers(0, 1, vertexBufferStaticText.GetAddressOf(), strides, offsets);
	// change the topology to a triangle list
	con->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// update the constant buffer data for the text
	constantBufferSpriteData = UpdateTextConstantBufferData(staticText);
	// bind the texture used for rendering the font
	con->PSSetShaderResources(0, 1, shaderResourceView[TEXTURE_ID::FONT_CONSOLAS].GetAddressOf());
	// update the constant buffer with the text's data
	con->UpdateSubresource(cbuffSprite.Get(), 0, nullptr, &constantBufferSpriteData, 0, 0);
	// draw the static text using the number of vertices
	con->Draw(staticVerts.size(), 0);

	// set the vertex buffer for the dynamic text
	con->IASetVertexBuffers(0, 1, vertexBufferDynamicText.GetAddressOf(), strides, offsets);
	// change the topology to a triangle list
	con->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// update the constant buffer data for the text
	constantBufferSpriteData = UpdateTextConstantBufferData(dynamicText);
	// bind the texture used for rendering the font
	con->PSSetShaderResources(0, 1, shaderResourceView[TEXTURE_ID::FONT_CONSOLAS].GetAddressOf());
	// update the constant buffer with the text's data
	con->UpdateSubresource(cbuffSprite.Get(), 0, nullptr, &constantBufferSpriteData, 0, 0);
	// draw the static text using the number of vertices
	con->Draw(verts.size(), 0);

	// release temp handles
	depth->Release();
	view->Release();
	con->Release();
	creator->Release();
}

// Update Function - called each frame
void DirectXRendererLogic::Update()
{
	ID3D11DeviceContext* con = nullptr;
	d3dSurface.GetImmediateContext((void**)&con);

	// Window/D3D surface data
	UINT screenHeight = 0;
	window.GetClientHeight(screenHeight);
	UINT screenWidth = 0;
	window.GetClientWidth(screenWidth);
	float aspect = 0.0f;
	d3dSurface.GetAspectRatio(aspect);

	// Time keeping code
	std::chrono::high_resolution_clock::time_point lastUpdate;
	float deltaTime;
	auto now = std::chrono::high_resolution_clock::now();
	deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - lastUpdate).count() / 1000000.0f;

	// HUD Updates/2D updates
#pragma region HUD & UI
	// store the the scissor rectangles for the HP bar & Ammo Bar
	//auto scissor_rect_HP_left = hud[TEXTURE_ID::HUD_HP_LEFT].GetScissorRect();
	//auto scissor_rect_HP_right = hud[TEXTURE_ID::HUD_HP_RIGHT].GetScissorRect();
	//auto scissor_rect_Ammo_left = hud[TEXTURE_ID::HUD_MP_LEFT].GetScissorRect();
	//auto scissor_rect_Ammo_right = hud[TEXTURE_ID::HUD_MP_RIGHT].GetScissorRect();

	//float size_1 = ((screenWidth / 2) - (screenWidth / 4) - 15);
	//float size_2 = (screenWidth / 2);

	//const float barChange = 53.75f; // total bar / 4
#pragma endregion
	// Debug Input:
#pragma region DebugInput
	// Level loading input and logic
	float f1_State = 0.0f;
	float rightBumperState = 0.0f;
	bool totalDoChangeLevel = false;
	if (inputProxy.GetState(G_KEY_F1, f1_State) != GW::GReturn::REDUNDANT)
	{
		totalDoChangeLevel = false;
	}
	if (f1_State + rightBumperState > 0)
	{
		totalDoChangeLevel = true;
	}
	if (totalDoChangeLevel == true)
	{
		totalDoChangeLevel = false;
		levelIndex += 1;


		if (levelIndex >= sizeof(levels) / sizeof(levels[0]))
		{
			levelIndex = 0;
		}

		loadLevel();
	}

	// Debug Free Camera input
	float numPad_1_State = 0.0f;

	if (startDebugFreeCamCounter == true && debugFreeCamInputCounter > 0)
	{
		debugFreeCamInputCounter -= 1;
	}
	if (startDebugFreeCamCounter == true && debugFreeCamInputCounter <= 0)
	{
		debugFreeCamInputCounter = 30;
		startDebugFreeCamCounter = false;
	}
	if (startDebugFreeCamCounter == false)
	{
		if (inputProxy.GetState(G_KEY_NUMPAD_1, numPad_1_State) == GW::GReturn::REDUNDANT)
		{
			numPad_1_State = 0.0f;
		}

		if (numPad_1_State > 0)
		{
			numPad_1_State = 0.0f;
			startDebugFreeCamCounter = true;
			debugFreeCamInputCounter = 30;

			if (debugCamera == true)
			{
				debugCamera = false;
			}
			else if (debugCamera == false)
			{
				debugCamera = true;
			}
		}
	}

	// splitscreen input
	float numPad_2_State = 0.0f;
	float dPad_Down_State = 0.0f;
	if (startCounter == true && splitCounter > 0)
	{
		splitCounter -= 1;
	}
	if (startCounter == true && splitCounter <= 0)
	{
		splitCounter = 30;
		startCounter = false;
	}
	if (startCounter == false)
	{
		if (inputProxy.GetState(G_KEY_NUMPAD_2, numPad_2_State) == GW::GReturn::REDUNDANT)
		{
			numPad_2_State = 0.0f;
			dPad_Down_State = 0.0f;
		}
		if (controllerInputProxy.GetState(0, G_DPAD_DOWN_BTN, dPad_Down_State) == GW::GReturn::REDUNDANT)
		{
			numPad_2_State = 0.0f;
			dPad_Down_State = 0.0f;
		}
		float totalDoSplit = numPad_2_State + dPad_Down_State;

		if (totalDoSplit > 0)
		{
			numPad_2_State = 0.0f;
			dPad_Down_State = 0.0f;
			totalDoSplit = 0.0f;
			startCounter = true;
			splitCounter = 30;

			if (splitScreen == true)
			{
				splitScreen = false;
			}
			else if (splitScreen == false)
			{
				splitScreen = true;
			}
		}
	}

	// orthographic Mode input
	float numPad_6_State = 0.0f;
	float dPad_Left_State = 0.0f;

	if (startOrthoCounter == true && orthoCounter > 0)
	{
		orthoCounter -= 1;
	}
	if (startOrthoCounter == true && orthoCounter <= 0)
	{
		orthoCounter = 30;
		startOrthoCounter = false;
	}
	if (startOrthoCounter == false)
	{
		if (inputProxy.GetState(G_KEY_NUMPAD_6, numPad_6_State) == GW::GReturn::REDUNDANT)
		{
			numPad_6_State = 0.0f;
			dPad_Left_State = 0.0f;
		}

		float doTotalOrtho = numPad_6_State + dPad_Left_State;

		if (doTotalOrtho > 0)
		{
			numPad_6_State = 0.0f;
			dPad_Left_State = 0.0f;
			doTotalOrtho = 0.0f;
			startOrthoCounter = true;
			orthoCounter = 30;

			if (orthoMode == true)
			{
				orthoMode = false;
			}
			else if (orthoMode == false)
			{
				orthoMode = true;
			}
		}
	}

	// wireframe input
	float numPad_3_State = 0.0f;
	float dPad_Up_State = 0.0f;

	if (startWireframeCounter == true && wireframeInputCounter > 0)
	{
		wireframeInputCounter -= 1;
	}
	if (startWireframeCounter == true && wireframeInputCounter <= 0)
	{
		wireframeInputCounter = 30;
		startWireframeCounter = false;
	}
	if (startWireframeCounter == false)
	{
		if (inputProxy.GetState(G_KEY_NUMPAD_3, numPad_3_State) == GW::GReturn::REDUNDANT)
		{
			numPad_3_State = 0.0f;
			dPad_Up_State = 0.0f;
		}

		float doTotalWireFrame = numPad_3_State + dPad_Up_State;

		if (doTotalWireFrame > 0)
		{
			numPad_3_State = 0.0f;
			dPad_Up_State = 0.0f;
			doTotalWireFrame = 0.0f;
			startWireframeCounter = true;
			wireframeInputCounter = 30;

			if (wireFrameMode == true)
			{
				wireFrameMode = false;
			}
			else if (wireFrameMode == false)
			{
				wireFrameMode = true;
			}
		}
	}

	//// Debug Input for Health and Ammo:
	//float numPad_7_State = 0.0f;
	//float numPad_8_State = 0.0f;
	//float numPad_4_State = 0.0f;
	//float numPad_5_State = 0.0f;

	//// Health Updates
	//if (startDebugHealthCounter == true && debugHealthCounter > 0)
	//{
	//	debugHealthCounter -= 1;
	//}
	//if (startDebugHealthCounter == true && debugHealthCounter <= 0)
	//{
	//	debugHealthCounter = 30;
	//	startDebugHealthCounter = false;
	//}
	//if (startDebugHealthCounter == false)
	//{
	//	if (inputProxy.GetState(G_KEY_NUMPAD_7, numPad_7_State) != GW::GReturn::REDUNDANT) // gain health
	//	{
	//		if (numPad_7_State > 0)
	//		{
	//			numPad_7_State = 0;
	//			startDebugHealthCounter = true;
	//			debugHealthCounter = 30;

	//			if (scissor_rect_HP_left.min.x > size_1)
	//			{
	//				scissor_rect_HP_left.min.x -= barChange;
	//				scissor_rect_HP_right.max.x += barChange;
	//			}
	//		}
	//	}
	//	if (inputProxy.GetState(G_KEY_NUMPAD_8, numPad_8_State) != GW::GReturn::REDUNDANT) // lose health
	//	{
	//		if (numPad_8_State > 0)
	//		{
	//			numPad_8_State = 0;
	//			startDebugHealthCounter = true;
	//			debugHealthCounter = 30;

	//			if (scissor_rect_HP_left.min.x < size_2)
	//			{
	//				scissor_rect_HP_left.min.x += barChange;
	//				scissor_rect_HP_right.max.x -= barChange;
	//			}
	//		}
	//	}
	//}

	//// Ammo Updates
	//if (startDebugAmmoCounter == true && debugAmmoCounter > 0)
	//{
	//	debugAmmoCounter -= 1;
	//}
	//if (startDebugAmmoCounter == true && debugAmmoCounter <= 0)
	//{
	//	debugAmmoCounter = 30;
	//	startDebugAmmoCounter = false;
	//}
	//if (startDebugAmmoCounter == false)
	//{
	//	if (inputProxy.GetState(G_KEY_NUMPAD_4, numPad_4_State) != GW::GReturn::REDUNDANT) // gain ammo
	//	{
	//		if (numPad_4_State > 0)
	//		{
	//			numPad_4_State = 0;
	//			startDebugAmmoCounter = true;
	//			debugAmmoCounter = 30;

	//			if (scissor_rect_Ammo_left.min.x > size_1)
	//			{
	//				scissor_rect_Ammo_left.min.x -= barChange;
	//				scissor_rect_Ammo_right.max.x += barChange;
	//			}
	//		}
	//	}
	//	if (inputProxy.GetState(G_KEY_NUMPAD_5, numPad_5_State) != GW::GReturn::REDUNDANT) // lose ammo
	//	{
	//		if (numPad_5_State > 0)
	//		{
	//			numPad_5_State = 0;
	//			startDebugAmmoCounter = true;
	//			debugAmmoCounter = 30;

	//			if (scissor_rect_Ammo_left.min.x < size_2)
	//			{
	//				scissor_rect_Ammo_left.min.x += barChange;
	//				scissor_rect_Ammo_right.max.x -= barChange;
	//			}
	//		}
	//	}
	//}
#pragma endregion
	// Camera movement variables
	float totalY = 0.0f;
	float totalZ = 0.0f;
	float totalX = 0.0f;
	float totalPitch = 0.0f;
	float totalYaw = 0.0f;
	float totalRot = 0.0f;
	const float camMoveSpeed = 0.3f;
	float perFrameSpeed = camMoveSpeed * deltaTime;
	float thumbstickSpeed = 3.14 * deltaTime;

	// Camera Input variables
	float spaceKeyState = 0.0f;
	float leftShiftKeyState = 0.0f;
	float rightTriggerState = 0.0f;
	float leftTriggerState = 0.0f;
	float wState = 0.0f;
	float sState = 0.0f;
	float dState = 0.0f;
	float aState = 0.0f;
	float leftStickX = 0.0f;
	float leftStickY = 0.0f;
	float rightStickY = 0.0f;
	float rightStickX = 0.0f;
	float mouseDeltaY = 0.0f;
	float mouseDeltaX = 0.0f;
	float qKeyState = 0.0f;
	float eKeyState = 0.0f;
	float leftButtonKeyState = 0.0f;
	float rightButtonKeyState = 0.0f;
	const float y_axis_value = 4.0f;

	// Camera Logic
	if (debugCamera == false)
	{
		// place viewMat at player position
		GW::MATH::GMATRIXF tempViewMat = GW::MATH::GIdentityMatrixF;

		// inverse view into cam
		GW::MATH::GMATRIXF tempCamMat = GW::MATH::GIdentityMatrixF;
		GW::MATH::GMatrix::InverseF(viewMat, tempCamMat);
		camMatrix = tempCamMat;

		// record cameras world position for use in pixel shader
		GW::MATH::GMATRIXF tempMat = camMatrix;
		GW::MATH::GVECTORF tempVec = camMatrix.row4;
		cbuffSceneData.camWorldPos = tempVec;

		// inverse camera matrix and assign it back into view matrix
		GW::MATH::GMatrix::InverseF(camMatrix, viewMat);

		// Apply changes to constantBuffer
		cbuffSceneData.viewMat = viewMat;
	}
	else if (debugCamera == true)
	{
		// get user input:
		// up/down input
		inputProxy.GetState(G_KEY_SPACE, spaceKeyState);
		inputProxy.GetState(G_KEY_LEFTSHIFT, leftShiftKeyState);
		controllerInputProxy.GetState(0, G_RIGHT_TRIGGER_AXIS, rightTriggerState);
		controllerInputProxy.GetState(0, G_LEFT_TRIGGER_AXIS, leftTriggerState);
		totalY = spaceKeyState - leftShiftKeyState + rightTriggerState - leftTriggerState;
		// local translations input for forward/back & strafe left/right
		inputProxy.GetState(G_KEY_W, wState);
		inputProxy.GetState(G_KEY_S, sState);
		controllerInputProxy.GetState(0, G_LX_AXIS, leftStickX);
		controllerInputProxy.GetState(0, G_LY_AXIS, leftStickY);
		inputProxy.GetState(G_KEY_D, dState);
		inputProxy.GetState(G_KEY_A, aState);
		totalZ = wState - sState + leftStickY;
		totalX = dState - aState + leftStickX;
		// Rotation input 1 (mouse/sticks)
		if (inputProxy.GetMouseDelta(mouseDeltaX, mouseDeltaY) == GW::GReturn::REDUNDANT)
		{
			mouseDeltaX = 0.0f;
			mouseDeltaY = 0.0f;
		}
		if (controllerInputProxy.GetState(0, G_RY_AXIS, rightStickY) == GW::GReturn::REDUNDANT)
		{
			rightStickY = 0.0f;
		}
		if (controllerInputProxy.GetState(0, G_RX_AXIS, rightStickX) == GW::GReturn::REDUNDANT)
		{
			rightStickX = 0.0f;
		}
		totalPitch = (65.0f * mouseDeltaY) / screenHeight + rightStickY * thumbstickSpeed * -1;
		totalYaw = (65.0f * aspect * mouseDeltaX) / screenWidth + rightStickX * thumbstickSpeed;
		// rotation input 2 (Q/E & left/right buttons)
		inputProxy.GetState(G_KEY_Q, qKeyState);
		inputProxy.GetState(G_KEY_E, eKeyState);
		controllerInputProxy.GetState(0, G_WEST_BTN, leftButtonKeyState);
		controllerInputProxy.GetState(0, G_EAST_BTN, rightButtonKeyState);
		totalRot = qKeyState - eKeyState + leftButtonKeyState - rightButtonKeyState;

		// final values to apply to camera
		float appliedY = 0.0f;
		float appliedZ = 0.0f;
		float appliedX = 0.0f;
		float appliedPitch = 0.0f;
		float appliedYaw = 0.0f;
		float appliedRot = 0.0f;
		float moveSpeed = debugFreeCamMoveSpeedModifier;

		// manipulate camera matrix by applying user input:
		// Up/Down
		appliedY += moveSpeed * (totalY * camMoveSpeed * deltaTime);
		GW::MATH::GVECTORF camTranslateY =
		{
			0, appliedY, 0, 0
		};
		GW::MATH::GMatrix::TranslateGlobalF(camMatrix, camTranslateY, camMatrix);

		// forward/backwards & left/right strafing
		appliedZ = moveSpeed * (totalZ * perFrameSpeed);
		appliedX = moveSpeed * (totalX * perFrameSpeed);
		GW::MATH::GVECTORF camTranslateZXVec =
		{
			appliedX, 0, appliedZ, 0
		};
		GW::MATH::GMatrix::TranslateLocalF(camMatrix, camTranslateZXVec, camMatrix);

		//rotate left/right
		appliedPitch = 0.05f * totalPitch;
		GW::MATH::GMatrix::RotateXLocalF(camMatrix, appliedPitch, camMatrix);

		// turn left/right
		appliedYaw = 0.05f * totalYaw;

		GW::MATH::GMatrix::RotateYLocalF(camMatrix, appliedYaw, camMatrix);

		// apply rotation2
		appliedRot = 0.05f * totalRot;
		GW::MATH::GMatrix::RotateZLocalF(camMatrix, appliedRot, camMatrix);

		// record cameras world position for use in pixel shader
		GW::MATH::GMATRIXF tempMat = camMatrix;
		GW::MATH::GVECTORF tempVec = camMatrix.row4;
		cbuffSceneData.camWorldPos = tempVec;

		// inverse camera matrix and assign it back into view matrix
		GW::MATH::GMatrix::InverseF(camMatrix, viewMat);

		// Apply changes to constantBuffer
		cbuffSceneData.viewMat = viewMat;
	}

	con->Release();
}

// Helper functions for setting up and using raster pipeline
#pragma region Pipeline Helper Functions
DirectXRendererLogic::PipelineHandles DirectXRendererLogic::GetCurrentPipelineHandles()
{
	PipelineHandles retval;
	d3dSurface.GetImmediateContext((void**)&retval.context);
	d3dSurface.GetRenderTargetView((void**)&retval.targetView);
	d3dSurface.GetDepthStencilView((void**)&retval.depthStencil);
	return retval;
}
void DirectXRendererLogic::SetUpPipeline(PipelineHandles& handles)
{
	ID3D11Device* creator;
	d3dSurface.GetDevice((void**)&creator);
	//InitializePipeline(creator);

	SetRenderTargets(handles);
	SetVertexBuffers(handles);
	SetShaders(handles);

	handles.context->RSSetScissorRects(0, nullptr);
	handles.context->IASetInputLayout(vertexFormat.Get());
	handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	handles.context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	ID3D11Buffer* const constantBuffers[] = { cbuffScene.Get(), cbuffMesh.Get() };

	handles.context->VSSetConstantBuffers(0, 2, constantBuffers);
	handles.context->PSSetConstantBuffers(0, 2, constantBuffers);

	creator->Release();
}
void DirectXRendererLogic::SetUpPipeline_2D(PipelineHandles& handles)
{
	ID3D11Device* creator;
	d3dSurface.GetDevice((void**)&creator);
	//InitializePipeline(creator);

	SetRenderTargets(handles);
	SetVertexBuffers(handles);
	SetShaders(handles);

	handles.context->RSSetScissorRects(0, nullptr);
	handles.context->IASetInputLayout(vertexFormat.Get());
	handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	handles.context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	ID3D11Buffer* const constantBuffers[] = { cbuffScene.Get(), cbuffMesh.Get() };

	handles.context->VSSetConstantBuffers(0, 2, constantBuffers);
	handles.context->PSSetConstantBuffers(0, 2, constantBuffers);

	creator->Release();
}
void DirectXRendererLogic::SetRenderTargets(PipelineHandles handles)
{
	ID3D11RenderTargetView* const views[] = { handles.targetView };
	handles.context->OMSetRenderTargets(ARRAYSIZE(views), views, handles.depthStencil);

}
void DirectXRendererLogic::SetVertexBuffers(PipelineHandles handles)
{
	const UINT strides[] = { sizeof(OBJ_VERT) };
	const UINT offsets[] = { 0 };
	ID3D11Buffer* const buffs[] = { vertexBuffer.Get() };
	handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
}
void DirectXRendererLogic::SetShaders(PipelineHandles handles)
{
	handles.context->VSSetShader(vertexShader.Get(), nullptr, 0);
	handles.context->PSSetShader(pixelShader.Get(), nullptr, 0);
}
void DirectXRendererLogic::ReleasePipelineHandles(PipelineHandles toRelease)
{
	toRelease.depthStencil->Release();
	toRelease.targetView->Release();
	toRelease.context->Release();
}
#pragma endregion