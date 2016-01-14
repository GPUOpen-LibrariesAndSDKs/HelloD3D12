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

#include "D3D12AnimatedQuad.h"
#include "D3D12Quad.h"
#include "D3D12TexturedQuad.h"

int WinMain (
	_In_ HINSTANCE /* hInstance */,
	_In_opt_ HINSTANCE /* hPrevInstance */,
	_In_ LPSTR     /* lpCmdLine */,
	_In_ int       /* nCmdShow */
	)
{
	AMD::D3D12Sample* sample = nullptr;

	const auto sampleId = 0;

	switch (sampleId) {
	case 0: sample = new AMD::D3D12Quad; break;
	case 1: sample = new AMD::D3D12AnimatedQuad; break;
	case 2: sample = new AMD::D3D12TexturedQuad; break;
	}

	if (sample == nullptr) {
		return 1;
	}

	sample->Run (512);
	delete sample;

	return 0;
}