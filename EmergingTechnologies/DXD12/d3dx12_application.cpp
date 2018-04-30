#include "d3dx12_application.h"

void d3dx12_application::run()
{
	initialize_window();
	init_d3d();
	mainloop();
	wait_for_previous_frame();
	CloseHandle(fence_event_);
	cleanup();
}

bool d3dx12_application::init_d3d()
{
	//create a directx 12 instance
	IDXGIFactory4* dxgiFactory;
	CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory));

	IDXGIAdapter1* adapter;
	auto adapter_index = 0;
	auto adapter_found = false;

	//Loop through all devices on the system
	while (dxgiFactory->EnumAdapters1(adapter_index, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		//get the details of the device
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		//if the device is a software renderer then skip it
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		//create a directx 12 device
		auto hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr);
		if (SUCCEEDED(hr))
		{
			//if this was sucessful then directx 12 is supported, and we chose that device
			adapter_found = true;
			break;
		}

		adapter_index++;
	}

	//if no adapter was found then directx 12 is not supported
	if (!adapter_found)
	{
		return false;
	}

	//create a device using the found device
	D3D12CreateDevice(
		adapter,
		D3D_FEATURE_LEVEL_11_0,
		IID_PPV_ARGS(&device_)
	);


	//create a command queue
	D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
	command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	device_->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(&command_queue_));

	//define the back buffer
	DXGI_MODE_DESC back_buffer_desc = {};
	back_buffer_desc.Width = width_;
	back_buffer_desc.Height = height_;
	back_buffer_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	//define the number of samples (multisampling)
	DXGI_SAMPLE_DESC dxgi_sample_desc = {};
	dxgi_sample_desc.Count = 1;

	//describe the swapchain
	DXGI_SWAP_CHAIN_DESC dxgi_swap_chain_desc = {};
	dxgi_swap_chain_desc.BufferCount = 3;
	dxgi_swap_chain_desc.BufferDesc = back_buffer_desc;
	dxgi_swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgi_swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgi_swap_chain_desc.OutputWindow = hwnd_; //the win32 handle of the SDL2 window
	dxgi_swap_chain_desc.SampleDesc = dxgi_sample_desc;
	dxgi_swap_chain_desc.Windowed = true;

	//create a temporary swapchain
	IDXGISwapChain* temp_swap_chain;

	dxgiFactory->CreateSwapChain(
		command_queue_,
		&dxgi_swap_chain_desc,
		&temp_swap_chain
	);

	swap_chain_ = static_cast<IDXGISwapChain3*>(temp_swap_chain);

	//get the index of the back buffer
	frame_index_ = swap_chain_->GetCurrentBackBufferIndex();

	//create a descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {};
	descriptor_heap_desc.NumDescriptors = 3;
	descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	device_->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&rtv_descriptor_heap_));

	rtv_descriptor_size_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_handle(rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart());

	//create render target views and command allocators for each render target
	for (auto i = 0; i < 3; i++)
	{
		swap_chain_->GetBuffer(i, IID_PPV_ARGS(&render_targets_[i]));
		device_->CreateRenderTargetView(render_targets_[i], nullptr, rtv_handle);
		rtv_handle.Offset(1, rtv_descriptor_size_);
		device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator_[i]));
	}

	//create a command list for each framebuffer
	device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator_[frame_index_], nullptr,
	                           IID_PPV_ARGS(&command_list_));

	//create a synchronisation fence for each framebuffer
	for (auto i = 0; i < 3; i++)
	{
		device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_[i]));
		fence_value_[i] = 0;
	}

	fence_event_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	CD3DX12_ROOT_SIGNATURE_DESC dx12_root_signature_desc;
	dx12_root_signature_desc.Init(0,
	                              nullptr,
	                              0,
	                              nullptr,
	                              D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ID3DBlob* signature;
	D3D12SerializeRootSignature(&dx12_root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, nullptr);
	device_->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
	                             IID_PPV_ARGS(&root_signature_));

	//setup the vertex shader
	ID3DBlob* vertex_shader;
	ID3DBlob* error_buff;
	D3DCompileFromFile(L"shaders/VertexShader.hlsl",
	                   nullptr,
	                   nullptr,
	                   "main",
	                   "vs_5_0",
	                   D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
	                   0,
	                   &vertex_shader,
	                   &error_buff);

	D3D12_SHADER_BYTECODE d3_d12_shader_bytecode = {};
	d3_d12_shader_bytecode.BytecodeLength = vertex_shader->GetBufferSize();
	d3_d12_shader_bytecode.pShaderBytecode = vertex_shader->GetBufferPointer();

	//setup the pixel shader
	ID3DBlob* pixelShader;
	D3DCompileFromFile(L"shaders/PixelShader.hlsl",
	                   nullptr,
	                   nullptr,
	                   "main",
	                   "ps_5_0",
	                   D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION,
	                   0,
	                   &pixelShader,
	                   &error_buff);

	D3D12_SHADER_BYTECODE d12_shader_bytecode = {};
	d12_shader_bytecode.BytecodeLength = pixelShader->GetBufferSize();
	d12_shader_bytecode.pShaderBytecode = pixelShader->GetBufferPointer();

	//define the data that will be sent to the vertex shader
	D3D12_INPUT_ELEMENT_DESC inputLayout[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	D3D12_INPUT_LAYOUT_DESC d12_input_layout_desc = {};

	d12_input_layout_desc.NumElements = sizeof(inputLayout) / sizeof(D3D12_INPUT_ELEMENT_DESC);
	d12_input_layout_desc.pInputElementDescs = inputLayout;

	//define the graphics pipeline
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d12_graphics_pipeline_state_desc = {};
	d12_graphics_pipeline_state_desc.InputLayout = d12_input_layout_desc;
	d12_graphics_pipeline_state_desc.pRootSignature = root_signature_;
	d12_graphics_pipeline_state_desc.VS = d3_d12_shader_bytecode;
	d12_graphics_pipeline_state_desc.PS = d12_shader_bytecode;
	d12_graphics_pipeline_state_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	d12_graphics_pipeline_state_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	d12_graphics_pipeline_state_desc.SampleDesc = dxgi_sample_desc;
	d12_graphics_pipeline_state_desc.SampleMask = 0xffffffff;
	d12_graphics_pipeline_state_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	d12_graphics_pipeline_state_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	d12_graphics_pipeline_state_desc.NumRenderTargets = 1;


	device_->CreateGraphicsPipelineState(&d12_graphics_pipeline_state_desc, IID_PPV_ARGS(&pipeline_state_object_state_));


	//define the vertices to be sent to the GPU
	vertex vList[] = {
		{{0.0f, 0.5f, 0.5f},{1.0f, 0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.5f},{0.0f, 1.0f, 0.0f}},
		{{-0.5f, -0.5f, 0.5f},{0.0f, 0.0f, 1.0f}},
	};

	//create a vertex buffer and upload this data to the GPU

	int vBufferSize = sizeof(vList);

	device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize),
		D3D12_RESOURCE_STATE_COPY_DEST,

		nullptr,
		IID_PPV_ARGS(&vertex_buffer_));

	vertex_buffer_->SetName(L"Vertex Buffer Resource Heap");

	ID3D12Resource* buffer_upload_heap;
	device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(vBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer_upload_heap));
	buffer_upload_heap->SetName(L"Vertex Buffer Upload Resource Heap");


	D3D12_SUBRESOURCE_DATA d12_subresource_data = {};
	d12_subresource_data.pData = reinterpret_cast<BYTE*>(vList);
	d12_subresource_data.RowPitch = vBufferSize;
	d12_subresource_data.SlicePitch = vBufferSize;

	UpdateSubresources(command_list_, vertex_buffer_, buffer_upload_heap, 0, 0, 1, &d12_subresource_data);

	command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		                               vertex_buffer_, D3D12_RESOURCE_STATE_COPY_DEST,
		                               D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER));

	command_list_->Close();
	ID3D12CommandList* ppCommandLists[] = {command_list_};
	command_queue_->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	fence_value_[frame_index_]++;
	command_queue_->Signal(fence_[frame_index_], fence_value_[frame_index_]);

	vertex_buffer_view_.BufferLocation = vertex_buffer_->GetGPUVirtualAddress();
	vertex_buffer_view_.StrideInBytes = sizeof(vertex);
	vertex_buffer_view_.SizeInBytes = vBufferSize;

	//setup the viewport
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.Width = width_;
	viewport_.Height = height_;
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;

	//set up the scissor rect
	scissor_rect_.left = 0;
	scissor_rect_.top = 0;
	scissor_rect_.right = width_;
	scissor_rect_.bottom = height_;

	return true;
}

