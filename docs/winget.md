# WinGet distribution

The intended public install command is:

```powershell
winget install celestial-sora.Kagami
```

This command is **not available yet**. Kagami must have an anonymously downloadable release installer and an accepted entry in Microsoft's WinGet community catalog. Actions development artifacts do not meet the public download requirement. The current foundation has no phone receiver and must not be advertised as a finished receiver.

The Inno Setup EXE performs installation, upgrades and uninstall. WinGet downloads that same EXE and verifies its SHA-256; it does not require Git, Python, npm or Visual Studio on users' machines. Machine scope is required for the protected native camera DLL. Setup elevation does not enable capture; silent setup does not open the app. Optional OBS remains unchecked.

## Prepare a version

Build and test the installer using `tools/build.ps1 -Installer`. Generate the three catalog YAML files from that exact installer and a versioned release URL:

```powershell
./tools/New-WinGetManifest.ps1 `
  -InstallerPath ./dist/Kagami-Setup-0.1.0-dev-x64.exe `
  -InstallerUrl https://github.com/celestial-sora/kagami-win/releases/download/v0.1.0-dev/Kagami-Setup-0.1.0-dev-x64.exe
winget validate ./build/winget/manifests/c/celestial-sora/Kagami/0.1.0
```

That URL is an example for preparation, not a published release. Output is ignored under `build/winget`. Package version, display name and publisher are read from the Inno recipe; the hash comes from the built EXE. The current installer reports `0.1.0` in Apps & Features, even though its filename marks it as a development preview. Do not reuse that package version for different published bytes. Future packaging changes must update the version and preview description together.

After a release is approved and published, rerun with `-VerifyPublishedAsset`. It downloads the public asset and requires its SHA-256 to match the local installer. Then run `winget validate` again. This script does not publish releases, change WinGet settings or install/register a camera.

Before catalog submission, complete actual setup, upgrade, silent-install and uninstall acceptance on Windows 11 x64, including camera use in OBS/Discord. Validation of YAML does not validate those behaviors or phone compatibility. Stable releases still require guarded main CI approval under AGENTS.md.

## Catalog submission

Microsoft's public catalog is maintained in `microsoft/winget-pkgs`; submission uses a pull request there and Microsoft reviews it. Project changes continue through the feature-branch/CI/direct-main process without a Kagami PR. Obtain maintainer agreement on the external catalog submission before opening it under the current no-PR direction. The short install command becomes usable only after Microsoft accepts and distributes the entry. Each later release needs an updated version/hash/URL and catalog submission.

Sources: [Microsoft manifest instructions](https://learn.microsoft.com/en-us/windows/package-manager/package/manifest) and [Microsoft catalog submission](https://learn.microsoft.com/en-us/windows/package-manager/package/repository).
