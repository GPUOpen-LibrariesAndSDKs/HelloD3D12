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

#ifndef ANTERU_D3D12_SAMPLE_D3D12SAMPLE_H_
#define ANTERU_D3D12_SAMPLE_D3D12SAMPLE_H_

#include <d3d12.h>
#include <dxgi.h>
#include <wrl.h>
#include <memory>

namespace AMD {
class Window;

///////////////////////////////////////////////////////////////////////////////
class D3D12Sample
{
public:
	D3D12Sample (const D3D12Sample&) = delete;
	D3D12Sample& operator= (const D3D12Sample&) = delete;

	D3D12Sample ();
	virtual ~D3D12Sample ();

	void Run (const int frameCount);

protected:
	int GetQueueSlot () const
	{
		return currentBackBuffer_;
	}

	static const int QUEUE_SLOT_COUNT = 3;

	static constexpr int GetQueueSlotCount ()
	{
		return QUEUE_SLOT_COUNT;
	}

	D3D12_VIEWPORT viewport_;
	D3D12_RECT rectScissor_;
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapChain_;
	Microsoft::WRL::ComPtr<ID3D12Device> device_;
	Microsoft::WRL::ComPtr<ID3D12Resource> renderTargets_ [QUEUE_SLOT_COUNT];
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue_;

	HANDLE frameFenceEvents_ [QUEUE_SLOT_COUNT];
	Microsoft::WRL::ComPtr<ID3D12Fence> frameFences_ [QUEUE_SLOT_COUNT];
	UINT64 currentFenceValue_;
	UINT64 fenceValues_[QUEUE_SLOT_COUNT];

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> renderTargetDescriptorHeap_;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pso_;

	virtual void InitializeImpl (ID3D12GraphicsCommandList* uploadCommandList);
	virtual void RenderImpl (ID3D12GraphicsCommandList* commandList);

private:
	void Initialize ();
	void Shutdown ();

	void PrepareRender ();
	void FinalizeRender ();

	void Render ();
	void Present ();

	void CreateDeviceAndSwapChain ();
	void CreateAllocatorsAndCommandLists ();
	void CreateViewportScissor ();
	void CreatePipelineStateObject ();
	void SetupSwapChain ();
	void SetupRenderTargets ();

	std::unique_ptr<Window> window_;

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocators_[QUEUE_SLOT_COUNT];
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandLists_[QUEUE_SLOT_COUNT];

	int currentBackBuffer_ = 0;
	
	std::int32_t renderTargetViewDescriptorSize_;
};
}

#endif