void d3dx12_application::wait_for_previous_frame()
{
	frame_index_ = swap_chain_->GetCurrentBackBufferIndex();
	if (fence_[frame_index_]->GetCompletedValue() < fence_value_[frame_index_])
	{
		fence_[frame_index_]->SetEventOnCompletion(fence_value_[frame_index_], fence_event_);
		WaitForSingleObject(fence_event_, INFINITE);
	}

	fence_value_[frame_index_]++;
}

void d3dx12_application::update_pipeline()
{
	//wait for the previous frame to finish rendering
	wait_for_previous_frame();

	//reset the allocated command list for this frame
	command_allocator_[frame_index_]->Reset();
	//reset the allocated command list for this frame
	command_list_->Reset(command_allocator_[frame_index_], pipeline_state_object_state_);

	//transition to the resource state of presentation from render target
	command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		                               render_targets_[frame_index_], D3D12_RESOURCE_STATE_PRESENT,
		                               D3D12_RESOURCE_STATE_RENDER_TARGET));
	//get a handle to the descriptor set
	CD3DX12_CPU_DESCRIPTOR_HANDLE render_target_view(rtv_descriptor_heap_->GetCPUDescriptorHandleForHeapStart(),
	                                                 frame_index_, rtv_descriptor_size_);
	//set the render target
	command_list_->OMSetRenderTargets(1, &render_target_view, FALSE, nullptr);
	//set the clear color
	const float clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
	command_list_->ClearRenderTargetView(render_target_view, clearColor, 0, nullptr);

	//set the pipeline
	command_list_->SetGraphicsRootSignature(root_signature_); //set the root signature
	command_list_->RSSetViewports(1, &viewport_); //set the viewport
	command_list_->RSSetScissorRects(1, &scissor_rect_); //set the scissor rect
	command_list_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); //set the vertex topology to triangle list
	command_list_->IASetVertexBuffers(0, 1, &vertex_buffer_view_); //set the vertex buffer to use

	//draw our triangle
	command_list_->DrawInstanced(3, 1, 0, 0);

	//transition to the resource state of render target from presentation
	command_list_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		                               render_targets_[frame_index_], D3D12_RESOURCE_STATE_RENDER_TARGET,
		                               D3D12_RESOURCE_STATE_PRESENT));

	command_list_->Close();
}

