/**
* \class d3dx12_application
*
* \brief Provide an example of how to use D3DX12
*
* This is a basic example of how to display a coloured triangle
* using DirectX 12 api. The SDL2 library is used for window and 
* surface creation.
*
* \author Alixander Roden
*		  The University Of Suffolk
*		  Ipswich, United Kingdom
*
* \date 2018/04/30 05:01:00
*
* Contact: S190762@uos.ac.uk
*
* Last Updated on: 30th April 2018
*
*/

#ifndef D3DX12_H
#define D3DX12_H

//remove unused elements of the win32 api
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN   
#endif

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include <string>
#include <chrono>
#include <SDL.h>
#include <SDL_syswm.h>
#include <iostream>

//define the safe_release macro which destroys a directx objects
#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

//include directx
using namespace DirectX;

/**
 * \brief Structure to describe a vertex
 */
struct vertex {
	XMFLOAT3 pos;
	XMFLOAT3 col;
};

class d3dx12_application
{
public:
	void run();
protected:
	int width_ = 800;
	int height_ = 600;
	int frame_index_;
	int rtv_descriptor_size_;
private:
	HWND hwnd_ = nullptr;

	ID3D12Device* device_;
	IDXGISwapChain3* swap_chain_;
	ID3D12CommandQueue* command_queue_;
	ID3D12DescriptorHeap* rtv_descriptor_heap_;
	ID3D12Resource* render_targets_[3];
	ID3D12CommandAllocator* command_allocator_[3];
	ID3D12GraphicsCommandList* command_list_;
	ID3D12Fence* fence_[3];
	HANDLE fence_event_;
	UINT64 fence_value_[3];
	ID3D12PipelineState* pipeline_state_object_state_;
	ID3D12RootSignature* root_signature_;
	D3D12_VIEWPORT viewport_;
	D3D12_RECT scissor_rect_;
	ID3D12Resource* vertex_buffer_;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view_;

	/**
	 * \brief Create the SDL2 window and obtain the win32 handle of it
	 * \return 
	 */
	bool initialize_window();

	/**
	 * \brief Initialize the DirectX 12 API
	 * \return 
	 */
	bool init_d3d();

	/**
	 * \brief Wait for the previous frame to continue rendering
	 */
	void wait_for_previous_frame();


	/**
	 * \brief Update the pipeline. This is where draw commands are
	 */
	void update_pipeline();


	/**
	 * \brief Render
	 */
	void render();

	void cleanup();

	void mainloop();
};


#endif