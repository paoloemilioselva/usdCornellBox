usdCornellBox
-------------

Procedurally generated Cornell Box to fit any asset loaded/drag&dropped in the viewer.
Use it as example for a standalone OpenUSD app or as screensaver 

https://github.com/paoloemilioselva/usdCornellBox/assets/1212607/a7de8ca1-365b-43ac-a1f3-41a33fa55968

Dependencies
------------

- `Windows 11`
- `VisualStudio 2022`
- `GLFW` from github ( https://github.com/glfw/glfw )

Pixar's OpenUSD configuration embedded:
- `OpenUSD 23.11` from github ( https://github.com/PixarAnimationStudios/OpenUSD )
  - `python-3.11.7` (web installer is fine)
  - `git` for windows
  - Use `x64 Native Tools Prompt from VisualStudio 2022` for your compilation
    - `pip install pyside6`
    - `pip install pyopengl`
  - OPTIONAL:
    - `Renderman 25.2` (non-commercial) (if you want it as renderer in your standalone app)

- it will also install all the dependencies to use a localised openusd build for a redistributable app.
- Note this will create about 300MB of data in the output install bin folder.

Compiling Pixar's OpenUSD
-------------------------

After cloning the git repo, using the `x64 Native Tools Prompt from Visual Studio 2022`, 
cd into its main folder and launch:

```
python build_scripts\build_usd.py "C:\dev\usd-23.11" --embree --openimageio 
  --test --prman --prman-location "C:\Program Files\Pixar\RenderManProServer-25.2"
```

NOTE: `--prman` and `--prman-location` are only needed if you download `Renderman` 
and you want to use it as render-delegate in your app

NOTE2: `--test` could be skipped, but it is good to use it there, for future projects

Using Visual Studio 2022 with this project
------------------------------------------

Open the folder of this project in Visual Studio.
The cmake configurations will appear at the top, you can switch between the USD or the Houdini one, 
adjusting paths to folders if you have different ones.

Build the project via right-click on the `CMakeLists.txt` file and selecting `Build` and then `Install`.

Once installed, you can go in the `out/install/` folder, and the configuration subfolder you installed, 
and launch `launcher.bat`.

The batch-file is setting up all environments needed to launch your standalone app.

Resources
---------

"Pieta A 3D Tribute to Michelangelo's Masterpiece" (https://skfb.ly/oMIDG) 
by SUJESH NAIR is licensed under Creative Commons Attribution (http://creativecommons.org/licenses/by/4.0/).
