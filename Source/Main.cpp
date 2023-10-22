#define GATEWARE_ENABLE_CORE // All libraries need this
#define GATEWARE_ENABLE_SYSTEM // Graphics libs require system level libraries
#define GATEWARE_ENABLE_GRAPHICS // Enables all Graphics Libraries
#define GATEWARE_ENABLE_MATH // Enables Math
#define GATEWARE_ENABLE_INPUT // Enables User Input
#define GATEWARE_ENABLE_AUDIO // Enables Audio
#define GATEWARE_DISABLE_GDIRECTX12SURFACE
#define GATEWARE_DISABLE_GRASTERSURFACE
#define GATEWARE_DISABLE_GOPENGLSURFACE
#define GATEWARE_DISABLE_GVULKANSURFACE
#include "../gateware-main/Gateware.h"
#include "Utils/FileIntoString.h"
#include "Systems/renderer.h"

using namespace GW;
using namespace CORE;
using namespace SYSTEM;
using namespace GRAPHICS;

/* Controls:

Keyboard -
Q/E - Rotate Camera Orientation
W/A/S/D - Move Camera
Mouse - Look Around
F1 - Load Next Level
L Sheft/Space - Move up and down
Num Pad 1 - Toggle Orthographic mode
Num Pad 2 - Toggle Splitscreen mode
Num Pad 3 - Toggle WireFrame mode

Controller -
A/B - Rotate Camera Orientation
Left Stick - Move Camera
Right Stick - Look Around
Right Bumper - Load Next Level
Left/Right Trigger - move up and down
dPad Left - Toggle Orthographic mode
dPad down - Toggle Splitscreen mode
dPad up - Toggle WireFrame mode

*/

// lets pop a window and use D3D11 to clear to a navy blue screen
int main()
{
	GWindow win;
	GEventResponder msgs;
	GDirectX11Surface d3d11;

	if (+win.Create(0, 0, 800, 600, GWindowStyle::WINDOWEDBORDERED))
	{
		win.SetWindowName("Clardy_Tyler Level Renderer");
		float clr[] = { 0 / 255.0f, 0 / 255.0f, 25 / 255.0f, 1 };
		msgs.Create([&](const GW::GEvent& e) {
			GW::SYSTEM::GWindow::Events q;
			if (+e.Read(q) && q == GWindow::Events::RESIZE)
				clr[2] += 0.01f; // move towards a cyan as they resize
		});
		win.Register(msgs);
		if (+d3d11.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
		{
			Renderer renderer(win, d3d11);
			while (+win.ProcessWindowEvents())
			{
				IDXGISwapChain* swap;
				ID3D11DeviceContext* con;
				ID3D11RenderTargetView* view;
				ID3D11DepthStencilView* depth;
				if (+d3d11.GetImmediateContext((void**)&con) &&
					+d3d11.GetRenderTargetView((void**)&view) &&
					+d3d11.GetDepthStencilView((void**)&depth) &&
					+d3d11.GetSwapchain((void**)&swap))
				{
					con->ClearRenderTargetView(view, clr);
					con->ClearDepthStencilView(depth, D3D11_CLEAR_DEPTH, 1, 0);
					renderer.Update();
					renderer.Render();
					swap->Present(1, 0);

					// release incremented COM reference counts
					swap->Release();
					view->Release();
					depth->Release();
					con->Release();
				}
			}
		}
	}

	return 0;
}