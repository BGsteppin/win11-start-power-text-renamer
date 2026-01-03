// ==WindhawkModReadme==
/*
# Windows 11 Start Power Text Renamer

## What this mod does
This mod allows you to rename the **Windows 11 Start menu power options**:
- Lock
- Sleep
- Shut down
- Restart

You can change each label to any custom text you want (for example: jokes, clearer wording, or personal preferences).

✅ The layout, icons, and theme are NOT modified  
✅ All original power actions continue to work normally  
✅ Changes are applied live while the Start menu is running  

---

## How it works
The mod runs inside `StartMenuExperienceHost.exe` and intercepts string creation
used by the Start menu. When one of the default power option texts is detected,
it is replaced with your custom text before being displayed.

This approach avoids fragile UI manipulation and keeps the mod lightweight.

---

## Settings
The mod provides simple settings:

- **Enable mod** – Turn the mod on or off
- **Log replacements** – Log each successful text replacement (for debugging)
- **Lock → your text**
- **Sleep → your text**
- **Shut down → your text**
- **Restart → your text**

Only the replacement text is exposed to keep the settings clean and easy to read.

---

## Supported systems
- Windows 11 (English UI)
- Tested with StartMenuExperienceHost.exe
- Should work on most Windows 11 builds, but future updates may require adjustments

---

## Limitations
- Text-only changes (no reordering, icons, or styling)
- English UI only (strings are hardcoded)
- If Microsoft changes how the Start menu generates text, the mod may stop working

---

## Troubleshooting
- If text does not change, enable **Log replacements** and check Windhawk logs
- Restart `StartMenuExperienceHost.exe` or sign out/in
- Ensure your custom text is not empty

---

## License
MIT License
*/
// ==/WindhawkModReadme==