void d3dx12_application::render()
{
	//update the graphics pipeline
	update_pipeline();
	//obtain the command list
	ID3D12CommandList* id3_d12_command_lists[] = {command_list_};
	//execute each command list
	command_queue_->ExecuteCommandLists(_countof(id3_d12_command_lists), id3_d12_command_lists);
	//signal that the frame has finished rendering
	command_queue_->Signal(fence_[frame_index_], fence_value_[frame_index_]);
	//present the framge
	swap_chain_->Present(0, 0);
}

void d3dx12_application::cleanup()
{
	//destroy all framebuffers
	for (int i = 0; i < 3; ++i)
	{
		frame_index_ = i;
		wait_for_previous_frame();
	}

	//destroy all d3dx12 
	SAFE_RELEASE(device_);
	SAFE_RELEASE(swap_chain_);
	SAFE_RELEASE(command_queue_);
	SAFE_RELEASE(rtv_descriptor_heap_);
	SAFE_RELEASE(command_list_);

	for (int i = 0; i < 3; ++i)
	{
		SAFE_RELEASE(render_targets_[i]);
		SAFE_RELEASE(command_allocator_[i]);
		SAFE_RELEASE(fence_[i]);
	}

	SAFE_RELEASE(pipeline_state_object_state_);
	SAFE_RELEASE(root_signature_);
	SAFE_RELEASE(vertex_buffer_);
}

void d3dx12_application::mainloop()
{
	auto running = true;
	while (running)
	{
		SDL_Event event;
		//check for sdl events
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT) running = false;
		}
		//render the frame
		render();
	}
}

bool d3dx12_application::initialize_window()
{
	//Create a window using SDL
	auto* window = SDL_CreateWindow("DirectX12", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width_, height_, NULL);

	//Obtain the HWND of the window that SDL created
	SDL_SysWMinfo systemInfo;
	SDL_VERSION(&systemInfo.version);
	SDL_GetWindowWMInfo(window, &systemInfo);

	hwnd_ = systemInfo.info.win.window;

	return true;
}
