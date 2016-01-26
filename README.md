Hello D3D12
===========

HelloD3D12 is a small, introductory Direct3D 12 sample, which shows how to set up a window and render a textured quad complete with proper uploading handling, multiple frames queued, and constant buffers.

License
-------

MIT: see `LICENSE.txt` for details.

System requirements
-------------------

* A graphics card with Direct3D 12 support.
  * For instance, any GCN-based AMD Radeon&trade; GPU.
* Windows&reg; 10 (64-bit recommended).
* Visual Studio&reg; 2015 with Visual C++&reg; and the Windows 10 SDK installed. The [free community edition](https://www.visualstudio.com/downloads/download-visual-studio-vs) is sufficient.
  * Note: neither Visual C++ nor the Windows 10 SDK are installed with Visual Studio 2015 by default.
  * For a fresh installation of Visual Studio 2015, choose 'Custom', not 'Typical', and select the required components.
  * For existing installations, you can re-run the installer or select Visual Studio from Programs and Features and click 'Change'.
  * When selecting components to be installed, the option to install the Windows 10 SDK is tucked away under Windows and Web Development -> Universal Windows App Development Tools.

Building
--------

Visual Studio files can be found in the `hellod3d12\build` directory.

If you need to regenerate the Visual Studio files, open a command prompt in the `hellod3d12\premake` directory and run `..\..\premake\premake5.exe vs2015` (or `..\..\premake\premake5.exe vs2013` for Visual Studio 2013.)

Sample overview
---------------

There are three samples with increasing complexity. Common functionality, like window creation, present handling is part of `hellod3d12\src\D3D12Sample.cpp`. The rest is scaffolding of very minor interest; `ImageIO` has some helper classes to load an image from disk using WIC, `Window` contains a class to create a Win32 Window.

The samples are:

* `D3D12Quad`: Renders a quad. This is the most basic sample.
* `D3D12AnimatedQuad`: Animates the quad by using a constant buffer.
* `D3D12TexturedQuad`: Adds a texture to the animated quad sample.

Points of interest
------------------

* The application queues multiple frames. To protect the per-frame command lists and other resources, fences are created. After the command list for a frame is submitted, the fence is signaled and the next command list is used.
* The texture and mesh data is uploaded using an upload heap. This happens during the initialization and shows how to transfer data to the GPU. Ideally, this should be running on the copy queue but for the sake of simplicity it is run on the general graphics queue.
* Constant buffers are placed in an `upload` heap. Placing them in the upload heap is best if the buffers are read once.
* Barriers are as specific as possible and grouped. Transitioning many resources in one barrier is faster than using multiple barriers as the GPU have to flush caches, and if multiple barriers are grouped, the caches are only flushed once.
* The application uses a root signature slot for the most frequently changing constant buffer.
* The `DEBUG` configuration will automatically enable the debug layers to validate the API usage. Check the source code for details, as this requires the graphics tools to be installed.

Third-party software
------------------

* Premake is distributed under the terms of the BSD License. See `premake\LICENSE.txt`.

Attribution
------------------
* AMD, the AMD Arrow logo, Radeon, and combinations thereof are either registered trademarks or trademarks of Advanced Micro Devices, Inc. in the United States and/or other countries.
* Microsoft, Direct3D, DirectX, Visual Studio, Visual C++, and Windows are either registered trademarks or trademarks of Microsoft Corporation in the United States and/or other countries.
