Unreal Engine with VXGI and HBAO+
=================================

**Note**: for up-to-date information on Unreal Engine in general, please refer to [README by Epic Games](https://github.com/EpicGames/UnrealEngine/blob/release/README.md).

Introduction
------------

VXGI stands for [Voxel Global Illumination](http://www.geforce.com/hardware/technology/vxgi), and it's an advanced rendering technique for real-time indirect illumination.

Global illumination (GI) is a way of computing lighting in the scene that includes indirect illumination, i.e. simulating objects that are lit by other objects as well as ideal light sources. Adding GI to the scene greatly improves the realism of the rendered images. Modern real-time rendering engines simulate indirect illumination using different approaches, which include precomputed light maps (offline GI), local light sources placed by artists, and simple ambient light.

![Alt text](CornellBoxLightingModes.png "Cornell Box Scene")

HBAO+ stands for [Horizon-Based Ambient Occlusion Plus](http://www.geforce.com/hardware/technology/hbao-plus), and it's a fast and relatively stable screen-space ambient occlusion solution.

How to Build
------------

First, you need to download the source code. For beginners, it's easier to download a .zip file (look for the "Download ZIP" button on top of this page). Alternatively, you can use a Git client and clone the repository from GitHub onto your machine, then switch to the right branch - this way, keeping your copy up to date will be much easier. 

When you have the source code on your local hard drive, follow these steps to build it:

1.  Run Setup.bat, answer "N" when it asks you about overwriting some files - otherwise it will replace new NVAPI libraries with old ones, and UE4 won't build.

2.	Run GenerateProjectFiles.bat
	
3.	Open UE4.sln, build UE4 and **ShaderCompileWorker** projects.

4.	Run UE4 Editor, open CornellBox or SciFiHallway projects.

How to Use
----------

Please see the [overview document](UE4_VXGI_Overview.pdf).

FAQ
---

**Q:** Is VXGI a plug-in for Unreal Engine? Can it be combined with other GameWorks techs, such as WaveWorks or HairWorks?
**A:** No, it's a separate branch of the engine. The UE plug-in interface is very limiting, and complicated technologies like VXGI cannot work through it. In order to combine several GameWorks technologies, you have to merge the corresponding branches. Alternatively, you can use a third-party merged branch, like [this one](https://github.com/GalaxyMan2015/UnrealEngine/tree/4.9.2_NVIDIA_Techs) (not supported by NVIDIA).

**Q:** What are the minimum and recommended PC system requirements to run VXGI?
**A:** Minimum: a 64-bit Windows 7 system with any DirectX 11 class GPU. Recommended: a fast 4+ core processor (more is better because VXGI needs to compile a few heavy shaders for every material), 16 GB of system memory, and an NVIDIA GeForce GTX 9xx series GPU (GM20x or newer architecture).

**Q:** Do I have to build the engine from source to use VXGI, or there is a binary distribution available somewhere?
**A:** Currently there are no binary distributions, so you have to build it. It's not that hard, all you need is a free edition of Microsoft Visual Studio 2013 or 2015.

**Q:** I loaded a map but there is no indirect lighting.
**A:** Please make sure that...

- Console variable r.VXGI.DiffuseTracingEnable is set to 1
- Directly lit or emissive materials have "Used with VXGI Voxelization" box checked
- Direct lights are Movable and have "VXGI Indirect Lighting" box checked
- There is an active PostProcessVolume and the camera is inside it (or it's unbounded)
- In the PostProcessVolume, the "Settings/VXGI Diffuse/Enable Diffuse Tracing" box is checked

It is also useful to switch the View mode to "VXGI Opacity Voxels" or "VXGI Emittance Voxels" to make sure that the objects you need are represented as voxels and emit (or reflect) light.

**Q:** I'm trying to build the engine, and there are some linker errors related to NVAPI.
**A:** This means Setup.bat has overwritten the NVAPI libraries with older versions. You need to copy the right version (from the original zip file or from GitHub) of this file: `Engine\Source\ThirdParty\NVIDIA\nvapi\amd64\nvapi64.lib`

**Q:** There are no specular reflections on translucent objects, how do I add them?
**A:** You need to modify the translucent material and make it trace specular cones. See [this forum post](https://forums.unrealengine.com/showthread.php?53735-NVIDIA-GameWorks-Integration&p=423841&highlight=vxgi#post423841) for an example.

**Q:** Can specular reflections be less blurry?
**A:** Usually yes, but there is a limit. The quality of reflections is determined by the size of voxels representing the reflected object(s), so you need to reduce that size. There are several ways to do that:

- Place a "VXGI Anchor" actor near the reflected objects. VXGI's scene representation has a region where it is most detailed, and this actor controls the location of that region.
- Reduce r.VXGI.Range, which will make all voxels smaller, but also obviously reduce the range of VXGI effects.
- Increase r.VXGI.MapSize, but there are only 3 options for that parameter: 64, 128 and 256, and the latter is extremely expensive.

**Q:** Is it possible to pre-compute lighting with VXGI to use on low-end PCs or mobile devices?
**A:** No, as VXGI was designed as a fully dynamic solution. It is theoretically possible to use VXGI cone tracing to bake light maps, but such feature is not implemented, and it doesn't add enough value compared to traditional light map solutions like Lightmass: the only advantage is that baking will be faster.

**Q:** Does VXGI support DirectX 12?
**A:** It does, but in a limited and still experimental way. Switching to DX12 is not yet recommended. It will be slower than on DX11.

**Q:** Can I use VXGI in my own rendering engine, without Unreal Engine?
**A:** Yes. The SDK package is available on [NVIDIA GameWorks Developer website](https://developer.nvidia.com/vxgi).


Tech Support
------------

This branch of UE4 is primarily discussed on the Unreal Engine forums: [NVIDIA GameWorks Integration](https://forums.unrealengine.com/showthread.php?53735-NVIDIA-GameWorks-Integration). That forum thread contains many questions and answers, and some NVIDIA engineers also participate in the discussion. For VXGI related questions, comment on that thread, or contact [Alexey.Panteleev](https://forums.unrealengine.com/member.php?29363-Alexey-Panteleev) on the forum, or post an issue on GitHub.

Additional Resources
--------------------

- [Interactive Indirect Illumination Using Voxel Cone Tracing](http://maverick.inria.fr/Publications/2011/CNSGE11b/GIVoxels-pg2011-authors.pdf) - the original paper on voxel cone tracing.
- [NVIDIA VXGI: Dynamic Global Illumination for Games](http://on-demand.gputechconf.com/gtc/2015/presentation/S5670-Alexey-Panteleev.pdf) - a presentation about VXGI basics and its use in UE4.
- [Practical Real-Time Voxel-Based Global Illumination for Current GPUs](http://on-demand.gputechconf.com/gtc/2014/presentations/S4552-rt-voxel-based-global-illumination-gpus.pdf) - a technical presentation about VXGI while it was still work-in-progress.

Licensing
---------

This branch of UE4 is covered by the general [Unreal Engine End User License Agreement](LICENSE.pdf). There is no additional registration or fees associated with the use of VXGI or HBAO+ within Unreal Engine.

Acknowledgements
----------------

The VXGI integration was ported from UE 4.12 to 4.13 mostly by Unreal Engine Forums and GitHub user "GalaxyMan2015". 
