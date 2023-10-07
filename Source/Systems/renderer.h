// The rendering system is responsible for drawing all objects
#ifndef DIRECTXRENDERERLOGIC_H
#define DIRECTXRENDERERLOGIC_H

/*
Debug Keys:
Num Pad 1 - Toggle Debug Free Cams
Num Pad 2 - Toggle Splitscreen (DISABLED)
Num Pad 3 - Toggle WireFrame mode
Num Pad 6 - Toggle Orthographic Projection Mode
F1 - Load Next Level
Num Pad 7 & 8 - Change Health
Num Pad 4 & 5 - Change Ammo

FreeCam Controls :
Keyboard -
Q/E - Rotate Camera Orientation
W/A/S/D - Move Camera
Mouse - Look Around
L Shift/Space - Move up and down

Controller -
A/B - Rotate Camera Orientation
Left Stick - Move Camera
Right Stick - Look Around
Right Bumper - Load Next Level
Left/Right Trigger - move up and down
*/


// Required tools & Libraries
#include "../Utils/h2bParser.h"
#include "../Utils/load_data_oriented.h"
#include <wrl\client.h>
#include <d3d11.h>
#include <iostream>
#include "../Utils/sprite.h"
#include "../Utils/font.h"
#include "../Utils/tinyxml2.h"
#include <DDSTextureLoader.h>

// enumeration to easily index into an array of textures for 2D rendering
enum TEXTURE_ID { HUD_BACKPLATE = 0, HUD_HP_LEFT, HUD_HP_RIGHT, HUD_MP_LEFT, HUD_MP_RIGHT, HUD_CENTER, FONT_CONSOLAS, COUNT };
using HUD = std::vector<Sprite>;

class DirectXRendererLogic
{
	// Structs needed for renderer
#pragma region Structs
// structs for constant buffer data
	__declspec(align(16)) struct SceneData
	{
		GW::MATH::GMATRIXF viewMat, projMat;

		GW::MATH::GVECTORF lightDir, lightColor, camWorldPos, sunAmbient;
	};
	__declspec(align(16)) struct MeshData
	{
		GW::MATH::GMATRIXF worldMat;
		H2B::ATTRIBUTES material;
	};
	__declspec(align(16)) struct SpriteData
	{
		GW::MATH::GVECTORF pos_scale;
		GW::MATH::GVECTORF rotation_depth;
	};
	// structs for pipeline creation
	struct PipelineHandles
	{
		ID3D11DeviceContext* context;
		ID3D11RenderTargetView* targetView;
		ID3D11DepthStencilView* depthStencil;
	};
	typedef struct _OBJ_VEC3_
	{
		float x, y, z; // 3D Coordinate.
	}OBJ_VEC3;
	typedef struct _OBJ_VERT_
	{
		OBJ_VEC3 pos; // Left-handed +Z forward coordinate w not provided, assumed to be 1.
		OBJ_VEC3 uvw; // D3D/Vulkan style top left 0,0 coordinate.
		OBJ_VEC3 nrm; // Provided direct from obj file, may or may not be normalized.
	}OBJ_VERT;
#pragma endregion

