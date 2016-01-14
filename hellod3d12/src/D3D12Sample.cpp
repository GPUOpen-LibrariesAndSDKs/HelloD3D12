//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "D3D12Sample.h"

#include <dxgi1_4.h>
#include "d3dx12.h"
#include <iostream>
#include <d3dcompiler.h>
#include "Shaders.h"
#include <algorithm>

#include "ImageIO.h"
#include "Window.h"

#ifdef max 
#undef max
#endif

using namespace Microsoft::WRL;

namespace AMD {
namespace {
struct RenderEnvironment
{
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12CommandQueue> queue;
	ComPtr<IDXGISwapChain> swapChain;
};

///////////////////////////////////////////////////////////////////////////////
/**
Create everything we need for rendering, this includes a device, a command queue,
and a swap chain.
*/
RenderEnvironment CreateDeviceAndSwapChainHelper (
	_In_opt_ IDXGIAdapter* adapter,
	D3D_FEATURE_LEVEL minimumFeatureLevel,
	_In_ const DXGI_SWAP_CHAIN_DESC* swapChainDesc)
{
	RenderEnvironment result;

	auto hr = D3D12CreateDevice (adapter, minimumFeatureLevel,
		IID_PPV_ARGS (&result.device));

	if (FAILED (hr)) {
		throw std::runtime_error ("Device creation failed.");
	}

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	hr = result.device->CreateCommandQueue (&queueDesc, IID_PPV_ARGS (&result.queue));

	if (FAILED (hr)) {
		throw std::runtime_error ("Command queue creation failed.");
	}

	ComPtr<IDXGIFactory4> dxgiFactory;
	hr = CreateDXGIFactory1 (IID_PPV_ARGS (&dxgiFactory));

	if (FAILED (hr)) {
		throw std::runtime_error ("DXGI factory creation failed.");
	}

	// Must copy into non-const space
	DXGI_SWAP_CHAIN_DESC swapChainDescCopy = *swapChainDesc;
	hr = dxgiFactory->CreateSwapChain (
		result.queue.Get (),
		&swapChainDescCopy,
		&result.swapChain
		);

	if (FAILED (hr)) {
		throw std::runtime_error ("Swap chain creation failed.");
	}

	return result;
}
}

///////////////////////////////////////////////////////////////////////////////
D3D12Sample::D3D12Sample ()
{
}

///////////////////////////////////////////////////////////////////////////////
D3D12Sample::~D3D12Sample ()
{
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::PrepareRender ()
{
	commandAllocators_ [currentBackBuffer_]->Reset ();

	auto commandList = commandLists_ [currentBackBuffer_].Get ();
	commandList->Reset (
		commandAllocators_ [currentBackBuffer_].Get (), nullptr);

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE::InitOffsetted (renderTargetHandle,
		renderTargetDescriptorHeap_->GetCPUDescriptorHandleForHeapStart (),
		currentBackBuffer_, renderTargetViewDescriptorSize_);

	commandList->OMSetRenderTargets (1, &renderTargetHandle, true, nullptr);
	commandList->RSSetViewports (1, &viewport_);
	commandList->RSSetScissorRects (1, &rectScissor_);

	// Transition back buffer
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Transition.pResource = renderTargets_ [currentBackBuffer_].Get ();
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	commandList->ResourceBarrier (1, &barrier);

	static const float clearColor [] = {
		0.042f, 0.042f, 0.042f,
		1
	};

	commandList->ClearRenderTargetView (renderTargetHandle,
		clearColor, 0, nullptr);
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::Render ()
{
	PrepareRender ();
	
	auto commandList = commandLists_ [currentBackBuffer_].Get ();

	RenderImpl (commandList);
	
	FinalizeRender ();
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::RenderImpl (ID3D12GraphicsCommandList* commandList)
{
	// Set our state (shaders, etc.)
	commandList->SetPipelineState (pso_.Get ());

	// Set our root signature
	commandList->SetGraphicsRootSignature (rootSignature_.Get ());
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::FinalizeRender ()
{
	// Transition the swap chain back to present
	D3D12_RESOURCE_BARRIER barrier;
	barrier.Transition.pResource = renderTargets_ [currentBackBuffer_].Get ();
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	auto commandList = commandLists_ [currentBackBuffer_].Get ();
	commandList->ResourceBarrier (1, &barrier);

	commandList->Close ();

	// Execute our commands
	ID3D12CommandList* commandLists [] = { commandList };
	commandQueue_->ExecuteCommandLists (std::extent<decltype(commandLists)>::value, commandLists);
}

namespace {
void WaitForFence (ID3D12Fence* fence, UINT64 completionValue, HANDLE waitEvent)
{
	if (fence->GetCompletedValue () < completionValue) {
		fence->SetEventOnCompletion (completionValue, waitEvent);
		WaitForSingleObject (waitEvent, INFINITE);
	}
}
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::Run (const int frameCount)
{
	Initialize ();

	for (int i = 0; i < frameCount; ++i) {
		WaitForFence (frameFences_[GetQueueSlot ()].Get (), 
			fenceValues_[GetQueueSlot ()], frameFenceEvents_[GetQueueSlot ()]);
		
		Render ();
		Present ();
	}

	// Drain the queue, wait for everything to finish
	for (int i = 0; i < GetQueueSlotCount (); ++i) {
		WaitForFence (frameFences_[i].Get (), fenceValues_[i], frameFenceEvents_[i]);
	}

	Shutdown ();
}

///////////////////////////////////////////////////////////////////////////////
/**
Setup all render targets. This creates the render target descriptor heap and
render target views for all render targets.

This function does not use a default view but instead changes the format to
_SRGB.
*/
void D3D12Sample::SetupRenderTargets ()
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = GetQueueSlotCount ();
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	device_->CreateDescriptorHeap (&heapDesc, IID_PPV_ARGS (&renderTargetDescriptorHeap_));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle{ 
		renderTargetDescriptorHeap_->GetCPUDescriptorHandleForHeapStart () };

	for (int i = 0; i < GetQueueSlotCount (); ++i) {
		D3D12_RENDER_TARGET_VIEW_DESC viewDesc;
		viewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipSlice = 0;
		viewDesc.Texture2D.PlaneSlice = 0;

		device_->CreateRenderTargetView (renderTargets_ [i].Get (), &viewDesc,
			rtvHandle);

		rtvHandle.Offset (renderTargetViewDescriptorSize_);
	}
}

///////////////////////////////////////////////////////////////////////////////
/**
Present the current frame by swapping the back buffer, then move to the
next back buffer and also signal the fence for the current queue slot entry.
*/
void D3D12Sample::Present ()
{
	swapChain_->Present (1, 0);

	// Mark the fence for the current frame.
	const auto fenceValue = currentFenceValue_;
	commandQueue_->Signal (frameFences_ [currentBackBuffer_].Get (), fenceValue);
	fenceValues_[currentBackBuffer_] = fenceValue;
	++currentFenceValue_;

	// Take the next back buffer from our chain
	currentBackBuffer_ = (currentBackBuffer_ + 1) % GetQueueSlotCount ();
}

///////////////////////////////////////////////////////////////////////////////
/**
Set up swap chain related resources, that is, the render target view, the
fences, and the descriptor heap for the render target.
*/
void D3D12Sample::SetupSwapChain ()
{
	currentFenceValue_ = 1;

	// Create fences for each frame so we can protect resources and wait for
	// any given frame
	for (int i = 0; i < GetQueueSlotCount (); ++i) {
		frameFenceEvents_ [i] = CreateEvent (nullptr, FALSE, FALSE, nullptr);
		fenceValues_ [i] = 0;
		device_->CreateFence (0, D3D12_FENCE_FLAG_NONE, 
			IID_PPV_ARGS (&frameFences_ [i]));
	}

	for (int i = 0; i < GetQueueSlotCount (); ++i) {
		swapChain_->GetBuffer (i, IID_PPV_ARGS (&renderTargets_ [i]));
	}

	SetupRenderTargets ();
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::Initialize ()
{
	window_.reset (new Window ("AMD HelloD3D12", 1280, 720));

	CreateDeviceAndSwapChain ();
	CreateAllocatorsAndCommandLists ();
	CreateViewportScissor ();
	
	// Create our upload fence, command list and command allocator
	// This will be only used while creating the mesh buffer and the texture
	// to upload data to the GPU.
	ComPtr<ID3D12Fence> uploadFence;
	device_->CreateFence (0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS (&uploadFence));

	ComPtr<ID3D12CommandAllocator> uploadCommandAllocator;
	device_->CreateCommandAllocator (D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS (&uploadCommandAllocator));
	ComPtr<ID3D12GraphicsCommandList> uploadCommandList;
	device_->CreateCommandList (0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		uploadCommandAllocator.Get (), nullptr,
		IID_PPV_ARGS (&uploadCommandList));

	InitializeImpl (uploadCommandList.Get ());

	uploadCommandList->Close ();

	// Execute the upload and finish the command list
	ID3D12CommandList* commandLists [] = { uploadCommandList.Get () };
	commandQueue_->ExecuteCommandLists (std::extent<decltype(commandLists)>::value, commandLists);
	commandQueue_->Signal (uploadFence.Get (), 1);

	auto waitEvent = CreateEvent (nullptr, FALSE, FALSE, nullptr);

	if (waitEvent == NULL) {
		throw std::runtime_error ("Could not create wait event.");
	}

	WaitForFence (uploadFence.Get (), 1, waitEvent);

	// Cleanup our upload handle
	uploadCommandAllocator->Reset ();

	CloseHandle (waitEvent);
}

void D3D12Sample::InitializeImpl (ID3D12GraphicsCommandList * /*uploadCommandList*/)
{
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::Shutdown ()
{
	for (auto event : frameFenceEvents_) {
		CloseHandle (event);
	}
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::CreateDeviceAndSwapChain ()
{
	// Enable the debug layers when in debug mode
	// If this fails, install the Graphics Tools for Windows. On Windows 10,
	// open settings, Apps, Apps & Features, Optional features, Add Feature,
	// and add the graphics tools
#ifdef _DEBUG
	ComPtr<ID3D12Debug> debugController;
	D3D12GetDebugInterface (IID_PPV_ARGS (&debugController));
	debugController->EnableDebugLayer ();
#endif

	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	::ZeroMemory (&swapChainDesc, sizeof (swapChainDesc));

	swapChainDesc.BufferCount = GetQueueSlotCount ();
	// This is _UNORM but we'll use a _SRGB view on this. See 
	// SetupRenderTargets() for details, it must match what
	// we specify here
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferDesc.Width = window_->GetWidth ();
	swapChainDesc.BufferDesc.Height = window_->GetHeight ();
	swapChainDesc.OutputWindow = window_->GetHWND ();
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Windowed = true;

	auto renderEnv = CreateDeviceAndSwapChainHelper (nullptr, D3D_FEATURE_LEVEL_11_0,
		&swapChainDesc);

	device_ = renderEnv.device;
	commandQueue_ = renderEnv.queue;
	swapChain_ = renderEnv.swapChain;

	renderTargetViewDescriptorSize_ =
		device_->GetDescriptorHandleIncrementSize (D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	SetupSwapChain ();
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::CreateAllocatorsAndCommandLists ()
{
	for (int i = 0; i < GetQueueSlotCount (); ++i) {
		device_->CreateCommandAllocator (D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS (&commandAllocators_ [i]));
		device_->CreateCommandList (0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			commandAllocators_ [i].Get (), nullptr,
			IID_PPV_ARGS (&commandLists_ [i]));
		commandLists_ [i]->Close ();
	}
}

///////////////////////////////////////////////////////////////////////////////
void D3D12Sample::CreateViewportScissor ()
{
	rectScissor_ = { 0, 0, window_->GetWidth (), window_->GetHeight () };

	viewport_ = { 0.0f, 0.0f,
		static_cast<float>(window_->GetWidth ()),
		static_cast<float>(window_->GetHeight ()),
		0.0f, 1.0f
	};
}
}