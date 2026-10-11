# Smart View companion lifecycle

The EXE installer installs the camera DLL under protected Program Files and
installs Smart View as a standard Kagami feature. Elevated setup trusts this
development build's public signing certificate in machine `TrustedPeople`, then
registers the MSIX as the original unprivileged desktop user. Reception remains
stopped.

Setup calls `Add-AppxPackage` for the same exact package identity on install and
upgrade. It does not uninstall first, force a downgrade, or force a running
Kagami/receiver to close. Close Kagami before installing an update. CI artifacts
use increasing package versions; rebuilding an old or equal version may require
a newer artifact rather than silently replacing the installed app.

Before removing the EXE installation, use the Start-menu shortcut **Remove
Smart View receiver (current user)** from the normal desktop account that
installed it. Alternatively, remove **Kagami Development Preview** from
Windows Settings > Apps > Installed apps. The shortcut invokes
`Install-SmartView.ps1 -Uninstall`; it selects only the exact Kagami development
package name and publisher for the current user. It never enumerates or removes
other users' packages. Each user who registered the companion removes their own
registration.

The elevated EXE uninstaller cannot reliably recover the original desktop user,
particularly when UAC uses a different administrator account. Inno Setup's
`ExecAsOriginalUser` is unavailable during uninstall. Therefore EXE uninstall
does **not** automatically remove per-user Smart View registrations. If the EXE
has already been removed, use Windows Settings to remove the companion. This is
an explicit cleanup gap, not complete automatic companion removal.

Kagami preferences use unvirtualized `HKCU\Software\Kagami\Desktop` and are not
deleted by the cleanup script. The development signing certificate also remains
in TrustedPeople: another desktop user or development build may still depend on
it, so removing it based only on its subject would not establish ownership.

This lifecycle needs installed upgrade/uninstall acceptance testing. Parsing,
building, signing, and packaging do not prove installed Smart View compatibility.

References: [Inno Setup ExecAsOriginalUser](https://jrsoftware.org/ishelp/topic_isxfunc_execasoriginaluser.htm),
[Microsoft Remove-AppxPackage](https://learn.microsoft.com/en-us/powershell/module/appx/remove-appxpackage).