	// Used to query screen dimensions
	GW::SYSTEM::GWindow									window;
	// DirectX resources used for rendering 3D
	GW::GRAPHICS::GDirectX11Surface						d3dSurface;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>			vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>			pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>			vertexFormat;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				indexBuffer;
	// Constant Buffer Data containers
	SceneData											cbuffSceneData;
	MeshData											cbuffMeshData;
	SpriteData											constantBufferSpriteData;
	// DirectX resources used for rendering 2D
	Microsoft::WRL::ComPtr<ID3D11Buffer>				vertexBuffer_2D;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				indexBuffer_2D;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				constantBuffer_2D;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				vertexBufferStaticText;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				vertexBufferDynamicText;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>			vertexFormat_2D;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>			vertexShader_2D;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>			pixelShader_2D;
	// collection of sprites used in 2D rendering
	HUD													hud;
	// texuring related interface objects
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	shaderResourceView[TEXTURE_ID::COUNT];
	Microsoft::WRL::ComPtr<ID3D11SamplerState>			samplerState;
	// blending related interface objects
	Microsoft::WRL::ComPtr<ID3D11BlendState>			blendState_2D;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		depthStencilState_2D;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState>		rasterizerState_2D;
	// font objects used for rendering text
	Font												consolas32;
	Text												staticText;
	Text												dynamicText;
	// Rendering Data - Matrices
	GW::MATH::GMATRIXF									worldMat;
	GW::MATH::GMATRIXF									viewMat;
	GW::MATH::GMATRIXF									projectionMat;
	GW::MATH::GMATRIXF									camMatrix;
	GW::MATH::GMATRIXF									perspectiveMat; // to be assigned to projectionMat as needed
	GW::MATH::GMATRIXF									orthographicMat; // to be assigned to projectionMat as needed (When orthographic mode is enabled)
	// Rendering Data - Lighting
	GW::MATH::GVECTORF									lightDir;
	GW::MATH::GVECTORF									lightColor;
	GW::MATH::GVECTORF									sunAmbient;
	// Constant Buffers
	Microsoft::WRL::ComPtr<ID3D11Buffer>				cbuffScene;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				cbuffMesh;
	Microsoft::WRL::ComPtr<ID3D11Buffer>				cbuffSprite;
	// User Input 
	GW::INPUT::GInput									inputProxy;
	GW::INPUT::GController								controllerInputProxy;
	// WireFrame Debug Mode
	ID3D11RasterizerState* WireFrame;
	D3D11_RASTERIZER_DESC								wfdesc;
	HRESULT												hr_wireframe;
	// Shader Paths
	std::string											vertexShaderSource;
	std::string											pixelShaderSource;
	std::string											vertexShaderSource_2D;
	std::string											pixelShaderSource_2D;
	// Level Loading Containers
	GW::SYSTEM::GLog									log;
	Level_Data											loadedLevel;
	const char* level_00 = "../Levels/GameLevel.txt";
	const char* level_01 = "../Levels/GameLevelTest.txt";
	const char* levels[2] = { level_00, level_01 };
	int levelIndex = 0;
	// music and audioFX data
	GW::AUDIO::GAudio audioPlayer;
	GW::AUDIO::GSound loadingFX;
	GW::AUDIO::GMusic music;
	const char* loadingSound = "../SoundFX/loadingFX.wav";
	const char* backgroundMusic = "../SoundFX/music.wav";
	// variables for rendering wireframes
	ID3D11RasterizerState* WireFrame;
	D3D11_RASTERIZER_DESC wfdesc;
	HRESULT hr;
	bool wireFrameMode = false;
	int wireframeInputCounter = 30;
	bool startWireframeCounter = false;
public:
	bool Shutdown();
	void Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX11Surface _d3d);
	void Update();									// Update Renderer Each Frame				(Called First)
	void Render();									// Render Pass for 3D						(Called Second)
	void Render2D();								// Render Pass for 2D						(Called Third)
	void InitializeAll();

private:
	std::vector<Sprite>	LoadHudFromXML(std::string filepath);
	void loadHUD();
	SpriteData UpdateSpriteConstantBufferData(const Sprite& s);
	SpriteData UpdateTextConstantBufferData(const Text& s);
	void loadLevel();
	void loadSprites(ID3D11Device* creator);
	void IntializeGraphics();
	void InitializeSound();
	void InitializeWireframeMode(ID3D11Device* creator);
	void InitializeConstantBufferData();
	void InitializeConstantBuffers(ID3D11Device* creator);
	void CreateConstantBufferScene(ID3D11Device* creator, const void* data, unsigned int sizeInBytes);
	void CreateConstantBufferMesh(ID3D11Device* creator, const void* data, unsigned int sizeInBytes);
	void ReInitializeBuffers(ID3D11Device* creator);
	void InitializeIndexBuffer(ID3D11Device* creator);
	void CreateIndexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes);
	void CreateIndexBuffer2D(ID3D11Device* creator);
	void InitializeVertexBuffer(ID3D11Device* creator);
	void CreateVertexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes);
	void CreateVertexBuffer2D(ID3D11Device* creator);
	void InitializeTextandFont();
	void InitializeWorldMatrix();
	void InitializePerspectiveMatrix();
	void InitializeOrthographicMatrix();
	void InitializeCameraMatrix();
	void InitializeViewMatrix();
	void InitializeMatricesAndVariables();
	void InitializePipeline_2D(ID3D11Device* creator);
	Microsoft::WRL::ComPtr<ID3DBlob> CompileVertexShader(ID3D11Device* creator, UINT compilerFlags);
	Microsoft::WRL::ComPtr<ID3DBlob> CompilePixelShader(ID3D11Device* creator, UINT compilerFlags);
	Microsoft::WRL::ComPtr<ID3DBlob> CompileVertexShader_2D(ID3D11Device* creator, UINT compilerFlags);
	Microsoft::WRL::ComPtr<ID3DBlob> CompilePixelShader_2D(ID3D11Device* creator, UINT compilerFlags);
	void CreateVertexInputLayout(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob);
	void CreateVertexInputLayout_2D(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob);
	void CreateBlendState(ID3D11Device* creator);
	void CreateDepthStencilDesc(ID3D11Device* creator);
	void CreateRasterState_2D(ID3D11Device* creator);
	PipelineHandles GetCurrentPipelineHandles();
	void SetUpPipeline(PipelineHandles& handles);
	void SetUpPipeline_2D(PipelineHandles& handles);
	void SetRenderTargets(PipelineHandles handles);
	void SetVertexBuffers(PipelineHandles handles);
	void SetShaders(PipelineHandles handles);
	void ReleasePipelineHandles(PipelineHandles toRelease);

};
#endif