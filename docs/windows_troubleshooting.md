# Windows Error 0xc0000022 Troubleshooting Guide

## Error: "Application was unable to start correctly (0xc0000022)"

This error typically means STATUS_ACCESS_DENIED and can have several causes.

## Quick Fixes (Try These First)

### 1. Check File Permissions
- Right-click `pokerth_client.exe` → Properties → Security
- Ensure your user has "Full control" or at least "Read & Execute"
- Click "Advanced" → Ensure "Run as administrator" is NOT required (unless needed)

### 2. Disable DEP for This Executable
- Open Command Prompt as Administrator
- Run: `bcdedit /set {current} nx OptOut`
- Restart Windows
- Or add exception: System Properties → Advanced → Performance Settings → Data Execution Prevention → Add exception for pokerth_client.exe

### 3. Check Antivirus/Windows Defender
- Temporarily disable Windows Defender/antivirus
- Add the folder containing the game (e.g. the copied deploy folder) to exclusions
- Try running again

### 4. Verify All Files Copied Correctly
- Ensure entire deployment directory (e.g. `build_windows/deploy` or `docker/windows/build/deploy` when using Docker, copied to Windows) is present:
  ```
  deploy/
  ├── pokerth_client.exe
  ├── Qt6Core.dll
  ├── Qt6Gui.dll
  ├── Qt6Widgets.dll
  ├── Qt6Network.dll
  ├── Qt6Sql.dll
  ├── Qt6Xml.dll
  ├── Qt6Multimedia.dll
  ├── libgcc_s_seh-1.dll
  ├── libstdc++-6.dll
  ├── libwinpthread-1.dll
  ├── qt.conf
  ├── data/
  └── plugins/
      └── platforms/
          └── qwindows.dll
  ```

### 5. Use Dependency Walker on Windows
- Download Dependency Walker: http://www.dependencywalker.com/
- Open `pokerth_client.exe` in Dependency Walker
- Look for missing DLLs (marked in red)
- Check for version mismatches

### 6. Check Windows Event Viewer
- Press Win+R, type `eventvwr.msc`
- Go to Windows Logs → Application
- Look for errors related to `pokerth_client.exe`
- Check the details for specific DLL loading errors

### 7. Try Simple Path
- Copy the deploy folder to e.g. `C:\pokerth` (avoid spaces and special characters in the path)
- Run from there

### 8. Run from Command Prompt
- Open Command Prompt in the directory containing `pokerth_client.exe` (e.g. the copied `deploy` folder)
- Run: `pokerth_client.exe`
- This may show more detailed error messages

### 9. Check System Requirements
- Ensure you're on 64-bit Windows (executable is 64-bit)
- Windows 7 SP1 or later recommended
- Ensure Windows is up to date

### 10. Verify DLL Architecture
- All DLLs must be 64-bit (x64)
- Mixing 32-bit and 64-bit DLLs will cause this error
- Use `dumpbin /headers pokerth_client.exe` to verify architecture

## Advanced Diagnostics

### Check DLL Dependencies
On Windows, use PowerShell:
```powershell
Get-Item .\pokerth_client.exe | Select-Object -ExpandProperty VersionInfo
```

Or use `dumpbin`:
```cmd
dumpbin /dependents pokerth_client.exe
```

### Verify Qt Plugins
Ensure `qt.conf` contains:
```
[Paths]
Plugins = plugins
```

And `plugins/platforms/qwindows.dll` exists.

### Check MinGW Runtime Version
The MinGW runtime DLLs (libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll) must match the compiler that **linked** the exe (the system x86_64-w64-mingw32-g++). The build script copies them from the system toolchain first, then falls back to Qt's if needed.

---

## Error: "The procedure entry point ... could not be found"

This usually means a **DLL version mismatch**: the exe was linked against one version of a runtime (e.g. libstdc++-6.dll from your MinGW toolchain) but a different version was copied into the deploy folder (e.g. Qt's libstdc++, built with another GCC).

**Fix:** Rebuild and redeploy. The build script now **prefers the system MinGW runtime DLLs** (same toolchain that linked the exe) and only uses Qt's as fallback. Ensure you have the MinGW-w64 toolchain installed (`apt install mingw-w64`), then run the Windows build again and copy the new `deploy` folder to Windows.

## If Nothing Works

1. **Rebuild with static linking** (if possible) - eliminates DLL dependency issues
2. **Use Qt's windeployqt tool** - automatically copies all required DLLs:
   ```bash
   # From repo root (Linux host with Qt MinGW installed):
   windeployqt --dir build_windows/deploy build_windows/bin/pokerth_client.exe
   ```
3. **Check if Qt was built correctly** - verify Qt installation isn't corrupted
4. **Try a different Windows machine** - to rule out system-specific issues

## Build Failures on Linux (Before Copying to Windows)

If the Windows build fails on Ubuntu or when reusing a build directory, see **docs/building.md** (Linux Qt, Windows vcpkg, reconfigure) and **docs/building-developer.md** (Docker ensure, **ensure_docker_deps.py**, **docker/windows/build** cache).

## Common Causes Summary

| Cause | Solution |
|-------|----------|
| DEP blocking | Disable DEP or add exception |
| Antivirus blocking | Disable/whitelist directory |
| Missing DLL | Use Dependency Walker to find |
| **Procedure entry point ... could not be found** | DLL version mismatch; rebuild so deploy uses system MinGW DLLs (see above) |
| Wrong architecture | Ensure 64-bit Windows |
| Corrupted files | Re-copy all files |
| Path issues | Use simple path like C:\pokerth |
| Permissions | Check file/folder permissions |
